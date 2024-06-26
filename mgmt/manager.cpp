#include "mgmt/manager.h"
#include "mgmt/gen_config.h"
#include "mgmt/messages.h"
#include "mgmt/priv/http_server.h"
#include "log/tg_log.h"
#include "put/tg_assert.h"

namespace mgmt // management
{

class manager_impl
{
    using config_type        = manager::config;
    using target_type        = mgmt::priv::http_server::target_type;
    using req_body_type      = mgmt::priv::http_server::req_body_type;
    using resp_body_type     = mgmt::priv::http_server::resp_body_type;
    using error_info_type    = mgmt::priv::http_server::error_info_type;
    using resp_callback_type = mgmt::priv::http_server::callback_type;
    using req_handler_type   = void (manager_impl::*)(req_body_type,
                                                    resp_callback_type&&);
    using req_handlers_type =
        bcont::flat_map<std::string_view, req_handler_type>;

private:
    baio_context io_ctx_;
    mgmt::priv::http_server http_server_;
    inc_messages_queue* inc_queue_;
    out_messages_queue* out_queue_;

    req_handlers_type req_handlers_;
    resp_callback_type start_cb_;
    resp_callback_type stop_cb_;
    resp_callback_type stats_cb_;

public:
    explicit manager_impl(const config_type&);

    void process_events() noexcept;

private: // Callbacks called by the embedded HTTP server
    void
    on_http_request(target_type, req_body_type, resp_callback_type&&) noexcept;
    void on_http_server_error(error_info_type, bsys::error_code) noexcept;

private:
    void init_req_handlers() noexcept;

    void on_req_start_gen(req_body_type, resp_callback_type&&) noexcept;
    void on_req_stop_gen(req_body_type, resp_callback_type&&) noexcept;
    void on_req_get_stats(req_body_type, resp_callback_type&&) noexcept;

    void on_inc_msg(mgmt::res_start_generation&&) noexcept;
    void on_inc_msg(mgmt::res_stop_generation&&) noexcept;
    void on_inc_msg(mgmt::res_stats_report&&) noexcept;
    void on_inc_msg(mgmt::generation_report&&) noexcept;

    template <typename... Args>
    static resp_body_type make_response_body(fmt::format_string<Args...>,
                                             Args&&...) noexcept;
};

////////////////////////////////////////////////////////////////////////////////

manager_impl::manager_impl(const config_type& cfg)
: http_server_(io_ctx_, cfg.endpoint)
, inc_queue_(cfg.inc_queue)
, out_queue_(cfg.out_queue)
{
    TG_LOG_INFO("Started management server at {}\n", cfg.endpoint);

    init_req_handlers();

    http_server_.start_running(
        [this](target_type t, req_body_type b, resp_callback_type&& c) {
            on_http_request(t, b, std::move(c));
        },
        [this](error_info_type i, bsys::error_code e) {
            on_http_server_error(i, e);
        });
}

void manager_impl::process_events() noexcept
{
    io_ctx_.poll_one();
    inc_queue_->dequeue([this](auto&& msg) { on_inc_msg(std::move(msg)); });
}

void manager_impl::on_http_request(target_type target,
                                   req_body_type req_body,
                                   resp_callback_type&& cb) noexcept
{
    if (auto it = req_handlers_.find(target); it != req_handlers_.end()) {
        std::invoke(it->second, this, req_body, std::move(cb));
    } else {
        TG_LOG_INFO("Got request for invalid target: {}\n", target);
        cb(bhttp::status::not_found,
           make_response_body("Invalid target: {}", target));
    }
}

void manager_impl::on_http_server_error(error_info_type info,
                                        bsys::error_code ec) noexcept
{
    TG_LOG_ERROR("Management server error: {}. {}\n", info, ec);
}

void manager_impl::init_req_handlers() noexcept
{
    // It's safe to use such string views as keys because they are views to
    // string literals which are part of the binary.
    using namespace std::string_view_literals;
    req_handlers_["/start_gen"sv] = &manager_impl::on_req_start_gen;
    req_handlers_["/stop_gen"sv]  = &manager_impl::on_req_stop_gen;
    req_handlers_["/get_stats"sv] = &manager_impl::on_req_get_stats;
}

void manager_impl::on_req_start_gen(req_body_type req,
                                    resp_callback_type&& cb) noexcept
{
    TG_LOG_INFO("Got start generation request\n");
    if (start_cb_) {
        TG_LOG_INFO("Start already in progress\n");
        cb(bhttp::status::precondition_failed,
           make_response_body("Start already in progress"));
        return;
    }
    try {
        auto cfg = std::make_unique<mgmt::gen_config>(req);
        if (out_queue_->enqueue(req_start_generation{std::move(cfg)})) {
            TG_LOG_INFO("Enqueued start generation request\n");
            start_cb_ = std::move(cb);
        } else {
            TG_LOG_INFO("Failed to enqueue start generation request\n");
            cb(bhttp::status::internal_server_error,
               make_response_body("Failed to enqueue request"));
        }
    } catch (const std::exception& ex) {
        TG_LOG_INFO("Invalid generation configuration: {}\n", ex.what());
        cb(bhttp::status::bad_request,
           make_response_body("Invalid generation configuration: {}",
                              ex.what()));
    }
}

void manager_impl::on_req_stop_gen(req_body_type,
                                   resp_callback_type&& cb) noexcept
{
    TG_LOG_INFO("Got stop generation request\n");
    if (stop_cb_) {
        TG_LOG_INFO("Stop already in progress\n");
        cb(bhttp::status::precondition_failed,
           make_response_body("Stop already in progress"));
        return;
    }
    if (out_queue_->enqueue(req_stop_generation{})) {
        TG_LOG_INFO("Enqueued stop generation request\n");
        stop_cb_ = std::move(cb);
    } else {
        TG_LOG_INFO("Failed to enqueue stop generation request\n");
        cb(bhttp::status::internal_server_error,
           make_response_body("Failed to enqueue request"));
    }
}

void manager_impl::on_req_get_stats(req_body_type,
                                    resp_callback_type&& cb) noexcept
{
    TG_LOG_DEBUG("Got stats request\n");
    if (stats_cb_) {
        TG_LOG_DEBUG("Stats request already in progress\n");
        cb(bhttp::status::precondition_failed,
           make_response_body("Stats request already in progress"));
        return;
    }
    if (out_queue_->enqueue(req_stats_report{})) {
        TG_LOG_DEBUG("Enqueued stats request\n");
        stats_cb_ = std::move(cb);
    } else {
        TG_LOG_DEBUG("Failed to enqueue stsyd request\n");
        cb(bhttp::status::internal_server_error,
           make_response_body("Failed to enqueue request"));
    }
}

void manager_impl::on_inc_msg(mgmt::res_start_generation&& msg) noexcept
{
    TG_ENFORCE(start_cb_);
    if (msg.res) {
        TG_LOG_INFO("Successfully started generation\n");
        start_cb_(bhttp::status::ok, make_response_body("Generation started"));
    } else {
        TG_LOG_INFO("Failed to start generation: {}\n", msg.res.error());
        start_cb_(bhttp::status::precondition_failed,
                  make_response_body("Failed to start generation: {}\n",
                                     msg.res.error()));
    }
    start_cb_ = {};
}

void manager_impl::on_inc_msg(mgmt::res_stop_generation&& msg) noexcept
{
    TG_LOG_INFO("Successfully stopped generation\n");
    // This manual JSON generation is ugly and verbose but:
    // - there are only few messages where this is needed
    // - it's faster than putting the content into boost::json::value and
    // then serializing it to a string.
    std::string body;
    body.reserve(4096);
    body += R"({"result": {)";
    msg.res.summary.visit([&](std::string_view nam, auto val) {
        fmt::format_to(std::back_inserter(body), "\"{}\":{},", nam, val);
    });
    body.pop_back(); // Remove the last comma
    body += R"(}, "detailed": [)";
    for (const auto& ent : msg.res.detailed) {
        body += '{';
        fmt::format_to(std::back_inserter(body), "\"gen_idx\":{},",
                       ent.gen_idx);
        fmt::format_to(std::back_inserter(body), "\"flow_idx\":{},",
                       ent.flow_idx);
        fmt::format_to(std::back_inserter(body), "\"cnt_pkts\":{},",
                       ent.cnt_pkts);
        fmt::format_to(std::back_inserter(body), "\"cnt_bytes\":{},",
                       ent.cnt_bytes);
        fmt::format_to(std::back_inserter(body), "\"duration_usec\":{}",
                       ent.duration.to<stdcr::microseconds>().count());
        body += "},";
    }
    if (body.back() == ',') body.pop_back();
    body += R"(]})";

    TG_ENFORCE(stop_cb_);
    stop_cb_(bhttp::status::ok, std::move(body));
    stop_cb_ = {};
}

void manager_impl::on_inc_msg(mgmt::res_stats_report&& msg) noexcept
{
    TG_LOG_INFO("Successfully stopped generation\n");

    std::string body;
    body.reserve(1024);
    body += R"({"result": {)";
    msg.res.visit([&](std::string_view nam, auto val) {
        fmt::format_to(std::back_inserter(body), "\"{}\":{},", nam, val);
    });
    body.pop_back(); // Remove the last comma
    body += R"(}})";

    TG_ENFORCE(stats_cb_);
    stats_cb_(bhttp::status::ok, std::move(body));
    stats_cb_ = {};
}

void manager_impl::on_inc_msg(mgmt::generation_report&&) noexcept
{
    // TODO Write the generation report in CSV format:
    // - it can be written in the memory and dumped to a file when the
    // generation is stopped
    // - it can be written to a memory mapped file
    // - it can be written to a pipe and whoever is interested may listen on
    // the other side
    // - it can be written to a regular file with memory buffering and will have
    // some spikes here and there when the data is flushed to the disk.
}

template <typename... Args>
manager_impl::resp_body_type
manager_impl::make_response_body(fmt::format_string<Args...> fmtstr,
                                 Args&&... args) noexcept
{
    resp_body_type body;
    body.reserve(512);
    body += R"({"result": ")";
    fmt::format_to(std::back_inserter(body), fmtstr,
                   std::forward<Args>(args)...);
    body += R"("})";
    return body;
}

////////////////////////////////////////////////////////////////////////////////

manager::manager(const config& cfg) : impl_(std::make_unique<manager_impl>(cfg))
{
}

manager::~manager() noexcept = default;

void manager::process_events() noexcept
{
    impl_->process_events();
}

} // namespace mgmt
