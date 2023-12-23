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
#include <string.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/resource.h>

////////////////////////////////////////////////////////////////////////////////
// DPDK headers
#include <rte_errno.h>
#include <rte_ether.h>
#include <rte_launch.h>

////////////////////////////////////////////////////////////////////////////////
// c++ standard library headers
#include <array>
#include <atomic>
#include <charconv>
#include <exception>
#include <filesystem>
#include <optional>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>

#include <experimental/scope>

////////////////////////////////////////////////////////////////////////////////
// boost headers
#include <boost/algorithm/string/split.hpp>
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
