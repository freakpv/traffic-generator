#pragma once

namespace put
{

uint64_t duration_to_tsc(stdcr::seconds dur) noexcept
{
    return dur.count() * rte_get_tsc_hz();
}

uint64_t duration_to_tsc(stdcr::milliseconds dur) noexcept
{
    return (dur.count() * rte_get_tsc_hz()) / 1'000ul;
}

uint64_t duration_to_tsc(stdcr::microseconds dur) noexcept
{
    return (dur.count() * rte_get_tsc_hz()) / 1'000'000ul;
}

uint64_t duration_to_tsc(stdcr::nanoseconds dur) noexcept
{
    return (dur.count() * rte_get_tsc_hz()) / 1'000'000'000ul;
}

} // namespace put
