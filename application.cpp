#include "application.h"

#include "cfg/config.h"
#include "log/log.h"

#include "put/tg_assert.h"
#include "put/file_utils.h"
#include "put/throw.h"

////////////////////////////////////////////////////////////////////////////////

class application_impl
{
public:
    explicit application_impl(const cfg::config&) {}

    void run() noexcept {}

public:
    static void ensure_enough_filedesc();
    static void init_dpdk_eal(const cfg::config&);
};

void application_impl::ensure_enough_filedesc()
{
    // When creating the DPDK mbufs pool the DPDK framework opens file
    // descriptors for every huge page which is part of the pool. It does
    // this for the virtual to physical mapping machinery. Here we just
    // ensure that there are enough file descriptors even for very large
    // amount of huge pages. 65536 * 2 MB huge page size ensures that the
    // application will be able to open file descriptors for almost 128 GB
    // huge pages which is more than it'll even need in practice.
    constexpr auto cnt_fds = 64 * 1024uz;
    if (auto res = put::set_max_count_fds(cnt_fds); !res) {
        put::throw_system_error(
            res.error(),
            "Failed to set the max count of file descriptors to {}!", cnt_fds);
    }
    TGLOG_INFO("Set max file descriptors to {}\n", cnt_fds);
}

void application_impl::init_dpdk_eal(const cfg::config& cfg)
{
    auto make_arg = []<typename... Args>(std::span<char>& buf,
                                         fmt::format_string<Args...> fmtstr,
                                         Args&&... args) {
        const auto res = fmt::format_to_n(buf.data(), buf.size(), fmtstr,
                                          std::forward<Args>(args)...);
        TG_ENFORCE(buf.size() > res.size);
        buf[res.size] = '\0';
        char* ret     = buf.data();
        buf           = buf.subspan(res.size + 1);
        return ret;
    };

    const auto& cpus = cfg.cpus();
    TG_ENFORCE(cpus.size() == 2);
    // The argc/argv must live throughout the application lifetime.
    // That's why the `args` is static and the memory for them too.
    static std::array<char, 1024> mem_buf;
    std::span<char> buf(mem_buf);
    static std::array args = {
        make_arg(buf, "xproxy"), make_arg(buf, "--no-telemetry"),
        make_arg(buf, "-l {},{}", cpus[0], cpus[1]),
        make_arg(buf, "-n {}", cfg.num_memory_channels())};
    if (rte_eal_init(args.size(), args.data()) < 0) {
        put::throw_dpdk_error(
            rte_errno, "Failed to initialize the DPDK abstraction layer");
    }
    TG_ENFORCE(rte_eal_process_type() == RTE_PROC_PRIMARY);
}

////////////////////////////////////////////////////////////////////////////////

application::application(const stdfs::path& cfg_path)
{
    cfg::config cfg(cfg_path);
    try {
        TGLOG_INFO("Starting with with settings: {}\n", cfg);
        application_impl::ensure_enough_filedesc();
        application_impl::init_dpdk_eal(cfg);
        stdex::scope_fail cleanup([] { rte_eal_cleanup(); });
        impl_ = std::make_unique<application_impl>(cfg);
    } catch (const std::exception& ex) {
        put::throw_runtime_error("Can not create Traffic-Generator. {}",
                                 ex.what());
    }
}

application::~application() noexcept = default;

void application::run() noexcept
{
    impl_->run();
}
