#include "gen/priv/eth_dev.h"

#include "put/throw.h"
#include "put/tg_assert.h"

namespace gen::priv
{

eth_dev::eth_dev(config cfg)
{
    const auto [dev_info, dev_conf] = set_capabilities(cfg);

    configure(cfg, dev_info, dev_conf);

    stdex::scope_fail close_dev([&] { rte_eth_dev_close(cfg.port_id); });

    start(cfg);

    // Everything has been setup successfully. Mark the device as valid.
    port_id_ = cfg.port_id;
}

eth_dev::~eth_dev() noexcept
{
    if (is_valid()) {
        rte_eth_dev_stop(port_id_);
        rte_eth_dev_close(port_id_);
    }
}

eth_dev::eth_dev(eth_dev&& rhs) noexcept
: port_id_(std::exchange(rhs.port_id_, invalid_port_id))
{
}

eth_dev& eth_dev::operator=(eth_dev&& rhs) noexcept
{
    using std::swap;
    eth_dev tmp(std::move(rhs));
    swap(port_id_, tmp.port_id_);
    return *this;
}

rte_eth_link eth_dev::get_link_info() const noexcept
{
    TG_ASSERT(is_valid());
    rte_eth_link ret = {};
    rte_eth_link_get_nowait(port_id_, &ret);
    return ret;
}

rte_ether_addr eth_dev::get_mac_addr() const noexcept
{
    TG_ASSERT(is_valid());
    rte_ether_addr ret = {};
    rte_eth_macaddr_get(port_id_, &ret);
    return ret;
}

rte_eth_dev_info eth_dev::get_dev_info() const noexcept
{
    TG_ASSERT(is_valid());
    rte_eth_dev_info ret = {};
    rte_eth_dev_info_get(port_id_, &ret);
    return ret;
}

////////////////////////////////////////////////////////////////////////////////

std::pair<rte_eth_dev_info, rte_eth_conf>
eth_dev::set_capabilities(const config& cfg)
{
    rte_eth_dev_info dev_info = {};
    rte_eth_conf dev_conf     = {};
    auto check_capa           = [](uint64_t capa, uint64_t flag) {
        return ((capa & flag) == flag);
    };

    if (int ret = rte_eth_dev_info_get(cfg.port_id, &dev_info); ret != 0) {
        put::throw_dpdk_error(
            -ret,
            "Failed to initialize DPDK port {}. Failed to get device info",
            cfg.port_id);
    }
    if (check_capa(dev_info.rx_offload_capa, RTE_ETH_RX_OFFLOAD_CHECKSUM)) {
        dev_conf.rxmode.offloads |= RTE_ETH_RX_OFFLOAD_CHECKSUM;
    } else {
        put::throw_runtime_error("Failed to initialize DPDK port {}. No "
                                 "support for Rx checksum offload",
                                 cfg.port_id);
    }
    if (check_capa(dev_info.tx_offload_capa, RTE_ETH_TX_OFFLOAD_IPV4_CKSUM)) {
        dev_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_IPV4_CKSUM;
    } else {
        put::throw_runtime_error("Failed to initialize DPDK port {}. No "
                                 "support for Tx IPv4 checksum offload",
                                 cfg.port_id);
    }
    if (auto capa = RTE_ETH_TX_OFFLOAD_TCP_CKSUM | RTE_ETH_TX_OFFLOAD_UDP_CKSUM;
        check_capa(dev_info.tx_offload_capa, capa)) {
        dev_conf.txmode.offloads |= capa;
    } else {
        put::throw_runtime_error("Failed to initialize DPDK port {}. No "
                                 "support for Tx TCP/UDP checksum offload",
                                 cfg.port_id);
    }
    // An optimization for fast release of mbufs.
    // Possible when all mbufs enqueued to given TX queue come from the same
    // memory pool and have reference count of 1.
    if (check_capa(dev_info.tx_offload_capa,
                   RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)) {
        dev_conf.txmode.offloads |= RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;
    }

    return {dev_info, dev_conf};
}

void eth_dev::configure(const config& cfg,
                        const rte_eth_dev_info& dev_info,
                        const rte_eth_conf& dev_conf)
{
    if (rte_eth_dev_configure(cfg.port_id, nqueues, nqueues, &dev_conf) != 0) {
        put::throw_dpdk_error(
            rte_errno,
            "Failed to initialize DPDK port {}. Failed to configure the device",
            cfg.port_id);
    }
    rte_eth_rxconf rxq_conf = dev_info.default_rxconf;
    rxq_conf.offloads       = dev_conf.rxmode.offloads;
    if (rte_eth_rx_queue_setup(cfg.port_id, queue_id, cfg.queue_size,
                               cfg.socket_id, &rxq_conf, cfg.mempool) != 0) {
        const auto& lim = dev_info.rx_desc_lim;
        put::throw_dpdk_error(
            rte_errno,
            "Failed to initialize DPDK port {}. Failed to setup Rx queue {} "
            "with size {}. HW queue size range {}-{}",
            cfg.port_id, queue_id, cfg.queue_size, lim.nb_min, lim.nb_max);
    }
    rte_eth_txconf txq_conf = dev_info.default_txconf;
    txq_conf.offloads       = dev_conf.txmode.offloads;
    if (rte_eth_tx_queue_setup(cfg.port_id, queue_id, cfg.queue_size,
                               cfg.socket_id, &txq_conf) != 0) {
        const auto& lim = dev_info.tx_desc_lim;
        put::throw_dpdk_error(
            rte_errno,
            "Failed to initialize DPDK port {}. Failed to setup Tx queue {} "
            "with size {}. HW queue size range {}-{}",
            cfg.port_id, queue_id, cfg.queue_size, lim.nb_min, lim.nb_max);
    }
}

void eth_dev::start(const config& cfg)
{
    // Total time waiting 10'000'000 microseconds
    constexpr uint32_t check_cnt = 100;
    constexpr uint32_t sleep_us  = 100'000;

    if (rte_eth_dev_start(cfg.port_id) < 0) {
        put::throw_dpdk_error(rte_errno, "Failed to start DPDK port {}",
                              cfg.port_id);
    }
    // Wait for the main port to become active
    rte_eth_link link = {};
    for (auto i = 0u; (i < check_cnt) && !link.link_status; ++i) {
        usleep(sleep_us);
        rte_eth_link_get_nowait(cfg.port_id, &link);
    }
    if (!link.link_status) {
        rte_eth_dev_stop(cfg.port_id);
        put::throw_runtime_error("Failed to bring up DPDK port {}",
                                 cfg.port_id);
    }
}

} // namespace gen::priv
