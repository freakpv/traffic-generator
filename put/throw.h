#pragma once

namespace put  // project utilities
{

// clang-format off
template <typename... Args>
[[gnu::cold]]
[[noreturn]] 
inline void throw_runtime_error(fmt::format_string<Args...> fmtstr,
                                Args&&... args)
{
    fmt::memory_buffer mb;
    fmt::format_to(std::back_inserter(mb), fmtstr, std::forward<Args>(args)...);
    mb.push_back(0); // So data() is NULL terminated
    throw std::runtime_error(mb.data());
}

} // namespace put
