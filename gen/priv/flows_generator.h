#pragma once

namespace mgmt
{
class flows_config;
} // namespace mgmt
////////////////////////////////////////////////////////////////////////////////
namespace gen::priv
{
class event_scheduler;

class flows_generator
{
    struct pkt
    {
        uint64_t rel_tsc; // relative timestamp in TSC cycles
        rte_mbuf* mbuf;
    };
    std::vector<pkt> pkts_;
    // TODO:

public:
    struct config
    {
        stdfs::path cap_fpath;
        rte_ether_addr src_addr;
        rte_ether_addr dst_addr;
        uint32_t burst;
        uint32_t flows_per_sec;
        std::optional<stdcr::microseconds> inter_pkts_gap;
        baio_ip_net4 cln_ips;
        baio_ip_net4 srv_ips;
        std::optional<uint16_t> cln_port;
        rte_mempool* mbufs_pool;
        event_scheduler* scheduler;
    };

public:
    explicit flows_generator(const config&);
    ~flows_generator() noexcept;

    flows_generator()                                  = delete;
    flows_generator(flows_generator&&)                 = delete;
    flows_generator(const flows_generator&)            = delete;
    flows_generator& operator=(flows_generator&&)      = delete;
    flows_generator& operator=(const flows_generator&) = delete;
};

} // namespace gen::priv
