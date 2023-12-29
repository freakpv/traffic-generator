#pragma once

namespace gen::priv
{

// Decouple the interface for (re)scheduling events from the actual
// implementation
class event_scheduler
{
public:
    virtual ~event_scheduler() noexcept = default;
};

} // namespace gen::priv
