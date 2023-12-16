#include "application.h"

#include "cfg/config.h"
#include "log/log.h"
#include "put/throw.h"

class application_impl
{
public:
    explicit application_impl(const cfg::config&) {}

    void run() noexcept {}
};

application::application(const stdfs::path& cfg_path)
{
    cfg::config cfg(cfg_path);
    try {
        TGLOG_INFO("Starting with with settings: {}\n", cfg);
        // init_max_count_filedesc(cfg);
        // dpdk_eal_init(cfg);
        // xlib::scope_guard cleanup([] { rte_eal_cleanup(); });
        // impl_ = mem::make_unique<application_impl>(cfg);
    } catch (const std::exception& ex) {
        put::throw_runtime_error("Can not construct Traffic-Generator. {}",
                                 ex.what());
    }
}

application::~application() noexcept = default;

void application::run() noexcept
{
    impl_->run();
}
