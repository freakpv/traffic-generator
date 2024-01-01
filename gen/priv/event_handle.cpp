#include "gen/priv/event_handle.h"
#include "gen/priv/event_scheduler.h"

namespace gen::priv
{

event_handle::event_handle() noexcept = default;

event_handle::event_handle(event_scheduler* scheduler) noexcept
: tmr_(scheduler->get_timer()), scheduler_(scheduler)
{
}
event_handle::~event_handle() noexcept
{
    if (tmr_) scheduler_->ret_timer(std::move(tmr_));
}

event_handle::event_handle(event_handle&& rhs) noexcept
: tmr_(std::move(rhs.tmr_)), scheduler_(std::exchange(rhs.scheduler_, nullptr))
{
}

event_handle& event_handle::operator=(event_handle&& rhs) noexcept
{
    using std::swap;
    auto tmp(std::move(rhs));
    swap(tmp, *this);
    return *this;
}

void event_handle::schedule_single(put::cycles rel_time,
                                   event_callback_type cb,
                                   void* ctx) noexcept
{
    scheduler_->schedule_single(tmr_, rel_time, cb, ctx);
}

void event_handle::schedule_periodic(put::cycles rel_time,
                                     event_callback_type cb,
                                     void* ctx) noexcept
{
    scheduler_->schedule_periodic(tmr_, rel_time, cb, ctx);
}

} // namespace gen::priv
