#pragma once

#include "gen/priv/event_scheduler.h"
#include "put/time_utils.h"

namespace gen::priv
{

class time_event_scheduler final : public event_scheduler
{
    // We need precision of 1 microsecond because:
    // - this is the precision of the PCAP timestamps
    // - this is the precision that we allow for the inter packet gaps
    const uint64_t usec_ticks_ = put::duration_to_tsc(stdcr::microseconds{1});
    uint64_t prev_ticks_       = 0;

public:
    time_event_scheduler() noexcept { rte_timer_subsystem_init(); }
    ~time_event_scheduler() noexcept override
    {
        rte_timer_subsystem_finalize();
    }

    time_event_scheduler(time_event_scheduler&&)                 = delete;
    time_event_scheduler(const time_event_scheduler&)            = delete;
    time_event_scheduler& operator=(time_event_scheduler&&)      = delete;
    time_event_scheduler& operator=(const time_event_scheduler&) = delete;

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
