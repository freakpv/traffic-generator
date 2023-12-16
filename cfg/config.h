#pragma once

namespace cfg
{
#define TGN_CONFIG_SETTINGS(MACRO)  \
    MACRO(stdfs::path, working_dir) \
    MACRO(rte_ether_addr, gw_ether_addr)

// The class holds the settings coming from the configuration file
class config
{
    friend class fmt::formatter<config>;

    struct opts
    {
#define XXX(type, name) type name##_ = {};
        TGN_CONFIG_SETTINGS(XXX)
#undef XXX

        template <typename Fun>
        void visit(Fun&& fun)
        {
#define XXX(type, name) fun(#name, name##_);
            TGN_CONFIG_SETTINGS(XXX)
#undef XXX
        }
        template <typename Fun>
        void visit(Fun&& fun) const
        {
#define XXX(type, name) fun(#name, name##_);
            TGN_CONFIG_SETTINGS(XXX)
#undef XXX
        }

    } opts_;

public:
    explicit config(const stdfs::path& config_path);
    ~config() noexcept;

#define XXX(type, name) \
    const type& name() const { return opts_.name##_; }
    TGN_CONFIG_SETTINGS(XXX)
#undef XXX
};

#undef TGN_CONFIG_SETTINGS
} // namespace cfg
////////////////////////////////////////////////////////////////////////////////
template <>
struct fmt::formatter<cfg::config>
{
    constexpr auto parse(auto& ctx) const noexcept { return ctx.begin(); }
    format_context::iterator format(const cfg::config&,
                                    format_context&) const noexcept;
};
