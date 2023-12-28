#include "gen/manager.h"
#include "gen/priv/error.h"
#include "gen/priv/eth_dev.h"
#include "gen/priv/mbuf_pool.h"
#include "gen/priv/timer.h"

#include "log/log.h"
#include "mgmt/gen_config.h"
#include "mgmt/messages.h"

namespace gen // generator
{

class manager_impl
{
    using config_type = manager::config;

    // Currently we always work with single port only and the ports start from 0
    static constexpr uint16_t nic_port_id  = 0;
    static constexpr size_t cnt_burst_pkts = 64;

    gen::priv::mbuf_pool pool_;
    gen::priv::eth_dev if_;
    mgmt::out_messages_queue* inc_queue_;
    mgmt::inc_messages_queue* out_queue_;

    gen::priv::timer_subsystem timer_subsys_;

    // The time point, in TSC cycles, at which the current test should stop.
    std::optional<uint64_t> ticks_end_;

    std::vector<rte_mbuf*> tx_pkts_;

    const stdfs::path working_dir_;

public:
    manager_impl(const config_type&);

    void process_events() noexcept;

private:
    void on_inc_msg(mgmt::req_start_generation&&) noexcept;
    void on_inc_msg(mgmt::req_stop_generation&&) noexcept;

private:
    void send_pkt(rte_mbuf*) noexcept;
    void transmit_tx_pkts() noexcept;
    void receive_rx_pkts() noexcept;
};

////////////////////////////////////////////////////////////////////////////////

manager_impl::manager_impl(const config_type& cfg)
: pool_({.cnt_mbufs = cfg.max_cnt_mbufs, .socket_id = rte_socket_id()})
, if_({.port_id    = nic_port_id,
       .queue_size = cfg.nic_queue_size,
       .socket_id  = rte_socket_id(),
       .mempool    = pool_.pool()})
, inc_queue_(cfg.inc_queue)
, out_queue_(cfg.out_queue)
, working_dir_(cfg.working_dir)
{
    tx_pkts_.reserve(cnt_burst_pkts);
    TGLOG_INFO("Constructed the generation manager with working dir: {}\n",
               working_dir_);
}

void manager_impl::process_events() noexcept
{
    // TODO: Stop the generation if expired

    timer_subsys_.process_events();

    // We need to transmit the packets which have been enqueued for sending
    // during this cycle of `process_events`. We need to keep the latency as
    // small as possible.
    if (!tx_pkts_.empty()) transmit_tx_pkts();

    receive_rx_pkts();

    inc_queue_->dequeue([this](auto&& msg) { on_inc_msg(std::move(msg)); });
}

void manager_impl::on_inc_msg(mgmt::req_start_generation&& msg) noexcept
{
    TGLOG_INFO("Got request to start generation for {} with {} capture files\n",
               msg.cfg->duration(), msg.cfg->cap_configs().size());
    if (ticks_end_) {}
    // TODO: If it's already started report error
    // TODO:
    out_queue_->enqueue(mgmt::res_start_generation{});
}

void manager_impl::on_inc_msg(mgmt::req_stop_generation&&) noexcept
{
    TGLOG_INFO("Got request to stop generation\n");
    if (!ticks_end_) {
        TGLOG_INFO("Generation already stopped\n");
        out_queue_->enqueue(mgmt::res_stop_generation{
            .res = gen::priv::error::already_stopped});
        return;
    }
    // TODO:
    // - Stop and cleanup/reset everything needed
    // - Send summary stats
    out_queue_->enqueue(mgmt::res_stop_generation{});
}

void manager_impl::send_pkt(rte_mbuf* mbuf) noexcept
{
    tx_pkts_.push_back(mbuf);
    if (tx_pkts_.size() == cnt_burst_pkts) transmit_tx_pkts();
}

void manager_impl::transmit_tx_pkts() noexcept
{
    const auto cnt = if_.transmit_pkts(tx_pkts_);
    if (const auto cnt_all = tx_pkts_.size(); cnt_all > cnt) {
        const auto cnt_drop = cnt_all - cnt;
        rte_pktmbuf_free_bulk(&tx_pkts_[cnt], cnt_drop);
        // TODO: Stats counter
        TGLOG_ERROR("Dropped {} of {} on transmit\n", cnt_drop, cnt_all);
    }
}

void manager_impl::receive_rx_pkts() noexcept
{
    rte_mbuf* pkts[cnt_burst_pkts];
    if (const auto cnt = if_.receive_pkts(pkts); cnt > 0) {
        rte_pktmbuf_free_bulk(pkts, cnt);
    }
}

////////////////////////////////////////////////////////////////////////////////

manager::manager(const config& cfg) : impl_(std::make_unique<manager_impl>(cfg))
{
}

manager::~manager() noexcept = default;

void manager::process_events() noexcept
{
    impl_->process_events();
}

} // namespace gen
