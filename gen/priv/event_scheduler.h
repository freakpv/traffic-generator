#pragma once

#include "put/time_utils.h"

namespace gen::priv
{

class event_scheduler
{
    // We need precision of 1 microsecond because:
    // - this is the precision of the PCAP timestamps
    // - this is the precision that we allow for the inter packet gaps
    const uint64_t usec_ticks_ = put::duration_to_tsc(stdcr::microseconds{1});
    uint64_t prev_ticks_       = 0;

public:
    event_scheduler() noexcept { rte_timer_subsystem_init(); }
    ~event_scheduler() noexcept { rte_timer_subsystem_finalize(); }

    event_scheduler(event_scheduler&&)                 = delete;
    event_scheduler(const event_scheduler&)            = delete;
    event_scheduler& operator=(event_scheduler&&)      = delete;
    event_scheduler& operator=(const event_scheduler&) = delete;

    void process_events() noexcept
    {
        if (const auto ticks = rte_get_tsc_cycles();
            (ticks - prev_ticks_) >= usec_ticks_) {
            prev_ticks_ = ticks;
            rte_timer_manage();
        }
    }

    // TODO:
    size_t count_events() const noexcept { return 0; }
};

} // namespace gen::priv