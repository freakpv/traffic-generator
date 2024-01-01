#include "app/application.h"
#include "app/priv/config.h"

#include "gen/manager.h"

#include "log/tg_log.h"

#include "mgmt/manager.h"
#include "mgmt/messages.h"

#include "put/tg_assert.h"
#include "put/file_utils.h"
#include "put/throw.h"

////////////////////////////////////////////////////////////////////////////////
namespace app
{

class application_impl
{
    struct dpdk_eal
    {
        dpdk_eal(const app::priv::config&);
        ~dpdk_eal() noexcept;

        dpdk_eal()                           = delete;
        dpdk_eal(dpdk_eal&)                  = delete;
        dpdk_eal(const dpdk_eal&)            = delete;
        dpdk_eal& operator=(dpdk_eal&)       = delete;
        dpdk_eal& operator=(const dpdk_eal&) = delete;
    };

    [[no_unique_address]] dpdk_eal eal_;

    mgmt::inc_messages_queue g2m_queue_;
    mgmt::out_messages_queue m2g_queue_;

    gen::manager genr_;
    mgmt::manager mgmt_;

    const std::array<uint16_t, 2> cpus_;

    static inline std::atomic_flag stop_flag_{};

public:
    explicit application_impl(const app::priv::config&);

    void run() noexcept;

private:
    static void signal_handler(int sig) noexcept;
};

////////////////////////////////////////////////////////////////////////////////

application_impl::application_impl(const app::priv::config& cfg)
: eal_(cfg)
, genr_({.working_dir    = cfg.working_dir(),
         .max_cnt_mbufs  = cfg.max_cnt_mbufs(),
         .nic_queue_size = cfg.nic_queue_size(),
         .inc_queue      = &m2g_queue_,
         .out_queue      = &g2m_queue_})
, mgmt_({.endpoint  = cfg.mgmt_endpoint(),
         .inc_queue = &g2m_queue_,
         .out_queue = &m2g_queue_})
, cpus_(cfg.cpus())
{
    auto sig_sub = [](auto... sig) {
        return ((::signal(sig, signal_handler) != SIG_ERR) && ...);
    };
    if (!sig_sub(SIGINT, SIGTERM)) {
        put::throw_system_error(errno, "Failed to subscribe to the OS signal");
    }
}

void application_impl::run() noexcept
{
    auto run_loop = [](auto& mgr) {
        while (!stop_flag_.test(std::memory_order_seq_cst)) {
            mgr.process_events();
        }
    };

    if (const auto lcore = rte_lcore_id(); lcore == cpus_[0]) {
        run_loop(mgmt_);
    } else if (lcore == cpus_[1]) {
        run_loop(genr_);
    } else {
        TG_UNREACHABLE();
    }
}

void application_impl::signal_handler(int sig) noexcept
{
    switch (sig) {
    case SIGINT: {
        TG_LOG_INFO("Stop requested via SIGINT\n");
        stop_flag_.test_and_set(std::memory_order_seq_cst);
    } break;
    case SIGTERM:
        TG_LOG_INFO("Stop requested via SIGTERM\n");
        stop_flag_.test_and_set(std::memory_order_seq_cst);
        break;
    default: TG_UNREACHABLE(); break;
    }
}

application_impl::dpdk_eal::dpdk_eal(const app::priv::config& cfg)
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

application_impl::dpdk_eal::~dpdk_eal() noexcept
{
    rte_eal_cleanup();
}

////////////////////////////////////////////////////////////////////////////////

application::application(const stdfs::path& cfg_path)
{
    app::priv::config cfg(cfg_path);
    TG_LOG_INFO("Starting with with settings: {}\n", cfg);
    impl_ = std::make_unique<application_impl>(cfg);
}

application::~application() noexcept = default;

void application::run() noexcept
{
    rte_eal_mp_remote_launch(
        [](void* impl) {
            static_cast<application_impl*>(impl)->run();
            return 0;
        },
        impl_.get(), CALL_MAIN);
    rte_eal_mp_wait_lcore();

    TG_LOG_INFO("Application stopped\n");
}

} // namespace app
