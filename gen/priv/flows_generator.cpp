#include "gen/priv/flows_generator.h"
#include "gen/priv/generation_ops.h"
#include "gen/priv/tcap_loader.h"

#include "put/pkt_utils.h"
#include "put/throw.h"
#include "put/time_utils.h"

namespace gen::priv
{
/* TODO
    if (offl.ip_csum)
        pkt->ol_flags |= (RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_IPV4);
    if (offl.tcp_csum) pkt->ol_flags |= RTE_MBUF_F_TX_TCP_CKSUM;
    if (offl.udp_csum) pkt->ol_flags |= RTE_MBUF_F_TX_UDP_CKSUM;
    if (offl.ip_csum || offl.tcp_csum || offl.udp_csum) {
        pkt->l2_len = RTE_ETHER_HDR_LEN;
        pkt->l3_len = pkt_info.iph_len_;
    }
*/

static std::vector<flows_generator::pkt>
load_pkts(const flows_generator::config& cfg)
{
    std::vector<flows_generator::pkt> ret;
    /*
     * Load the packets and set the time-stamps as needed
     */
    const auto ipg = cfg.inter_pkts_gap;
    std::optional<stdcr::microseconds> ipg_tstamp;
    if (ipg) ipg_tstamp = stdcr::microseconds{0};
    std::optional<stdcr::microseconds> first_tstamp; // For the relative time
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
        if (!first_tstamp) first_tstamp = pk.tstamp;
        ret.push_back(flows_generator::pkt{
            .rel_tsc  = put::duration_to_tsc(pk.tstamp - *first_tstamp),
            .mbuf     = flows_generator::mbuf_ptr_type(pk.mbuf),
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

static std::vector<flows_generator::flow>
setup_flows(const flows_generator::config&, flows_generator*)
{
    // TODO:
    return {};
}

////////////////////////////////////////////////////////////////////////////////
// Note that we pass the `this`pointer to the `setup_flows` before the generator
// is actually fully constructed. It's OK because we don't actually use the
// pointer only store it in the flows.
flows_generator::flows_generator(const config& cfg)
: pkts_(load_pkts(cfg))
, flows_(setup_flows(cfg, this))
, gen_ops_(cfg.gen_ops)
, cln_ip_addrs_(cfg.cln_ip_addrs.hosts())
, srv_ip_addrs_(cfg.srv_ip_addrs.hosts())
, idx_(cfg.idx)
, cln_ip_addr_(*cln_ip_addrs_.begin())
, srv_ip_addr_(*srv_ip_addrs_.begin())
, burst_idx_(0)
, burst_cnt_(cfg.burst)
{
}

flows_generator::~flows_generator() noexcept                 = default;
flows_generator::flows_generator(flows_generator&&) noexcept = default;
flows_generator&
flows_generator::operator=(flows_generator&&) noexcept = default;

} // namespace gen::priv
