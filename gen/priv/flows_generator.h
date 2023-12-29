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
    flows_generator(const mgmt::flows_config&,
                    const stdfs::path&,
                    const rte_ether_addr&,
                    rte_mempool*,
                    event_scheduler*);
    ~flows_generator() noexcept;

    flows_generator()                                  = delete;
    flows_generator(flows_generator&&)                 = delete;
    flows_generator(const flows_generator&)            = delete;
    flows_generator& operator=(flows_generator&&)      = delete;
    flows_generator& operator=(const flows_generator&) = delete;
};

} // namespace gen::priv
