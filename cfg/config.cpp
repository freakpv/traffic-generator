#include "cfg/config.h"
#include "put/ether_addr.h"
#include "put/throw.h"

namespace boost
{
// Add custom parsers here

void validate(boost::any& out,
              const std::vector<std::string>& val,
              rte_ether_addr*,
              int)
{
    const auto& saddr = bpo::validators::get_single_string(val);
    const auto addr   = put::parse_ether_addr(saddr);
    if (!addr) {
        put::throw_runtime_error("Failed to parse ethernet address: '{}'",
                                 saddr);
    }
    out = *addr;
}

} // namespace boost
////////////////////////////////////////////////////////////////////////////////
namespace cfg
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

fmt::format_context::iterator tgn_format_to(fmt::format_context::iterator it,
                                            const config& cfg) noexcept
{
    cfg.opts_.visit([&it]<typename V>(std::string_view name, const V& val) {
        it = fmt::format_to(it, "\n\t{} = {}", name, val);
    });
    return it;
}

} // namespace cfg
