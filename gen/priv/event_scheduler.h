#pragma once

#include "gen/priv/event_handle.h"

#include "put/time_utils.h"

namespace gen::priv
{

class event_scheduler
{
    // We need precision of 1 microsecond because:
    // - this is the precision of the PCAP timestamps
    // - this is the precision that we allow for the inter packet gaps
    const put::cycles usec_cycles_ =
        put::cycles::from_duration(stdcr::microseconds{1});
    put::cycles prev_cycles_ = {0};

public:
    event_scheduler() noexcept { rte_timer_subsystem_init(); }
    ~event_scheduler() noexcept { rte_timer_subsystem_finalize(); }

    event_scheduler(event_scheduler&&)                 = delete;
    event_scheduler(const event_scheduler&)            = delete;
    event_scheduler& operator=(event_scheduler&&)      = delete;
    event_scheduler& operator=(const event_scheduler&) = delete;

    void process_events() noexcept
    {
        if (const auto cycles = put::cycles::current();
            (cycles - prev_cycles_) >= usec_cycles_) {
            prev_cycles_ = cycles;
            rte_timer_manage();
        }
    }

    // TODO:
    event_handle create_event() noexcept { return {}; }

    // It's unfortunate that the event scheduling system details leak here
    // with the first argument of the event callback.
    // I could have hidden it behind another callback layer but this would have
    // created another call indirection on every callback and ... here we are.
    // static void event_callback(rte_timer*, void*) noexcept;

    // TODO:
    size_t count_events() const noexcept { return 0; }
};

} // namespace gen::priv
