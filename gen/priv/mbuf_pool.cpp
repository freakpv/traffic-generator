#include "gen/priv/mbuf_pool.h"

#include "put/file_utils.h"
#include "put/throw.h"

namespace gen::priv
{

mbuf_pool::mbuf_pool(config cfg)
{
    // When creating the DPDK mbufs pool the DPDK framework opens file
    // descriptors for every huge page which is part of the pool. It does
    // this for the virtual to physical mapping machinery.
    // We add some number for other application needs.
    constexpr size_t huge_page_size = 2 * 1024 * 1024; // assume 2MB huge pages
    constexpr size_t other_fds      = 1024;
    constexpr size_t mbuf_size      = RTE_MBUF_DEFAULT_BUF_SIZE;
    constexpr size_t cache_size     = RTE_MEMPOOL_CACHE_MAX_SIZE;
    constexpr size_t priv_size      = 0;
    constexpr const char* name      = "tgn_mbuf_pool";
    const size_t cnt_fds =
        other_fds + std::max(1uz, (cfg.cnt_mbufs * mbuf_size) / huge_page_size);
    if (auto res = put::set_max_count_fds(cnt_fds); !res) {
        put::throw_system_error(
            res.error(),
            "Failed to set the max count of file descriptors to {}", cnt_fds);
    }

    auto* mp = rte_pktmbuf_pool_create(name, cfg.cnt_mbufs, cache_size,
                                       priv_size, mbuf_size, cfg.socket_id);
    if (!mp) {
        put::throw_dpdk_error(
            rte_errno,
            "Failed to create mbufs memory pool with name:{} and size:{}", name,
            cfg.cnt_mbufs);
    }
    pool_.reset(mp);
}

mbuf_pool::~mbuf_pool() noexcept                      = default;
mbuf_pool::mbuf_pool(mbuf_pool&&) noexcept            = default;
mbuf_pool& mbuf_pool::operator=(mbuf_pool&&) noexcept = default;

} // namespace gen::priv
