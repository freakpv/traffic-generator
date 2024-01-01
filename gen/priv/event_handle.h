#pragma once

namespace gen::priv
{

class event_handle
{
    // TODO:
public:
    event_handle() noexcept = default;

    template <typename... Args>
    void schedule_single(Args&&...) noexcept
    {
    }
};

}; // namespace gen::priv
