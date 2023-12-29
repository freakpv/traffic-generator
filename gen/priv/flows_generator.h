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
    // TODO:
public:
    flows_generator(const mgmt::flows_config&,
                    const rte_ether_addr&,
                    event_scheduler*)
    {
    }
    ~flows_generator() noexcept {}

    flows_generator()                                  = delete;
    flows_generator(flows_generator&&)                 = delete;
    flows_generator(const flows_generator&)            = delete;
    flows_generator& operator=(flows_generator&&)      = delete;
    flows_generator& operator=(const flows_generator&) = delete;
};

} // namespace gen::priv
