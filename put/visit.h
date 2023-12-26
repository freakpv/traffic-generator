#pragma once

namespace put // project utilities
{

template <typename Variant, typename... Visitor>
decltype(auto) visit(Variant&& var, Visitor&&... vis)
{
    using variant_type = std::remove_cvref_t<Variant>;
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
