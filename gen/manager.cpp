#include "gen/manager.h"
#include "gen/priv/eth_dev.h"

#include "mgmt/gen_config.h"
#include "mgmt/messages.h"

namespace gen // generator
{

class manager_impl
{
    gen::priv::eth_dev if_;
    mgmt::inc_messages_queue& out_queue_;
    mgmt::out_messages_queue& inc_queue_;

    const stdfs::path working_dir_;

public:
    manager_impl(const stdfs::path& working_dir,
                 uint16_t nic_queue_size,
                 mgmt::inc_messages_queue&,
                 mgmt::out_messages_queue&);

    void process_events() noexcept;
};

////////////////////////////////////////////////////////////////////////////////

manager_impl::manager_impl(const stdfs::path& working_dir,
                           uint16_t,
                           mgmt::inc_messages_queue& out_queue,
                           mgmt::out_messages_queue& inc_queue)
: if_(gen::priv::eth_dev::config{}) // TODO
, out_queue_(out_queue)
, inc_queue_(inc_queue)
, working_dir_(working_dir)
{
    // TODO:
}

void manager_impl::process_events() noexcept
{
    // TODO:
}

////////////////////////////////////////////////////////////////////////////////

manager::manager(const stdfs::path& working_dir,
                 uint16_t nic_queue_size,
                 mgmt::inc_messages_queue& out_queue,
                 mgmt::out_messages_queue& inc_queue)
: impl_(std::make_unique<manager_impl>(
      working_dir, nic_queue_size, out_queue, inc_queue))
{
}

manager::~manager() noexcept = default;

void manager::process_events() noexcept
{
    impl_->process_events();
}

} // namespace gen
