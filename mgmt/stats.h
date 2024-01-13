#pragma once

#include "put/time_utils.h"

namespace mgmt
{

// TODO: Further development
// These stats could be extended with stats per generator so that we can draw
// not only real-time summary graphs but also real-time graphs per generator
struct stats
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

// Stats send at the end when the generation is stopped.
struct summary_stats
{
    stats summary;

    struct entry
    {
        uint32_t gen_idx;
        uint32_t flow_idx;
        uint64_t cnt_pkts;
        uint64_t cnt_bytes;
        put::cycles duration;
    };
    std::vector<entry> detailed;
};

// Report used for producing a CSV report with per generator/flow/packet
// granularity. It can be used for drawing additional graphs.
struct generation_report
{
    put::cycles tstamp;
    uint32_t gen_idx;
    uint32_t flow_idx;
    uint32_t pkt_idx;
    uint32_t pkt_len;
    in_addr src_addr;
    in_addr dst_addr;
    uint32_t from_cln : 1; // true - from client, false - from server
    uint32_t ok       : 1; // true - generated, false - generation missed
};

} // namespace mgmt
