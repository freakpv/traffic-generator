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
    // The incoming queue for the management module is outgoing queue for us.
    // And vice versa.
    struct config
    {
        stdfs::path working_dir;
        uint32_t max_cnt_mbufs;
        uint16_t nic_queue_size;
        mgmt::out_messages_queue* inc_queue;
        mgmt::inc_messages_queue* out_queue;
    };

public:
    explicit manager(const config&);
    ~manager() noexcept;

    manager()                          = delete;
    manager(manager&&)                 = delete;
    manager(const manager&)            = delete;
    manager& operator=(manager&&)      = delete;
    manager& operator=(const manager&) = delete;

    void process_events() noexcept;
};

} // namespace gen
