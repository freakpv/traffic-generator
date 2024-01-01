#include "gen/manager.h"
#include "gen/priv/eth_dev.h"
#include "gen/priv/flows_generator.h"
#include "gen/priv/generation_ops.h"
#include "gen/priv/mbuf_pool.h"
#include "gen/priv/event_scheduler.h"

#include "log/tg_log.h"
#include "mgmt/gen_config.h"
#include "mgmt/messages.h"
#include "mgmt/stats.h"
#include "put/tg_assert.h"
#include "put/time_utils.h"

namespace gen // generator
{

class manager_impl final : public gen::priv::generation_ops
{
    using config_type = manager::config;

    // Currently we always work with single port only and the ports start from 0
    static constexpr uint16_t nic_port_id  = 0;
    static constexpr size_t cnt_burst_pkts = 64;

    gen::priv::mbuf_pool mbuf_pool_;
    gen::priv::eth_dev eth_dev_;
    mgmt::out_messages_queue* inc_queue_;
    mgmt::inc_messages_queue* out_queue_;

    // The event scheduler needs to be destroyed after the generators because
    // the latter use the timer subsystem.
    gen::priv::event_scheduler scheduler_;

    using flows_generator_type = gen::priv::flows_generator;
    std::vector<flows_generator_type> generators_;

    struct gen_cycles
    {
        put::cycles begin;
        put::cycles duration;
    };
    std::optional<const gen_cycles> gen_cycles_;

    std::vector<rte_mbuf*> tx_pkts_;

    uint64_t cnt_tx_pkts_qfull_  = 0;
    uint64_t cnt_tx_pkts_nombuf_ = 0;

    const stdfs::path working_dir_;

public:
    manager_impl(const config_type&);
    ~manager_impl() noexcept override = default;

    void process_events() noexcept;

private:
    void on_inc_msg(mgmt::req_start_generation&&) noexcept;
    void on_inc_msg(mgmt::req_stop_generation&&) noexcept;

    void stop_generation() noexcept;
    bool generation_started() const noexcept { return !!gen_cycles_; }

private:
    void transmit_tx_pkts() noexcept;
    void receive_rx_pkts() noexcept;

private: // The `generation_ops` interface
    rte_mbuf* alloc_mbuf() noexcept override;
    rte_mbuf* copy_pkt(const rte_mbuf*) noexcept override;
    void send_pkt(rte_mbuf*) noexcept override;
    gen::priv::event_handle create_scheduler_event() noexcept override;
    void do_report(const gen::priv::generation_report&) noexcept override;
};

////////////////////////////////////////////////////////////////////////////////

manager_impl::manager_impl(const config_type& cfg)
: mbuf_pool_({.cnt_mbufs = cfg.max_cnt_mbufs, .socket_id = rte_socket_id()})
, eth_dev_({.port_id    = nic_port_id,
            .queue_size = cfg.nic_queue_size,
            .socket_id  = rte_socket_id(),
            .mempool    = mbuf_pool_.pool()})
, inc_queue_(cfg.inc_queue)
, out_queue_(cfg.out_queue)
, working_dir_(cfg.working_dir)
{
    tx_pkts_.reserve(cnt_burst_pkts);
    TG_LOG_INFO("Constructed the generation manager with working dir: {}\n",
                working_dir_);
}

void manager_impl::process_events() noexcept
{
    // We need to receive management messages even if the generation is not
    // started.
    inc_queue_->dequeue([this](auto&& msg) { on_inc_msg(std::move(msg)); });

    // There could be packets which we may need to receive and throw away just
    // to keep the queue empty.
    receive_rx_pkts();

    // If the generation is not started no functionality should need timers and
    // as a result no functionality should generate packets for transmission.
    if (!generation_started()) {
        TG_ENFORCE(tx_pkts_.empty() == true);
        TG_ENFORCE(scheduler_.count_events() == 0);
        return;
    }

    // The generation timeout is tracked explicitly and not through a registered
    // timer because we can't do some actions with the timer system from inside
    // a timer callback.
    if ((put::cycles::current() - gen_cycles_->begin) > gen_cycles_->duration) {
        stop_generation();
        if (!tx_pkts_.empty()) transmit_tx_pkts();
        return;
    }

    // This call may generate packets for transmission and statistics
    scheduler_.process_events();

    // We need to transmit the packets which have been enqueued for sending
    // during this cycle of `process_events`. We need to keep the latency as
    // small as possible.
    if (!tx_pkts_.empty()) transmit_tx_pkts();
}

void manager_impl::on_inc_msg(mgmt::req_start_generation&& msg) noexcept
{
    TG_LOG_INFO(
        "Got request to start generation for {} with {} capture files\n",
        msg.cfg->duration(), msg.cfg->flows_configs().size());
    if (generation_started()) {
        TG_LOG_INFO("Generation already started\n");
        out_queue_->enqueue(
            mgmt::res_start_generation{.res = "Already started"});
        return;
    }

    std::vector<flows_generator_type> gens;
    gens.reserve(msg.cfg->flows_configs().size());
    try {
        const auto cln_ether_addr = eth_dev_.get_mac_addr();
        for (size_t idx = 0; const auto& cap_cfg : msg.cfg->flows_configs()) {
            gens.emplace_back(flows_generator_type::config{
                .idx            = idx++,
                .cap_fpath      = working_dir_ / cap_cfg.name,
                .cln_ether_addr = cln_ether_addr,
                .srv_ether_addr = msg.cfg->dut_address(),
                .burst          = cap_cfg.burst,
                .flows_per_sec  = cap_cfg.flows_per_sec,
                .inter_pkts_gap = cap_cfg.inter_pkts_gap,
                .cln_ip_addrs   = cap_cfg.cln_ips,
                .srv_ip_addrs   = cap_cfg.srv_ips,
                .cln_port       = cap_cfg.cln_port,
                .gen_ops        = this,
            });
        }
    } catch (const std::exception& ex) {
        TG_LOG_INFO("Failed to create flows generator: {}\n", ex.what());
        out_queue_->enqueue(mgmt::res_start_generation{.res = ex.what()});
        return;
    }

    // TODO: Debug log the prepared flows.

    TG_ENFORCE(generators_.empty());
    generators_ = std::move(gens);

    // Every generation run should report summary stats only from its own run
    if (const int err = rte_eth_stats_reset(nic_port_id); err == 0) {
        cnt_tx_pkts_qfull_  = 0;
        cnt_tx_pkts_nombuf_ = 0;
    } else {
        TG_LOG_ERROR("Failed to reset the ethernet device stats: ({}) {}\n",
                     -err, ::strerrordesc_np(-err));
    }

    TG_ENFORCE(!gen_cycles_);
    gen_cycles_.emplace(gen_cycles{
        .begin    = put::cycles::current(),
        .duration = put::cycles::from_duration(msg.cfg->duration()),
    });

    TG_LOG_INFO("Generation started with {} flows generators\n",
                generators_.size());

    out_queue_->enqueue(mgmt::res_start_generation{});
}

void manager_impl::on_inc_msg(mgmt::req_stop_generation&&) noexcept
{
    // The generation stop request is always fully processed so that the
    // summary stats can be reported via the response.
    TG_LOG_INFO("Got request to stop generation\n");

    stop_generation();

    rte_eth_stats tmp = {};
    rte_eth_stats_get(nic_port_id, &tmp);

    out_queue_->enqueue(
        mgmt::res_stop_generation{.res = mgmt::summary_stats{
                                      .cnt_rx_pkts        = tmp.ipackets,
                                      .cnt_tx_pkts        = tmp.opackets,
                                      .cnt_rx_bytes       = tmp.ibytes,
                                      .cnt_tx_bytes       = tmp.obytes,
                                      .cnt_rx_pkts_qfull  = tmp.imissed,
                                      .cnt_rx_pkts_nombuf = tmp.rx_nombuf,
                                      .cnt_tx_pkts_qfull  = cnt_tx_pkts_qfull_,
                                      .cnt_tx_pkts_nombuf = cnt_tx_pkts_nombuf_,
                                      .cnt_rx_pkts_err    = tmp.ierrors,
                                      .cnt_tx_pkts_err    = tmp.oerrors,
                                  }});
}

void manager_impl::stop_generation() noexcept
{
    generators_.clear();

    // Removing the generators should automatically remove all registered timers
    // because every generator should unregister its timers upon destruction.
    TG_ENFORCE(scheduler_.count_events() == 0);

    gen_cycles_.reset();

    TG_LOG_INFO("Generation stopped\n");
}

////////////////////////////////////////////////////////////////////////////////

void manager_impl::transmit_tx_pkts() noexcept
{
    const auto cnt = eth_dev_.transmit_pkts(tx_pkts_);
    if (const auto cnt_all = tx_pkts_.size(); cnt_all > cnt) {
        const auto cnt_drop = cnt_all - cnt;
        rte_pktmbuf_free_bulk(&tx_pkts_[cnt], cnt_drop);
        cnt_tx_pkts_qfull_ += cnt_drop;
        TG_LOG_ERROR("Dropped {} of {} on transmit\n", cnt_drop, cnt_all);
    }
    tx_pkts_.clear();
}

void manager_impl::receive_rx_pkts() noexcept
{
    rte_mbuf* pkts[cnt_burst_pkts];
    if (const auto cnt = eth_dev_.receive_pkts(pkts); cnt > 0) {
        rte_pktmbuf_free_bulk(pkts, cnt);
    }
}

////////////////////////////////////////////////////////////////////////////////

rte_mbuf* manager_impl::alloc_mbuf() noexcept
{
    rte_mbuf* ret = rte_pktmbuf_alloc(mbuf_pool_.pool());
    if (!ret) ++cnt_tx_pkts_nombuf_;
    return ret;
}

rte_mbuf* manager_impl::copy_pkt(const rte_mbuf* pkt) noexcept
{
    rte_mbuf* ret = rte_pktmbuf_copy(pkt, mbuf_pool_.pool(), 0, pkt->pkt_len);
    if (!ret) ++cnt_tx_pkts_nombuf_;
    return ret;
}

void manager_impl::send_pkt(rte_mbuf* pkt) noexcept
{
    tx_pkts_.push_back(pkt);
    if (tx_pkts_.size() == cnt_burst_pkts) transmit_tx_pkts();
}

gen::priv::event_handle manager_impl::create_scheduler_event() noexcept
{
    return gen::priv::event_handle(&scheduler_);
}

void manager_impl::do_report(const gen::priv::generation_report&) noexcept
{
    // TODO: Send the report to the management system
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
