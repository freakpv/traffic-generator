#include "application.h"

#include "cfg/config.h"
#include "log/log.h"

#include "put/file_utils.h"
#include "put/throw.h"

namespace
{

void ensure_enough_filedesc()
{
    // When creating the DPDK mbufs pool the DPDK framework opens file
    // descriptors for every huge page which is part of the pool. It does this
    // for the virtual to physical mapping machinery.
    // Here we just ensure that there are enough file descriptors even for
    // very large amount of huge pages.
    // 65536 * 2 MB huge page size ensures that the application will be able
    // to open file descriptors for almost 128 GB huge pages which is more than
    // it'll even need in practice.
    constexpr auto cnt_fds = 64 * 1024uz;
    if (auto res = put::set_max_count_fds(cnt_fds); !res) {
        put::throw_system_error(
            res.error(),
            "Failed to set the max count of file descriptors to {}!", cnt_fds);
    }
    TGLOG_INFO("Set max file descriptors to {}\n", cnt_fds);
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

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
        ensure_enough_filedesc();
        // dpdk_eal_init(cfg);
        stdex::scope_fail cleanup([] { rte_eal_cleanup(); });
        impl_ = std::make_unique<application_impl>(cfg);
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
