#pragma once

namespace app::priv
{
// boost::container::vector is used instead of std::vector because the
// boost::program_options library provides special treatment for std::vector
// which is not applicable to our usage.
#define TGN_CONFIG_SETTINGS(MACRO)          \
    MACRO(stdfs::path, working_dir)         \
    MACRO(baio_tcp_endpoint, mgmt_endpoint) \
    MACRO(cpu_idxs, cpus)                   \
    MACRO(uint16_t, num_memory_channels)    \
    MACRO(uint16_t, nic_queue_size)

// The class holds the settings coming from the configuration file
class config
{
    friend class fmt::formatter<config>;

public:
    struct cpu_idxs : std::array<uint16_t, 2>
    {
    };

private:
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
} // namespace app::priv
////////////////////////////////////////////////////////////////////////////////
template <>
struct fmt::formatter<app::priv::config>
{
    constexpr auto parse(auto& ctx) const noexcept { return ctx.begin(); }
    format_context::iterator format(const app::priv::config&,
                                    format_context&) const noexcept;
};
