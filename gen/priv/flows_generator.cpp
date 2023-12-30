#include "gen/priv/flows_generator.h"
#include "gen/priv/generation_ops.h"
#include "gen/priv/tcap_loader.h"

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

flows_generator::flows_generator(const config& cfg)
{
    auto alloc_mbuf = [ops = cfg.gen_ops] { return ops->alloc_mbuf(); };

    decltype(pkts_) pkts;
    std::optional<stdcr::microseconds> first_tstamp; // For the relative time
    for (gen::priv::tcap_loader tcap(cfg.cap_fpath); !tcap.is_eof();) {
        const auto res = tcap.load_pkt(alloc_mbuf);
        if (!res) {
            put::throw_system_error(
                res.error(), "Failed to load packet from {}", cfg.cap_fpath);
        }
        const auto& pk = res.value();
        if (!first_tstamp) first_tstamp = pk.tstamp;
        pkts.push_back(pkt{
            .rel_tsc = put::duration_to_tsc(pk.tstamp - *first_tstamp),
            .mbuf    = mbuf_ptr_type(pk.mbuf),
        });
    }

    // TODO:
    // - change the client port in the packets

    pkts_ = std::move(pkts);
}

flows_generator::~flows_generator() noexcept                 = default;
flows_generator::flows_generator(flows_generator&&) noexcept = default;
flows_generator&
flows_generator::operator=(flows_generator&&) noexcept = default;

} // namespace gen::priv
