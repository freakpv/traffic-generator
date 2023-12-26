#pragma once

namespace mgmt // management
{
struct inc_messages_queue;
struct out_messages_queue;

class manager
{
    std::unique_ptr<class manager_impl> impl_;

public:
    struct config
    {
        baio_tcp_endpoint endpoint;
        inc_messages_queue* inc_queue;
        out_messages_queue* out_queue;
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

} // namespace mgmt
