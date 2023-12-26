#include "gen/manager.h"
#include "gen/priv/eth_dev.h"

#include "mgmt/gen_config.h"
#include "mgmt/messages.h"

namespace gen // generator
{
/*
void application_impl::ensure_enough_filedesc()
{
    // When creating the DPDK mbufs pool the DPDK framework opens file
    // descriptors for every huge page which is part of the pool. It does
    // this for the virtual to physical mapping machinery. Here we just
    // ensure that there are enough file descriptors even for very large
    // amount of huge pages. 65536 * 2 MB huge page size ensures that the
    // application will be able to open file descriptors for almost 128 GB
    // huge pages which is more than it'll even need in practice.
    constexpr auto cnt_fds = 64 * 1024uz;
    if (auto res = put::set_max_count_fds(cnt_fds); !res) {
        put::throw_system_error(
            res.error(),
            "Failed to set the max count of file descriptors to {}!", cnt_fds);
    }
    TGLOG_INFO("Set max file descriptors to {}\n", cnt_fds);
}
*/

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
