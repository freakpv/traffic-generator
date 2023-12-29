#pragma once

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
