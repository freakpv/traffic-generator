#pragma once

namespace put // project utilities
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

template <typename... Args>
[[gnu::cold]]
[[noreturn]] 
inline void throw_system_error(int err, 
                               fmt::format_string<Args...> fmtstr, 
                               Args&&... args)
{
    fmt::memory_buffer mb;
    fmt::format_to(std::back_inserter(mb), fmtstr, std::forward<Args>(args)...);
    mb.push_back(0); // So data() is NULL terminated
    throw bsys::system_error(err, bsys::system_category(), mb.data());
}

template <typename... Args>
[[gnu::cold]]
[[noreturn]] 
inline void throw_system_error(const bsys::error_code& ec,
                               fmt::format_string<Args...> fmtstr,
                               Args&&... args)
{
    fmt::memory_buffer mb;
    fmt::format_to(std::back_inserter(mb), fmtstr, std::forward<Args>(args)...);
    mb.push_back(0); // So data() is NULL terminated
    throw bsys::system_error(ec, mb.data());
}

class dpdk_error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

template <typename... Args>
[[gnu::cold]]
[[noreturn]]
inline void throw_dpdk_error(int dpdk_errno,
                             fmt::format_string<Args...> fmtstr,
                             Args&&... args)
{
    fmt::memory_buffer mb;
    fmt::format_to(std::back_inserter(mb), fmtstr, std::forward<Args>(args)...);
    fmt::format_to(std::back_inserter(mb), ": {}", rte_strerror(dpdk_errno));
    mb.push_back(0); // So data() is NULL terminated
    throw dpdk_error(mb.data());
}
// clang-format on

} // namespace put
