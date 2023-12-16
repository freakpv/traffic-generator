#pragma once

namespace stdex = std::experimental;
namespace stdfs = std::filesystem;

namespace bout = boost::outcome_v2;
namespace bpo  = boost::program_options;
namespace bsys = boost::system;
namespace bx3  = boost::spirit::x3;

////////////////////////////////////////////////////////////////////////////////
// Some common types and functions used throughout the project
namespace tglib
{

inline bsys::error_code system_error_code(int ec) noexcept
{
    return {ec, bsys::system_category()};
}

} // namespace tglib
