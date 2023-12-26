#pragma once

namespace mgmt // management
{
struct inc_messages_queue;
struct out_messages_queue;

class manager
{
    std::unique_ptr<class manager_impl> impl_;

public:
    manager(const baio_tcp_endpoint&, inc_messages_queue&, out_messages_queue&);
    ~manager() noexcept;

    manager()                          = delete;
    manager(manager&&)                 = delete;
    manager(const manager&)            = delete;
    manager& operator=(manager&&)      = delete;
    manager& operator=(const manager&) = delete;

    void process_events() noexcept;
};

} // namespace mgmt
