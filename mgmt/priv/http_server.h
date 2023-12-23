#pragma once

namespace mgmt::priv
{

class http_server
{
public:
    using error_info_type = std::string_view;
    using target_type     = std::string_view;
    using req_body_type   = std::string_view;
    using resp_body_type  = std::string;
    using callback_type = std::function<void(bhttp::status, resp_body_type&&)>;
    using user_handler_type =
        std::function<void(target_type, req_body_type, callback_type&&)>;
    using error_handler_type =
        std::function<void(error_info_type, bsys::error_code)>;

private:
    baio_tcp_acceptor acceptor_;
    user_handler_type usr_handler_;
    error_handler_type err_handler_;

public:
    http_server(baio_context&, const baio_tcp_endpoint&);
    ~http_server() noexcept;

    http_server(http_server&&)                 = delete;
    http_server(const http_server&)            = delete;
    http_server& operator=(http_server&&)      = delete;
    http_server& operator=(const http_server&) = delete;

    void start_running(user_handler_type&&, error_handler_type&&) noexcept;

private:
    using response_awaitable_type =
        baio::awaitable<std::tuple<bhttp::status, resp_body_type>>;

    baio::awaitable<void> accept_connections(baio_tcp_acceptor&) noexcept;
    baio::awaitable<void> process_request(baio_tcp_socket) noexcept;
    response_awaitable_type get_response(target_type, req_body_type) noexcept;
};

} // namespace mgmt::priv
