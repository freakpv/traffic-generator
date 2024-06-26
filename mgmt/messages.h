#pragma once

#include "mgmt/gen_config.h"
#include "mgmt/stats.h"
#include "put/spsc_ring.h"
#include "put/visit.h"

namespace mgmt
{

struct req_start_generation
{
    std::unique_ptr<gen_config> cfg;
};

struct res_start_generation
{
    bout::result<void, std::string> res = bout::success();
};

struct req_stop_generation
{
};

struct res_stop_generation
{
    mgmt::summary_stats res = {};
};

struct req_stats_report
{
};

struct res_stats_report
{
    mgmt::stats res = {};
};

////////////////////////////////////////////////////////////////////////////////

template <size_t Capacity, typename... Msgs>
class messages_queue
{
    using msg_type  = std::variant<Msgs...>;
    using impl_type = put::spsc_ring<msg_type, Capacity>;

    impl_type impl_;

public:
    messages_queue() noexcept  = default;
    ~messages_queue() noexcept = default;

    messages_queue(messages_queue&&)                 = delete;
    messages_queue(const messages_queue&)            = delete;
    messages_queue& operator=(messages_queue&&)      = delete;
    messages_queue& operator=(const messages_queue&) = delete;

    template <typename Msg>
    bool enqueue(Msg&& msg) noexcept
    {
        return impl_.try_push(std::forward<Msg>(msg));
    }

    template <typename... Funs>
    void dequeue(Funs&&... funs) noexcept
    {
        std::array<msg_type, std::min(Capacity, 8uz)> msgs;
        const auto cnt = impl_.try_pop(msgs.data(), msgs.size());
        for (auto&& msg : std::span(msgs.data(), cnt)) {
            put::visit(std::move(msg), std::forward<Funs>(funs)...);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct out_messages_queue
: messages_queue<32,
                 req_start_generation,
                 req_stop_generation,
                 req_stats_report>
{
};

struct inc_messages_queue
: messages_queue<256,
                 res_start_generation,
                 res_stop_generation,
                 res_stats_report,
                 generation_report>
{
};

} // namespace mgmt
