#pragma once

namespace put // project utilities
{

inline bsys::error_code system_error_code(int ec) noexcept
{
    return {ec, bsys::system_category()};
}

} // namespace put
