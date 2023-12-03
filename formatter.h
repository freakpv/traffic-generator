#pragma once

template <typename T>
concept tgn_formattable =
    requires(fmt::format_context::iterator it, const T& v) {
        {
            tgn_format_to(it, v)
        } -> std::same_as<fmt::format_context::iterator>;
    };

template <tgn_formattable T>
struct fmt::formatter<T>
{
    constexpr fmt::format_parse_context::iterator
    parse(fmt::format_parse_context& ctx) noexcept
    {
        return ctx.begin();
    }
    fmt::format_context::iterator
    format(const T& v, fmt::format_context& ctx) const noexcept
    {
        return tgn_format_to(ctx.out(), v);
    }
};
