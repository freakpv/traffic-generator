#pragma once

#include "put/mem_utils.h"

namespace put
{

inline size_t hdr_len(const rte_ether_hdr*) noexcept
{
    return sizeof(rte_ether_hdr);
}

inline size_t hdr_len(const rte_ipv4_hdr* hdr) noexcept
{
    return ((hdr->version_ihl & 0x0F) * 4u);
}

inline size_t hdr_len(const rte_ipv6_hdr*) noexcept
{
    return sizeof(rte_ipv6_hdr);
}

// Includes the length of the TCP options, if present
inline size_t hdr_len(const rte_tcp_hdr* hdr) noexcept
{
    return ((hdr->data_off & 0xf0u) >> 2u);
}

inline size_t hdr_len(const rte_udp_hdr*) noexcept
{
    return sizeof(rte_udp_hdr);
}

////////////////////////////////////////////////////////////////////////////////
// The functions intentionally read the headers only if they are fully present
// in the first segment of the given packet.
// The DPDK `rte_pktmbuf_read` can read segmented data, if needed, but it just
// returns const void pointer and representing this as any type is UB according
// to the C++ standard. In addition we need to be able to modify fields from the
// headers and these changes need to be reflected in the corresponding packet.
template <typename Hdr>
inline const Hdr* read_hdr(const rte_mbuf* pkt, size_t off) noexcept
{
    if ((off + sizeof(Hdr)) <= rte_pktmbuf_data_len(pkt)) {
        return put::start_lifetime_as<Hdr>(
            rte_pktmbuf_mtod_offset(pkt, const char*, off));
    }
    return nullptr;
}

template <typename Hdr>
inline Hdr* read_hdr(rte_mbuf* pkt, size_t off) noexcept
{
    if ((off + sizeof(Hdr)) <= rte_pktmbuf_data_len(pkt)) {
        return put::start_lifetime_as<Hdr>(
            rte_pktmbuf_mtod_offset(pkt, char*, off));
    }
    return nullptr;
}

template <typename Hdr>
inline const Hdr* read_hdr_advance(const rte_mbuf* pkt, size_t& off) noexcept
{
    auto* hdr = read_hdr<Hdr>(pkt, off);
    if (hdr) off += hdr_len(hdr);
    return hdr;
}

template <typename Hdr>
inline Hdr* read_hdr_advance(rte_mbuf* pkt, size_t& off) noexcept
{
    auto* hdr = read_hdr<Hdr>(pkt, off);
    if (hdr) off += hdr_len(hdr);
    return hdr;
}

} // namespace put
