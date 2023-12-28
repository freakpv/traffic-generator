#pragma once

namespace gen::priv
{

#define TG_GEN_ERRORS(MACRO)                  \
    MACRO(success, "Success")                 \
    MACRO(already_started, "Already started") \
    MACRO(already_stopped, "Already stopped")

struct error
{
    enum code
    {
#define XXX(var, str) var,
        TG_GEN_ERRORS(XXX)
#undef XXX
    };
};

const bsys::error_category& error_category() noexcept;

[[nodiscard]] inline bsys::error_code make_error_code(error::code ec) noexcept
{
    return {static_cast<int>(ec), error_category()};
}

} // namespace gen::priv
////////////////////////////////////////////////////////////////////////////////
namespace boost::system
{
template <>
struct is_error_code_enum<gen::priv::error::code>
{
    static const bool value = true;
};
} // namespace boost::system
