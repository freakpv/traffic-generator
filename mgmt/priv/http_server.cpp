#include "mgmt/priv/http_server.h"
#include "put/throw.h"

namespace mgmt::priv
{

static baio_tcp_acceptor make_acceptor(baio_context& ctx,
                                       const baio_tcp_endpoint& endpoint)
{
    try {
        baio_tcp_acceptor acceptor(ctx);
        acceptor.open(endpoint.protocol());
        acceptor.set_option(baio::socket_base::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen(baio::socket_base::max_listen_connections);
        return acceptor;
    } catch (const bsys::system_error& err) {
        put::throw_system_error(err.code(), "Failed to start HTTP server at {}",
                                endpoint);
    }
}

http_server::http_server(baio_context& ctx, const baio_tcp_endpoint& endpoint)
: acceptor_(make_acceptor(ctx, endpoint))
{
}

http_server::~http_server() noexcept = default;

void http_server::start_running(user_handler_type&& usr_hnd,
                                error_handler_type&& err_hnd) noexcept
{
    usr_handler_ = std::move(usr_hnd);
    err_handler_ = std::move(err_hnd);
    baio::co_spawn(acceptor_.get_executor(), accept_connections(acceptor_),
                   baio::detached);
}

baio::awaitable<void>
http_server::accept_connections(baio_tcp_acceptor& acceptor) noexcept
{
    for (;;) {
        auto [err, sock] =
            co_await acceptor.async_accept(baio::as_tuple(baio::use_awaitable));
        if (!err) {
            auto executor = acceptor.get_executor();
            baio::co_spawn(executor, process_request(std::move(sock)),
                           baio::detached);
        } else {
            err_handler_("accept failed", err);
        }
    }
}

// The socket is taken by value because this is a coroutine function
baio::awaitable<void>
http_server::process_request(baio_tcp_socket sock) noexcept
{
    using msg_body_type = bhttp::string_body;
    using request_type  = bhttp::request<msg_body_type>;
    using response_type = bhttp::response<msg_body_type>;
    static_assert(std::is_same_v<msg_body_type::value_type, resp_body_type>);

    request_type req;
    beast::flat_static_buffer<1024> buffer;
    auto [rerr, rbytes] = co_await bhttp::async_read(
        sock, buffer, req, baio::as_tuple(baio::use_awaitable));
    (void)rbytes;
    if (rerr) {
        err_handler_("receive failed", rerr);
        co_return;
    }

    auto [code, body] = co_await get_response(req.target(), req.body());

    response_type res;
    res.result(code);
    res.version(req.version());
    res.set(bhttp::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(bhttp::field::content_type, "text/json");
    res.body() = std::move(body);
    res.prepare_payload();

    auto [werr, wbytes] = co_await bhttp::async_write(
        sock, res, baio::as_tuple(baio::use_awaitable));
    (void)wbytes;
    if (werr) err_handler_("send failed", werr);
}

// The target and the body types are just views because their content lives
// in the above coroutine until the user handler "returns" the response.
http_server::response_awaitable_type
http_server::get_response(target_type target, req_body_type body) noexcept
{
    using handler_type = decltype(baio::use_awaitable);
    return baio::async_initiate<handler_type,
                                void(bhttp::status, resp_body_type&&)>(
        [target, body, this](auto&& callme) {
            usr_handler_(target, body, std::move(callme));
        },
        baio::use_awaitable);
}

} // namespace mgmt::priv
