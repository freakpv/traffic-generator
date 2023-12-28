#pragma once

namespace gen::priv
{

class timer_subsystem
{
    // We need precision of 1 microsecond because:
    // - this is the precision of the PCAP timestamps
    // - this is the precision that we allow for the inter packet gaps
    const uint64_t usec_ticks_ = rte_get_tsc_hz() / 1'000'000ull;
    uint64_t prev_ticks_       = 0;

public:
    timer_subsystem() noexcept { rte_timer_subsystem_init(); }
    ~timer_subsystem() noexcept { rte_timer_subsystem_finalize(); }

    timer_subsystem(timer_subsystem&&)                 = delete;
    timer_subsystem(const timer_subsystem&)            = delete;
    timer_subsystem& operator=(timer_subsystem&&)      = delete;
    timer_subsystem& operator=(const timer_subsystem&) = delete;

    void process_events() noexcept
    {
        // The `prev_ticks_ + usec_ticks_` may overflow and this will cause
        // calling the `rte_timer_manage` function sooner than needed.
        // This is fine because if there are not timers expired nothing will
        // happen. In addition, the overflow will most likely never happen in
        // practice.
        if (const auto ticks = rte_get_tsc_cycles();
            ticks >= (prev_ticks_ + usec_ticks_)) {
            prev_ticks_ = ticks;
            rte_timer_manage();
        }
    }
};

} // namespace gen::priv
