#include "mgmt/manager.h"
#include "mgmt/priv/http_server.h"
#include "log/log.h"

namespace mgmt // management
{

class manager_impl
{
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
    req_handlers_type req_handlers_;

public:
    explicit manager_impl(const baio_tcp_endpoint& endpoint);

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
};

////////////////////////////////////////////////////////////////////////////////

manager_impl::manager_impl(const baio_tcp_endpoint& endpoint)
: http_server_(io_ctx_, endpoint)
{
    TGLOG_INFO("Started management server at {}\n", endpoint);

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
    io_ctx_.poll();
    // TODO:
}

void manager_impl::on_http_request(target_type target,
                                   req_body_type req_body,
                                   resp_callback_type&& cb) noexcept
{
    if (auto it = req_handlers_.find(target); it != req_handlers_.end()) {
        std::invoke(it->second, this, req_body, std::move(cb));
    } else {
        cb(bhttp::status::bad_request,
           fmt::format("Invalid target: {}", target));
    }
}

void manager_impl::on_http_server_error(error_info_type info,
                                        bsys::error_code ec) noexcept
{
    TGLOG_ERROR("Management server error: {}. {}\n", info, ec);
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

void manager_impl::on_req_start_gen(req_body_type,
                                    resp_callback_type&&) noexcept
{
    // TODO
}

void manager_impl::on_req_stop_gen(req_body_type, resp_callback_type&&) noexcept
{
    // TODO
}

void manager_impl::on_req_get_stats(req_body_type,
                                    resp_callback_type&&) noexcept
{
    // TODO
}

////////////////////////////////////////////////////////////////////////////////

manager::manager(const baio_tcp_endpoint& endpoint)
: impl_(std::make_unique<manager_impl>(endpoint))
{
}

manager::~manager() noexcept = default;

void manager::process_events() noexcept
{
    impl_->process_events();
}

} // namespace mgmt
