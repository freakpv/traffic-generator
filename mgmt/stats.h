#pragma once

namespace mgmt
{

struct summary_stats
{
#define TG_COUNTERS(MACRO)              \
    MACRO(uint64_t, cnt_rx_pkts)        \
    MACRO(uint64_t, cnt_tx_pkts)        \
    MACRO(uint64_t, cnt_rx_bytes)       \
    MACRO(uint64_t, cnt_tx_bytes)       \
    MACRO(uint64_t, cnt_rx_pkts_qfull)  \
    MACRO(uint64_t, cnt_rx_pkts_nombuf) \
    MACRO(uint64_t, cnt_tx_pkts_qfull)  \
    MACRO(uint64_t, cnt_tx_pkts_nombuf) \
    MACRO(uint64_t, cnt_rx_pkts_err)    \
    MACRO(uint64_t, cnt_tx_pkts_err)

#define XXX(type, name) type name = 0;
    TG_COUNTERS(XXX)
#undef XXX

    template <typename Visitor>
    void visit(Visitor&& vis) const noexcept
    {
#define XXX(type, name) vis(#name, name);
        TG_COUNTERS(XXX)
#undef XXX
    }

#undef TG_COUNTERS
};

} // namespace mgmt
