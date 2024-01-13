#include "gen/priv/flows_generator.h"
#include "gen/priv/generation_ops.h"
#include "gen/priv/tcap_loader.h"

#include "put/pkt_utils.h"
#include "put/tg_assert.h"
#include "put/throw.h"

namespace gen::priv
{

template <typename T>
static bool inc_reset(T& val, T beg, T end) noexcept
{
    if (++val == end) {
        val = beg;
        return true;
    }
    return false;
}

static std::vector<flows_generator::pkt>
load_pkts(const flows_generator::config& cfg)
{
    std::vector<flows_generator::pkt> ret;
    // The gap between flows with the same index i.e. when the same flow is
    // restarted because its duration is shorter than the duration of the whole
    // test. This may come from the generation config, if/when needed.
    constexpr stdcr::milliseconds if_gap(100);
    /*
     * Load the packets and set the time-stamps as needed
     * Note that every time-stamp is relative to the time-stamp of the previous
     * packet. The time-stamp of the first packet is 0 and is relative to the
     * time-stamp of the particular flow the packet is part of at the moment.
     */
    const auto ipg = cfg.inter_pkts_gap;
    std::optional<stdcr::microseconds> ipg_tstamp;
    if (ipg) ipg_tstamp = stdcr::microseconds{0};
    std::optional<stdcr::microseconds> prev_tstamp; // For the relative time
    auto alloc_mbuf = [ops = cfg.gen_ops] { return ops->alloc_mbuf(); };
    for (gen::priv::tcap_loader tcap(cfg.cap_fpath); !tcap.is_eof();) {
        auto res = tcap.load_pkt(alloc_mbuf);
        if (!res) {
            put::throw_system_error(
                res.error(), "Failed to load packet from {}", cfg.cap_fpath);
        }
        auto& pk = res.value();
        if (ipg_tstamp) {
            pk.tstamp   = *ipg_tstamp;
            *ipg_tstamp = *ipg_tstamp + *ipg;
        }
        if (!prev_tstamp) prev_tstamp = pk.tstamp;
        using put::cycles;
        ret.push_back(flows_generator::pkt{
            .rel_tsc = cycles::from_duration(if_gap + pk.tstamp - *prev_tstamp),
            .mbuf    = flows_generator::mbuf_ptr_type(pk.mbuf),
            .from_cln = false, // Will be set later to a correct value
        });
    }
    /*
     * Verify that we can work with the packets and change the fields
     * that don't change during the generation
     * The assumption is that the first packet is always from client to server.
     * We need to make sure that the headers that we are going to change now or
     * later are in the first segment of the packet.
     */
    for (std::optional<rte_ether_addr> cln_ether_addr; auto& pkt : ret) {
        size_t offs = 0;
        auto* eh = put::read_hdr_advance<rte_ether_hdr>(pkt.mbuf.get(), offs);
        if (!eh) {
            put::throw_runtime_error(
                "Detected too short/fragmented packet from {}", cfg.cap_fpath);
        }
        if (const auto proto = ben::big_to_native(eh->ether_type);
            proto != RTE_ETHER_TYPE_IPV4) {
            put::throw_runtime_error(
                "Detected non IPv4 packet (proto: {}) from {}", proto,
                cfg.cap_fpath);
        }
        auto* ih = put::read_hdr_advance<rte_ipv4_hdr>(pkt.mbuf.get(), offs);
        if (!ih) {
            put::throw_runtime_error(
                "Detected too short/fragmented packet from {}", cfg.cap_fpath);
        }
        if (!cln_ether_addr) cln_ether_addr = eh->src_addr;
        pkt.from_cln =
            rte_is_same_ether_addr(&(*cln_ether_addr), &eh->src_addr);
        if (pkt.from_cln) {
            eh->src_addr = cfg.cln_ether_addr;
            eh->dst_addr = cfg.srv_ether_addr;
        } else {
            eh->src_addr = cfg.srv_ether_addr;
            eh->dst_addr = cfg.cln_ether_addr;
        }
        // No need need to check/load the next headers if we are not going to
        // change the port.
        if (!cfg.cln_port) continue;
        auto set_cport = [fc = pkt.from_cln, po = *cfg.cln_port](auto* hdr) {
            if (fc)
                hdr->src_port = ben::native_to_big(po);
            else
                hdr->dst_port = ben::native_to_big(po);
        };
        switch (ih->next_proto_id) {
        case IPPROTO_TCP: {
            auto* th = put::read_hdr<rte_tcp_hdr>(pkt.mbuf.get(), offs);
            if (!th) {
                put::throw_runtime_error(
                    "Detected too short/fragmented packet from {}",
                    cfg.cap_fpath);
            }
            set_cport(th);
            break;
        }
        case IPPROTO_UDP: {
            auto* uh = put::read_hdr<rte_udp_hdr>(pkt.mbuf.get(), offs);
            if (!uh) {
                put::throw_runtime_error(
                    "Detected too short/fragmented packet from {}",
                    cfg.cap_fpath);
            }
            set_cport(uh);
            break;
        }
        }
    }
    if (ret.empty()) {
        put::throw_runtime_error("Loaded no packets from {}", cfg.cap_fpath);
    }
    return ret;
}

static auto setup_flows(baio_ip_addr4_rng cln_ip_addrs,
                        baio_ip_addr4_rng srv_ip_addrs,
                        uint32_t flows_per_sec,
                        uint32_t burst_cnt,
                        flows_generator* fgen) noexcept
{
    TG_ENFORCE(burst_cnt >= 1);

    std::vector<flows_generator::flow> flows;
    flows.reserve(flows_per_sec);

    auto cln_ip_addr   = cln_ip_addrs.begin();
    auto srv_ip_addr   = srv_ip_addrs.begin();
    uint32_t burst_idx = 0;

    for (auto i : boost::irange(flows_per_sec)) {
        flows.push_back(flows_generator::flow{
            .idx         = i,
            .pkt_idx     = 0, // always start from the 1st packet
            .cln_ip_addr = *cln_ip_addr,
            .srv_ip_addr = *srv_ip_addr,
            .event       = {}, // We'll be set later
            .fgen        = fgen,
            .cnt_pkts    = {},
            .cnt_bytes   = {},
            .tstamp_beg  = {},
            .tstamp_end  = {},
        });
        // All streams from a given burst are with the same client and server
        // addresses. The addresses change for the next burst.
        if (inc_reset(burst_idx, 0u, burst_cnt)) {
            inc_reset(cln_ip_addr, cln_ip_addrs.begin(), cln_ip_addrs.end());
            inc_reset(srv_ip_addr, srv_ip_addrs.begin(), srv_ip_addrs.end());
        }
    }

    return std::tuple(std::move(flows), cln_ip_addr, srv_ip_addr, burst_idx);
}

////////////////////////////////////////////////////////////////////////////////
// Note that we pass the `this`pointer to the `setup_flows` before the generator
// is actually fully constructed. It's OK because we don't actually use the
// pointer only store it in the flows.
flows_generator::flows_generator(const config& cfg)
: pkts_(load_pkts(cfg))
, gen_ops_(cfg.gen_ops)
, cln_ip_addrs_(cfg.cln_ip_addrs.hosts())
, srv_ip_addrs_(cfg.srv_ip_addrs.hosts())
, idx_(cfg.idx)
, cln_ip_addr_(cln_ip_addrs_.begin())
, srv_ip_addr_(srv_ip_addrs_.begin())
, burst_idx_(0)
, burst_cnt_(cfg.burst)
{
    std::tie(flows_, cln_ip_addr_, srv_ip_addr_, burst_idx_) = setup_flows(
        cln_ip_addrs_, srv_ip_addrs_, cfg.flows_per_sec, burst_cnt_, this);
    // At this point the `flows_` vector is filled and it won't be reallocated
    // from this point on. Thus it's safe to setup the flow events because the
    // event callbacks will keep a pointer to the corresponding flow. This
    // means that it's a MUST that the flow entries don't move in the memory.
    setup_flow_events();
}

flows_generator::~flows_generator() noexcept                 = default;
flows_generator::flows_generator(flows_generator&&) noexcept = default;
flows_generator&
flows_generator::operator=(flows_generator&&) noexcept = default;

void flows_generator::setup_flow_events()
{
    // The flows need to be evenly spread through out the second
    using namespace std::chrono_literals;
    const auto flow_tsc_step = put::cycles::from_duration(
        stdcr::microseconds{1'000'000us / flows_.size()});
    if (flow_tsc_step == put::cycles{0}) {
        put::throw_runtime_error(
            "Can't work with so many ({}) flows per second", flows_.size());
    }

    // TODO: Optimization
    // For the case of working with predefined inter packet gaps we can
    // schedule periodic event only once.
    for (put::cycles flow_tsc{0}; auto& flow : flows_) {
        const auto evtm = flow_tsc + pkts_[flow.pkt_idx].rel_tsc;
        flow.event      = gen_ops_->create_scheduler_event();
        flow.event.schedule_single(evtm, on_event, &flow);
        flow_tsc += flow_tsc_step;
    }
}

void flows_generator::on_flow_event(flow& fl) noexcept
{
    if (++fl.pkt_idx == pkts_.size()) {
        // Upon restarting the stream we need to change it's client and server
        // addresses according to the burst counter logic.
        fl.pkt_idx     = 0;
        fl.cln_ip_addr = *cln_ip_addr_;
        fl.srv_ip_addr = *srv_ip_addr_;
        if (inc_reset(burst_idx_, 0u, burst_cnt_)) {
            inc_reset(cln_ip_addr_, cln_ip_addrs_.begin(), cln_ip_addrs_.end());
            inc_reset(srv_ip_addr_, srv_ip_addrs_.begin(), srv_ip_addrs_.end());
        }
    }
    const auto& pkt                 = pkts_[fl.pkt_idx];
    const auto [src_addr, dst_addr] = [&] {
        return pkt.from_cln
                   ? std::pair(ben::native_to_big(fl.cln_ip_addr.to_uint()),
                               ben::native_to_big(fl.srv_ip_addr.to_uint()))
                   : std::pair(ben::native_to_big(fl.srv_ip_addr.to_uint()),
                               ben::native_to_big(fl.cln_ip_addr.to_uint()));
    }();
    const auto tstamp = put::cycles::current();
    fl.cnt_pkts += 1;
    fl.cnt_bytes += pkt.mbuf->pkt_len;
    fl.tstamp_end = tstamp;
    // Note that the first packet marks the beginning of the flow.
    // If a flow contains only single packet and burst is equal to 1 then the
    // duration of this flow will be report as 0 which is a bit weird but
    // it shouldn't happen in practice ... or we can change the logic here.
    if (fl.cnt_pkts == 1) fl.tstamp_beg = tstamp;
    generation_report report = {
        .tstamp   = tstamp,
        .gen_idx  = idx_,
        .flow_idx = fl.idx,
        .pkt_idx  = fl.pkt_idx,
        .pkt_len  = pkt.mbuf->pkt_len,
        .src_addr = in_addr{src_addr},
        .dst_addr = in_addr{dst_addr},
        .from_cln = pkt.from_cln,
        .ok       = true,
    };
    // TODO: Optimization
    // The copy of the whole packet here is really unfortunate.
    // It's needed because we are going to change the client and server
    // addresses in the IP header. Other flows may do the same while the packet
    // is waiting in the queues to be transmitted and before the NIC actually
    // do the transmission. Thus every single flow need to work on its own copy
    // of the packet.
    // A possible optimization here is to segment the packets upon loading so
    // that the packet headers are in the first and smaller segment while the
    // payload is in a separate packet and thus only the first segment will need
    // to be copied here.
    rte_mbuf* mbuf = gen_ops_->copy_pkt(pkt.mbuf.get());
    if (!mbuf) {
        report.ok = false;
        gen_ops_->do_report(report);
        return;
    }
    // All packets have been checked upon loading and thus we may jump right
    // to the IP header.
    auto* ih     = put::read_hdr<rte_ipv4_hdr>(mbuf, RTE_ETHER_HDR_LEN);
    ih->src_addr = src_addr;
    ih->dst_addr = dst_addr;
    // The hardware needs to (re)calculate the checksums of the packet.
    // For this we need to set the appropriate flags,
    constexpr auto flags = RTE_MBUF_F_TX_IPV4 | RTE_MBUF_F_TX_IP_CKSUM |
                           RTE_MBUF_F_TX_TCP_CKSUM | RTE_MBUF_F_TX_UDP_CKSUM;
    mbuf->ol_flags |= flags;
    mbuf->l2_len = RTE_ETHER_HDR_LEN;
    mbuf->l3_len = put::hdr_len(ih);

    gen_ops_->send_pkt(mbuf);

    gen_ops_->do_report(report);

    fl.event.schedule_single(pkt.rel_tsc, on_event, &fl);
}

void flows_generator::on_event(rte_timer*, void* ctx) noexcept
{
    auto fl = static_cast<flow*>(ctx);
    fl->fgen->on_flow_event(*fl);
}

} // namespace gen::priv
