#pragma once

#include "gen/priv/event_handle.h"
#include "put/time_utils.h"

namespace gen::priv
{
class generation_ops;

class flows_generator
{
public:
    struct mbuf_free
    {
        void operator()(rte_mbuf* p) const noexcept { rte_pktmbuf_free(p); }
    };
    using mbuf_ptr_type = std::unique_ptr<rte_mbuf, mbuf_free>;
    struct pkt
    {
        put::cycles rel_tsc; // relative timestamp
        mbuf_ptr_type mbuf;
        bool from_cln; // true - client to server, false - server to client
    };
    // The scheduler event notifications are fired for given flow instance and
    // upon receiving such notification we need to do some things in the
    // flows generator intself and thus we need to keep a pointer to it in every
    // flow entry.
    struct flow
    {
        // Member variables needed for the generation
        uint32_t idx;
        uint32_t pkt_idx;
        baio_ip_addr4 cln_ip_addr;
        baio_ip_addr4 srv_ip_addr;
        event_handle event;
        flows_generator* fgen;

        // Member variables needed for the stats
        uint64_t cnt_pkts;
        uint64_t cnt_bytes;
        put::cycles tstamp_beg;
        put::cycles tstamp_end;
    };

private:
    // Most of the members are never changed once set upon construction.
    // However, they are not marked as const due to the requirements of the move
    // operations, although the latter are never used actually.
    // I could have workaround this but the code will get messier.
    // So, I decided to stick to comments only.

    // The packets vector and its content is never changed once created.
    std::vector<pkt> pkts_;
    // The flows vector is never changed once created.
    // However, the member of each flow are modified to track the flow state
    std::vector<flow> flows_;

    // These members are never changed once set upon construction
    gen::priv::generation_ops* gen_ops_;
    baio_ip_addr4_rng cln_ip_addrs_;
    baio_ip_addr4_rng srv_ip_addrs_;
    uint32_t idx_;

    // These members are used to handle the burst functionality and they
    // are changed during runtime, except the burst count.
    baio_ip_addr4_rng::iterator cln_ip_addr_;
    baio_ip_addr4_rng::iterator srv_ip_addr_;
    uint32_t burst_idx_;
    uint32_t burst_cnt_;

public:
    struct config
    {
        uint32_t idx;
        stdfs::path cap_fpath;
        rte_ether_addr cln_ether_addr;
        rte_ether_addr srv_ether_addr;
        uint32_t burst;
        uint32_t flows_per_sec;
        std::optional<stdcr::microseconds> inter_pkts_gap;
        baio_ip_net4 cln_ip_addrs;
        baio_ip_net4 srv_ip_addrs;
        std::optional<uint16_t> cln_port;
        gen::priv::generation_ops* gen_ops;
    };

public:
    explicit flows_generator(const config&);
    ~flows_generator() noexcept;

    flows_generator(flows_generator&&) noexcept;
    flows_generator& operator=(flows_generator&&) noexcept;

    flows_generator()                                  = delete;
    flows_generator(const flows_generator&)            = delete;
    flows_generator& operator=(const flows_generator&) = delete;

    std::span<const flow> flows() const noexcept { return flows_; }
    uint32_t idx() const noexcept { return idx_; }

private:
    void setup_flow_events();
    void on_flow_event(flow&) noexcept;
    static void on_event(rte_timer*, void*) noexcept;
};

} // namespace gen::priv
