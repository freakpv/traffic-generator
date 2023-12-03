#pragma once

namespace put // project utilities
{

std::optional<rte_ether_addr> parse_ether_addr(std::string_view) noexcept;

} // namespace put
////////////////////////////////////////////////////////////////////////////////
template <>
struct fmt::formatter<rte_ether_addr>
{
    constexpr auto parse(auto& ctx) const noexcept { return ctx.begin(); }
    fmt::format_context::iterator
    format(const rte_ether_addr& arg, fmt::format_context& ctx) const noexcept;
};
