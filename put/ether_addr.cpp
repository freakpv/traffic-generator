#include "put/ether_addr.h"

namespace put // project utilities
{

std::optional<rte_ether_addr> parse_ether_addr(std::string_view str) noexcept
{
    int i = 0;
    rte_ether_addr ret;

    static_assert(sizeof(rte_ether_addr::addr_bytes) == 6);
    auto fill = [&ret, &i](const auto& ctx) mutable {
        const auto val      = bx3::_attr(ctx);
        bx3::_pass(ctx)     = (val <= UINT8_MAX);
        ret.addr_bytes[i++] = val;
    };
    auto parser = bx3::hex[fill] >> ':' >> bx3::hex[fill] >> ':' >>
                  bx3::hex[fill] >> ':' >> bx3::hex[fill] >> ':' >>
                  bx3::hex[fill] >> ':' >> bx3::hex[fill] >> bx3::eoi;

    auto beg       = str.begin();
    const auto end = str.end();
    if (bx3::parse(beg, end, parser)) return ret;
    return std::nullopt;
}

} // namespace put
////////////////////////////////////////////////////////////////////////////////

fmt::format_context::iterator
fmt::formatter<rte_ether_addr>::format(const rte_ether_addr& arg,
                                       fmt::format_context& ctx) const noexcept
{
    return fmt::format_to(
        ctx.out(), "{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
        arg.addr_bytes[0], arg.addr_bytes[1], arg.addr_bytes[2],
        arg.addr_bytes[3], arg.addr_bytes[4], arg.addr_bytes[5]);
}
