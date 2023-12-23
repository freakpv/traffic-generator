#pragma once

namespace stdex = std::experimental;
namespace stdfs = std::filesystem;

namespace baio  = boost::asio;
namespace balgo = boost::algorithm;
namespace beast = boost::beast;
namespace bhttp = boost::beast::http;
namespace bout  = boost::outcome_v2;
namespace bpo   = boost::program_options;
namespace bsys  = boost::system;
namespace bx3   = boost::spirit::x3;

using baio_context      = boost::asio::io_context;
using baio_ip_addr      = boost::asio::ip::address;
using baio_ip_addr4     = boost::asio::ip::address_v4;
using baio_ip_addr6     = boost::asio::ip::address_v6;
using baio_tcp_acceptor = boost::asio::ip::tcp::acceptor;
using baio_tcp_endpoint = boost::asio::ip::tcp::endpoint;
using baio_tcp_socket   = boost::asio::ip::tcp::socket;
