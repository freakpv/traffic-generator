#pragma once

namespace stdcr = std::chrono;
namespace stdex = std::experimental;
namespace stdfs = std::filesystem;
namespace stdrg = std::ranges;

namespace baio  = boost::asio;
namespace balgo = boost::algorithm;
namespace beast = boost::beast;
namespace bcont = boost::container;
namespace bhttp = boost::beast::http;
namespace bjson = boost::json;
namespace bout  = boost::outcome_v2;
namespace bpo   = boost::program_options;
namespace bsys  = boost::system;
namespace bx3   = boost::spirit::x3;

using baio_context      = boost::asio::io_context;
using baio_ip_addr      = boost::asio::ip::address;
using baio_ip_addr4     = boost::asio::ip::address_v4;
using baio_ip_addr6     = boost::asio::ip::address_v6;
using baio_ip_net4      = boost::asio::ip::network_v4;
using baio_tcp_acceptor = boost::asio::ip::tcp::acceptor;
using baio_tcp_endpoint = boost::asio::ip::tcp::endpoint;
using baio_tcp_socket   = boost::asio::ip::tcp::socket;

////////////////////////////////////////////////////////////////////////////////

template <>
struct fmt::formatter<baio_tcp_endpoint> : fmt::ostream_formatter
{
};

template <>
struct fmt::formatter<bsys::error_code>
{
    constexpr auto parse(auto& ctx) noexcept { return ctx.begin(); }
    auto format(const bsys::error_code& arg, auto& ctx) const noexcept
    {
        return stdrg::copy(arg.message(), ctx.out()).out;
    }
};
