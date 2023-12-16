#pragma once

namespace put
{

inline bout::result<void> set_max_count_fds(uint32_t cnt) noexcept
{
    rlimit cur_lims = {};
    if (::getrlimit(RLIMIT_NOFILE, &cur_lims) != 0)
        return tglib::system_error_code(errno);

    // Don't try to change the hard limit if it's big enough.
    rlimit new_lims   = cur_lims;
    new_lims.rlim_cur = cnt;
    if (cnt > new_lims.rlim_max) new_lims.rlim_max = cnt;

    if (::setrlimit(RLIMIT_NOFILE, &new_lims) != 0)
        return tglib::system_error_code(errno);

    return bout::success();
}

} // namespace put
