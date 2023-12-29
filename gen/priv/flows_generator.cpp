#include "gen/priv/flows_generator.h"
#include "gen/priv/tcap_loader.h"

// TODO: Remove this dependency by using own config
#include "mgmt/gen_config.h"

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

flows_generator::flows_generator(const mgmt::flows_config& cfg,
                                 const stdfs::path& pcaps_dir,
                                 const rte_ether_addr&,
                                 rte_mempool* pool,
                                 event_scheduler*)
{
    auto alloc_mbuf = [pool] { return rte_pktmbuf_alloc(pool); };

    decltype(pkts_) pkts;
    const auto fpath = pcaps_dir / cfg.name;
    std::optional<stdcr::microseconds> first_tstamp; // For the relative time
    for (gen::priv::tcap_loader tcap(fpath); !tcap.is_eof();) {
        const auto res = tcap.load_pkt(alloc_mbuf);
        if (!res) {
            put::throw_system_error(res.error(),
                                    "Failed to load packet from {}", fpath);
        }
        const auto& pk = res.value();
        if (!first_tstamp) first_tstamp = pk.tstamp;
        pkts.push_back(pkt{
            .rel_tsc = put::duration_to_tsc(pk.tstamp - *first_tstamp),
            .mbuf    = pk.mbuf,
        });
    }

    // TODO:
    // - change the client port in the packets

    pkts_ = std::move(pkts);
}

flows_generator::~flows_generator() noexcept
{
    for (auto pk : pkts_) rte_pktmbuf_free(pk.mbuf);
    pkts_.clear();

    // TODO:
}

} // namespace gen::priv
