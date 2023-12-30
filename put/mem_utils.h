#pragma once

namespace put
{
// GCC 13.2 still doesn't have these from C++23.
// The implementation is taken from the CppConn 2022 talk by Robert Leahy
// Taking a Byte Out of C++ - Avoiding Punning by Starting Lifetimes
template <typename T>
[[nodiscard]] T* start_lifetime_as(void* mem) noexcept
{
    auto bytes = new (mem) unsigned char[sizeof(T)];
    auto ptr   = reinterpret_cast<T*>(bytes);
    (void)*ptr;
    return ptr;
}

template <typename T>
[[nodiscard]] const T* start_lifetime_as(const void* mem) noexcept
{
    const auto mp    = const_cast<void*>(mem);
    const auto bytes = new (mp) unsigned char[sizeof(T)];
    const auto ptr   = reinterpret_cast<const T*>(bytes);
    (void)*ptr;
    return ptr;
}

} // namespace put
