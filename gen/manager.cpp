#include "gen/manager.h"
#include "gen/priv/eth_dev.h"
#include "gen/priv/flows_generator.h"
#include "gen/priv/mbuf_pool.h"
#include "gen/priv/time_event_scheduler.h"

#include "log/log.h"
#include "mgmt/gen_config.h"
#include "mgmt/messages.h"
#include "mgmt/stats.h"
#include "put/tg_assert.h"
#include "put/time_utils.h"

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

    // The event scheduler needs to be destroyed after the generators because
    // the latter use the timer subsystem.
    gen::priv::time_event_scheduler time_events_;

    using flows_generator_type     = gen::priv::flows_generator;
    using flows_generator_ptr_type = std::unique_ptr<flows_generator_type>;
    std::vector<flows_generator_ptr_type> generators_;

    struct gen_ticks
    {
        uint64_t begin;
        uint64_t duration;
    };
    std::optional<const gen_ticks> gen_ticks_;

    std::vector<rte_mbuf*> tx_pkts_;

    uint64_t cnt_tx_pkts_qfull_ = 0;

    const stdfs::path working_dir_;

public:
    manager_impl(const config_type&);

    void process_events() noexcept;

private:
    void on_inc_msg(mgmt::req_start_generation&&) noexcept;
    void on_inc_msg(mgmt::req_stop_generation&&) noexcept;

    void stop_generation() noexcept;
    bool generation_started() const noexcept { return !!gen_ticks_; }

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
        TG_ENFORCE(time_events_.count_events() == 0);
        return;
    }

    // The generation timeout is tracked explicitly and not through a registered
    // timer because we can't do some actions with the timer system from inside
    // a timer callback.
    if ((rte_get_tsc_cycles() - gen_ticks_->begin) >= gen_ticks_->duration) {
        stop_generation();
        if (!tx_pkts_.empty()) transmit_tx_pkts();
        return;
    }

    // This call may generate packets for transmission and statistics
    time_events_.process_events();

    // We need to transmit the packets which have been enqueued for sending
    // during this cycle of `process_events`. We need to keep the latency as
    // small as possible.
    if (!tx_pkts_.empty()) transmit_tx_pkts();
}

void manager_impl::on_inc_msg(mgmt::req_start_generation&& msg) noexcept
{
    TGLOG_INFO("Got request to start generation for {} with {} capture files\n",
               msg.cfg->duration(), msg.cfg->flows_configs().size());
    if (generation_started()) {
        TGLOG_INFO("Generation already started\n");
        out_queue_->enqueue(
            mgmt::res_start_generation{.res = "Already started"});
        return;
    }

    std::vector<flows_generator_ptr_type> gens;
    gens.reserve(msg.cfg->flows_configs().size());
    try {
        for (const auto& cap_cfg : msg.cfg->flows_configs()) {
            gens.push_back(std::make_unique<flows_generator_type>(
                cap_cfg, working_dir_, msg.cfg->dut_address(), pool_.pool(),
                &time_events_));
        }
    } catch (const std::exception& ex) {
        TGLOG_INFO("Failed to create flows generator: {}\n", ex.what());
        out_queue_->enqueue(mgmt::res_start_generation{.res = ex.what()});
        return;
    }
    TG_ENFORCE(generators_.empty());
    generators_ = std::move(gens);

    // Every generation run should report summary stats only from its own run
    if (const int err = rte_eth_stats_reset(nic_port_id); err == 0) {
        cnt_tx_pkts_qfull_ = 0;
    } else {
        TGLOG_ERROR("Failed to reset the ethernet device stats: ({}) {}\n",
                    -err, ::strerrordesc_np(-err));
    }

    TG_ENFORCE(!gen_ticks_);
    gen_ticks_.emplace(gen_ticks{
        .begin    = rte_get_tsc_cycles(),
        .duration = put::duration_to_tsc(msg.cfg->duration()),
    });

    TGLOG_INFO("Generation started with {} flows generators\n",
               generators_.size());

    out_queue_->enqueue(mgmt::res_start_generation{});
}

void manager_impl::on_inc_msg(mgmt::req_stop_generation&&) noexcept
{
    // The generation stop request is always fully processed so that the
    // summary stats can be reported via the response.
    TGLOG_INFO("Got request to stop generation\n");

    stop_generation();

    rte_eth_stats tmp = {};
    rte_eth_stats_get(nic_port_id, &tmp);

    out_queue_->enqueue(
        mgmt::res_stop_generation{.res = mgmt::summary_stats{
                                      .cnt_rx_pkts_        = tmp.ipackets,
                                      .cnt_tx_pkts_        = tmp.opackets,
                                      .cnt_rx_bytes_       = tmp.ibytes,
                                      .cnt_tx_bytes_       = tmp.obytes,
                                      .cnt_rx_pkts_qfull_  = tmp.imissed,
                                      .cnt_rx_pkts_nombuf_ = tmp.rx_nombuf,
                                      .cnt_tx_pkts_qfull_  = cnt_tx_pkts_qfull_,
                                      .cnt_rx_pkts_err_    = tmp.ierrors,
                                      .cnt_tx_pkts_err_    = tmp.oerrors,
                                  }});
}

void manager_impl::stop_generation() noexcept
{
    generators_.clear();

    // Removing the generators should automatically remove all registered timers
    // because every generator should unregister its timers upon destruction.
    TG_ENFORCE(time_events_.count_events() == 0);

    gen_ticks_.reset();

    TGLOG_INFO("Generation stopped\n");
}

////////////////////////////////////////////////////////////////////////////////

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
        cnt_tx_pkts_qfull_ += cnt_drop;
        TGLOG_ERROR("Dropped {} of {} on transmit\n", cnt_drop, cnt_all);
    }
    tx_pkts_.clear();
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
