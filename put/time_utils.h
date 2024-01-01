#pragma once

// TODO: Use rte_get_timer_hz and rte_get_timer_cycles
// because this is what rte_timer functions use internally.

namespace put
{

inline uint64_t duration_to_tsc(stdcr::seconds dur) noexcept
{
    return dur.count() * rte_get_tsc_hz();
}

inline uint64_t duration_to_tsc(stdcr::milliseconds dur) noexcept
{
    return (dur.count() * rte_get_tsc_hz()) / 1'000ul;
}

inline uint64_t duration_to_tsc(stdcr::microseconds dur) noexcept
{
    return (dur.count() * rte_get_tsc_hz()) / 1'000'000ul;
}

inline uint64_t duration_to_tsc(stdcr::nanoseconds dur) noexcept
{
    return (dur.count() * rte_get_tsc_hz()) / 1'000'000'000ul;
}

} // namespace put
