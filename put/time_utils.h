#pragma once

namespace put
{
// The functionality here:
// - allows easy switching between rte_get_tsc_cycles/rte_get_tsc_hz and
// rte_get_timer_cycles/rte_get_timer_hz. Currently the functionality uses the
// latter pair of functions because the DPDK `rte_timer` functionality uses them
// - strong typing is always better than a weak one
// - provides conversion functions between the std::chrono duration types and
// the `cycles` type.
struct cycles
{
    uint64_t num;

    static cycles current() noexcept { return {rte_get_timer_cycles()}; }
    static uint64_t frequency_hz() noexcept { return rte_get_timer_hz(); }

    static cycles from_duration(stdcr::seconds dur) noexcept
    {
        return {dur.count() * frequency_hz()};
    }
    static cycles from_duration(stdcr::milliseconds dur) noexcept
    {
        return {(dur.count() * frequency_hz()) / 1'000ul};
    }
    static cycles from_duration(stdcr::microseconds dur) noexcept
    {
        return {(dur.count() * frequency_hz()) / 1'000'000ul};
    }
    static cycles from_duration(stdcr::nanoseconds dur) noexcept
    {
        return {(dur.count() * frequency_hz()) / 1'000'000'000ul};
    }

    template <typename Dur>
    Dur to() const noexcept
    {
        if constexpr (std::is_same_v<Dur, stdcr::seconds>)
            return Dur(num / frequency_hz());
        else if constexpr (std::is_same_v<Dur, stdcr::milliseconds>)
            return Dur((num * 1'000ul) / frequency_hz());
        else if constexpr (std::is_same_v<Dur, stdcr::microseconds>)
            return Dur((num * 1'000'000ul) / frequency_hz());
        else if constexpr (std::is_same_v<Dur, stdcr::nanoseconds>)
            return Dur((num * 1'000'000'000ul) / frequency_hz());
        else
            static_assert(false, "Unsupported duration type");
    }

    constexpr cycles& operator+=(const cycles& rhs) noexcept
    {
        num += rhs.num;
        return *this;
    }
    constexpr cycles& operator-=(const cycles& rhs) noexcept
    {
        num -= rhs.num;
        return *this;
    }

    constexpr cycles operator+(const cycles& rhs) const noexcept
    {
        return {num + rhs.num};
    }
    constexpr cycles operator-(const cycles& rhs) const noexcept
    {
        return {num - rhs.num};
    }

    constexpr bool operator==(const cycles&) const noexcept  = default;
    constexpr auto operator<=>(const cycles&) const noexcept = default;
};

} // namespace put
