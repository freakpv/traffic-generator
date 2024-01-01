#pragma once

namespace put
{
struct cycles;
} // namespace put
namespace gen::priv
{
class event_scheduler;

class event_handle
{
    // The `rte_timer` instance shouldn't be moved around once created because
    // the DPDK `rte_timer` functions store pointers to this instance and every-
    // thing will fall apart, if the timer instance is copied/moved/etc - UB!
    // I could have made the `event_handle` class non-copy-able/move-able but
    // this would have been too limiting for its users.
    // Thus the only other option is heap allocation of the `rte_timer`.
    std::unique_ptr<rte_timer> tmr_;
    event_scheduler* scheduler_ = nullptr;

public:
    using event_callback_type = void (*)(rte_timer*, void*);

public:
    event_handle() noexcept;

    explicit event_handle(event_scheduler*) noexcept;
    ~event_handle() noexcept;

    event_handle(event_handle&&) noexcept;
    event_handle& operator=(event_handle&&) noexcept;

    event_handle(const event_handle&)            = delete;
    event_handle& operator=(const event_handle&) = delete;

    // These functions schedule or re-schedule the event depending on its
    // current state.
    void schedule_single(put::cycles, event_callback_type, void*) noexcept;
    void schedule_periodic(put::cycles, event_callback_type, void*) noexcept;
};

}; // namespace gen::priv
