#pragma once

#include "gen/priv/event_handle.h"

#include "put/tg_assert.h"
#include "put/time_utils.h"

namespace gen::priv
{

class event_scheduler
{
    // Specific timer data instance could have been used with
    // rte_timer_data_alloc/rte_timer_data_dealloc but currently this is the
    // only `rte_timer` based functionality and that's why the functionality
    // uses the default timer data.

    // We need precision of 1 microsecond because:
    // - this is the precision of the PCAP timestamps
    // - this is the precision that we allow for the inter packet gaps
    const put::cycles usec_cycles_ =
        put::cycles::from_duration(stdcr::microseconds{1});
    put::cycles prev_cycles_ = {0};

    uint32_t cnt_timers_ = 0;

    const uint32_t lcore_id_ = rte_lcore_id();

public:
    // It's unfortunate that the event scheduling system details leak here
    // with the first argument of the event callback.
    // I could have hidden it behind another callback layer but this would have
    // created another call indirection on every callback and ... here we are.
    using event_callback_type = void (*)(rte_timer*, void*);
    using timer_ptr_type      = std::unique_ptr<rte_timer>;

public:
    event_scheduler() noexcept { rte_timer_subsystem_init(); }
    ~event_scheduler() noexcept
    {
        // All timers should have been returned to the scheduler.
        // Assertion here would mean that there is an event handle which still
        // holds a timer and pointer to the scheduler instance.
        // It'd be an UB if the handle tries to use the timer/scheduler.
        TG_ENFORCE(cnt_timers_ == 0);
        rte_timer_subsystem_finalize();
    }

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

    timer_ptr_type get_timer() noexcept
    {
        auto ret = std::make_unique<rte_timer>();
        rte_timer_init(ret.get());
        ++cnt_timers_;
        return ret;
    }
    void ret_timer(timer_ptr_type&& tmr) noexcept
    {
        int r = rte_timer_stop(tmr.get());
        // Assertion here would mean that the event handle is incorrectly used.
        // More likely destroying an event from the callback of another one.
        TG_ENFORCE(r == 0);
        tmr.reset();
        --cnt_timers_;
    }

    // These functions schedule or re-schedule the event depending on its
    // current state.
    void schedule_single(timer_ptr_type& tmr,
                         put::cycles rel_time,
                         event_callback_type cb,
                         void* ctx) noexcept
    {
        int r = rte_timer_reset(tmr.get(), rel_time.num, rte_timer_type::SINGLE,
                                lcore_id_, cb, ctx);
        // Assertion here would mean that the event handle is incorrectly used.
        // More likely scheduling an event from the callback of another one.
        TG_ENFORCE(r == 0);
    }
    void schedule_periodic(timer_ptr_type& tmr,
                           put::cycles rel_time,
                           event_callback_type cb,
                           void* ctx) noexcept
    {
        int r = rte_timer_reset(tmr.get(), rel_time.num,
                                rte_timer_type::PERIODICAL, lcore_id_, cb, ctx);
        // Assertion here would mean that the event handle is incorrectly used.
        // More likely scheduling an event from the callback of another one.
        TG_ENFORCE(r == 0);
    }

    size_t count_events() const noexcept { return cnt_timers_; }
};

} // namespace gen::priv
