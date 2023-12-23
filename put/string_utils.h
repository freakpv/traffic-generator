#pragma once

namespace put
{

[[nodiscard]] inline std::string_view
str_trim_left(std::string_view sv) noexcept
{
    using namespace std::string_view_literals;
    sv.remove_prefix(std::min(sv.find_first_not_of(" \t"sv), sv.size()));
    return sv;
}

[[nodiscard]] inline std::string_view
str_trim_right(std::string_view sv) noexcept
{
    using namespace std::string_view_literals;
    auto pos = sv.find_last_not_of(" \t"sv);
    if (pos == std::string_view::npos) return {};
    sv.remove_suffix(sv.size() - pos - 1);
    return sv;
}

[[nodiscard]] inline std::string_view str_trim(std::string_view sv) noexcept
{
    return str_trim_right(str_trim_left(sv));
}

template <std::integral Int>
[[nodiscard]] inline std::optional<Int>
str_to_int(std::string_view str) noexcept
{
    Int ret              = {};
    const auto beg       = str.data();
    const auto end       = str.data() + str.size();
    const auto [ptr, ec] = std::from_chars(beg, end, ret, 10);
    if (ptr == end) return ret;
    return std::nullopt;
}

// The caller needs to ensure that the passed buffer is big enough
[[nodiscard]] inline std::string_view int_to_str(std::integral auto i,
                                                 std::span<char> buff) noexcept
{
    const auto res = fmt::format_to_n(buff.data(), buff.size(), "{}", i);
    X3ME_ENFORCE(res.size <= buff.size());
    return {buff.data(), res.size};
}

} // namespace put
