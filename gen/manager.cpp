#include "gen/manager.h"
#include "gen/priv/eth_dev.h"
#include "gen/priv/mbuf_pool.h"

#include "mgmt/gen_config.h"
#include "mgmt/messages.h"

namespace gen // generator
{

class manager_impl
{
    using config_type = manager::config;

    // Currently we always work with single port only and the ports start from 0
    static constexpr uint16_t nic_port_id = 0;

    gen::priv::mbuf_pool pool_;
    gen::priv::eth_dev if_;
    mgmt::out_messages_queue* inc_queue_;
    mgmt::inc_messages_queue* out_queue_;

    const stdfs::path working_dir_;

public:
    manager_impl(const config_type&);

    void process_events() noexcept;
};

////////////////////////////////////////////////////////////////////////////////

manager_impl::manager_impl(const config_type& cfg)
: pool_({.cnt_mbufs = cfg.max_cnt_mbufs, .socket_id = rte_socket_id()})
, if_({.port_id    = nic_port_id,
       .queue_size = cfg.nic_queue_size,
       .socket_id  = rte_socket_id(),
       .mempool    = pool_.pool()})
, inc_queue_(cfg.inc_queue)
, out_queue_(cfg.out_queue)
, working_dir_(cfg.working_dir)
{
    // TODO:
}

void manager_impl::process_events() noexcept
{
    // TODO:
}

////////////////////////////////////////////////////////////////////////////////

manager::manager(const config& cfg) : impl_(std::make_unique<manager_impl>(cfg))
{
}

manager::~manager() noexcept = default;

void manager::process_events() noexcept
{
    impl_->process_events();
}

} // namespace gen
