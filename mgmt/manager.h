#pragma once

namespace mgmt // management
{

class manager
{
    std::unique_ptr<class manager_impl> impl_;

public:
    explicit manager(const baio_tcp_endpoint&);
    ~manager() noexcept;

    manager()                          = delete;
    manager(manager&&)                 = delete;
    manager(const manager&)            = delete;
    manager& operator=(manager&&)      = delete;
    manager& operator=(const manager&) = delete;

    void process_events() noexcept;
};

} // namespace mgmt
