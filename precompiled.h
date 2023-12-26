#pragma once

////////////////////////////////////////////////////////////////////////////////
// C standard library and system headers
#include <assert.h>
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h> // size_t
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/resource.h>

////////////////////////////////////////////////////////////////////////////////
// DPDK headers
#include <rte_errno.h>
#include <rte_ether.h>
#include <rte_ring.h>
#include <rte_launch.h>

////////////////////////////////////////////////////////////////////////////////
// c++ standard library headers
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <exception>
#include <filesystem>
#include <functional>
#include <optional>
#include <memory>
#include <new> // launder
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include <experimental/scope>

////////////////////////////////////////////////////////////////////////////////
// boost headers
#include <boost/algorithm/string/split.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/json/parser.hpp>
#include <boost/json/value.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/outcome/result.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

////////////////////////////////////////////////////////////////////////////////
// fmt headers
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

////////////////////////////////////////////////////////////////////////////////
// this project headers
#include "aliases.h"
