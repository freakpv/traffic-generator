#pragma once

namespace put // project utilities
{

// Range [x1, x2)
template <std::integral T, std::integral U, std::integral V>
constexpr bool in_range(T x, U x1, V x2) noexcept
{
    return (x1 <= x) && (x < x2);
}

// Range [x1, x2)
template <std::integral T, std::integral U, std::integral V>
constexpr bool in_range_inclusive(T x, U x1, V x2) noexcept
{
    return (x1 <= x) && (x <= x2);
}

} // namespace put
