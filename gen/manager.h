#pragma once

namespace mgmt // management
{
struct inc_messages_queue;
struct out_messages_queue;
} // namespace mgmt
////////////////////////////////////////////////////////////////////////////////
namespace gen // generator
{

class manager
{
    std::unique_ptr<class manager_impl> impl_;

public:
    manager(const stdfs::path& working_dir,
            uint16_t nic_queue_size,
            mgmt::inc_messages_queue&,
            mgmt::out_messages_queue&);
    ~manager() noexcept;

    manager()                          = delete;
    manager(manager&&)                 = delete;
    manager(const manager&)            = delete;
    manager& operator=(manager&&)      = delete;
    manager& operator=(const manager&) = delete;

    void process_events() noexcept;
};

} // namespace gen
