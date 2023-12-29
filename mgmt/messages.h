#pragma once

#include "mgmt/gen_config.h"
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
    std::optional<std::string> error_desc;
};

struct req_stop_generation
{
};

struct res_stop_generation
{
    // TODO: The summary stats should be added here
    std::optional<std::string> error_desc;
};

struct stats_report
{
    // TODO:
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
: messages_queue<4, req_start_generation, req_stop_generation>
{
};

struct inc_messages_queue
: messages_queue<64, res_start_generation, res_stop_generation, stats_report>
{
};

} // namespace mgmt
