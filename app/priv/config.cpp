#include "app/priv/config.h"
#include "put/ether_addr.h"
#include "put/string_utils.h"
#include "put/throw.h"

namespace boost
{
// Add custom parsers here

void validate(boost::any& out,
              const std::vector<std::string>& val,
              baio_tcp_endpoint*,
              int)
{
    const auto& s = bpo::validators::get_single_string(val);
    std::vector<std::string_view> parts;
    balgo::split(
        parts, s, [](char c) { return (c == ':'); }, balgo::token_compress_off);
    if (parts.size() != 2) {
        put::throw_runtime_error("No port in endpoint: {}", val);
    }
    bsys::error_code ec;
    const auto addr = baio::ip::make_address_v4(parts[0], ec);
    if (ec) {
        put::throw_runtime_error("Invalid address in endpoint: {}", val);
    }
    const std::optional port = put::str_to_int<uint16_t>(parts[2]);
    if (!port) {
        put::throw_runtime_error("Invalid port in endpoint: {}", val);
    }
    out = baio_tcp_endpoint(addr, *port);
}

void validate(boost::any& out,
              const std::vector<std::string>& val,
              app::priv::config::cpu_idxs*,
              int)
{
    try {
        constexpr bool allow_empty = true;
        const auto& s = bpo::validators::get_single_string(val, allow_empty);
        std::vector<std::string_view> parts;
        balgo::split(
            parts, s, [](char c) { return (c == ','); },
            balgo::token_compress_on);
        app::priv::config::cpu_idxs ret;
        if (const auto cnt = ret.size(); parts.size() != cnt) {
            put::throw_runtime_error("Expect {} cpus", cnt);
        }
        for (auto idx = 0uz; const auto& p : parts) {
            ret[idx++] = put::str_to_int<uint16_t>(put::str_trim(p)).value();
        }
        out = std::move(ret);
    } catch (...) {
    }
}

} // namespace boost
////////////////////////////////////////////////////////////////////////////////
namespace app::priv
{

config::config(const stdfs::path& config_path)
{
    std::ifstream ifs(config_path.c_str());
    if (!ifs) {
        put::throw_runtime_error("Missing or non-accessible config file: {}.",
                                 config_path);
    }

    try {
        bpo::options_description desc;

        opts tmp;
        tmp.visit([&desc]<typename V>(const char* name, const V&) {
            desc.add_options()(name, bpo::value<V>());
        });

        constexpr bool allow_unreg_opts = true;
        bpo::variables_map vm;
        bpo::store(bpo::parse_config_file(ifs, desc, allow_unreg_opts), vm);
        bpo::notify(vm);

        std::string err;
        tmp.visit([&vm, &err]<typename V>(const std::string& name, V& val) {
            if (vm.count(name)) {
                try {
                    val = vm[name].as<V>();
                } catch (...) {
                    err += "\n\tInvalid config option '" + name + "'.";
                }
            } else {
                err += "\n\tMissing config option '" + name + "'.";
            }
        });
        if (!err.empty()) throw std::runtime_error(err);

        opts_ = std::move(tmp);
    } catch (const std::exception& ex) {
        put::throw_runtime_error("Invalid config file: {}. {}", config_path,
                                 ex.what());
    }
}

config::~config() noexcept = default;

} // namespace app::priv
////////////////////////////////////////////////////////////////////////////////
fmt::format_context::iterator fmt::formatter<app::priv::config>::format(
    const app::priv::config& arg, fmt::format_context& ctx) const noexcept
{
    auto it = ctx.out();
    arg.opts_.visit([&it](std::string_view name, const auto& val) {
        it = fmt::format_to(it, "\n\t{} = {}", name, val);
    });
    return it;
}

