#pragma once

namespace put // project utilities
{

template <class... Funs>
struct overload : Funs...
{
    using Funs::operator()...;
};
template <class... Funs>
overload(Funs...) -> overload<Funs...>;

template <typename Variant, typename... Visitor>
decltype(auto) visit(Variant&& var, Visitor&&... vis)
{
    using variant_type = std::remove_cvref_t<Variant>;
    auto fun           = overload(std::forward<Visitor>(vis)...);
    return bmp11::mp_with_index<std::variant_size_v<variant_type>>(
        var.index(), [&](auto idx) -> decltype(auto) {
            using std::get_if;
            if constexpr (std::is_rvalue_reference_v<decltype(var)>)
                return fun(std::move(*get_if<idx>(&var)));
            else
                return fun(*get_if<idx>(&var));
        });
}

} // namespace put
