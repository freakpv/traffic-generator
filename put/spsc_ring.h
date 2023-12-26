#pragma once

namespace put
{
// This implementation of SPSC ring is used instead of DPDK rte_ring because
// the latter can't work with C++ object model for non-trivially copyable types.
// The main idea for this comes from: https://rigtorp.se/ringbuffer/
// This class was created because
// 1. boost::lockfree::spsc_queue doesn't have support for move operations and
// thus it doesn't support move only types.
// 2. boost::lockfree::spsc_queue support sizes which are not power of 2 and
// thus it's implementation requires additional branching to handle the wrapping
// 3. boost::lockfree::spsc_queue doesn't cache the head/tail variables and thus
// most calls access both the producer and the consumer atomic variables and
// create additional traffic between the CPU caches.
// In general, this implementation is between 2 and 3 times faster than
// boost::lockfree::spsc_queue with integer payload and static capacity.
// Here are some benchmarks with perf:
/* boost::lockfree::spsc_queue<int, 16 * 1024>
     926779558      branches
        842231      cache-misses              #    0.512 % of all cache refs
     164515478      cache-references
    6710883701      cycles
    3699190748      instructions              #    0.55  insn per cycle

   1.526478300 seconds time elapsed
   2.904214000 seconds user
   0.140203000 seconds sys
*/
/* put::spsc_ring<int, 16 * 1024>
     603995422      branches
        317309      cache-misses              #    0.960 % of all cache refs
      33064612      cache-references
    2334265593      cycles
    3079839602      instructions              #    1.32  insn per cycle

   0.531765692 seconds time elapsed
   0.989709000 seconds user
   0.069979000 seconds sys
*/
template <typename T, size_t Size>
class spsc_ring
{
    // The `Size` should be non-zero power of 2 number!
    static_assert((Size > 0) && ((Size & (Size - 1)) == 0));

    using byte_type        = unsigned char;
    using value_type       = T;
    using size_type        = size_t;
    using atomic_size_type = std::atomic<size_t>;
    static_assert(atomic_size_type::is_always_lock_free);

    // std::hardware_destructive_interference_size is still not available in
    // libstdc++ for GCC 11. It seems that it won't be added any time soon.
    // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88466.
    // Thus in order to avoid pointless `#include <new>`, for something that may
    // not appear at all, the constant is hard-coded here.
    static constexpr size_type size            = Size;
    static constexpr size_type size_mask       = Size - 1;
    static constexpr size_type cache_line_size = 64;
    static constexpr size_type buff_align =
        std::max(alignof(value_type), cache_line_size);

    // These variables are used in groups from the producer and consumer
    // threads Thus they are placed at different cache lines. Producer
    // thread variables
    alignas(cache_line_size) atomic_size_type atomic_head_{0};
    size_type prod_tail_{0};
    // Consumer thread variables
    alignas(cache_line_size) atomic_size_type atomic_tail_{0};
    size_type cons_head_{0};
    // The buffer is two times the given size in order to be able to represent
    // the buffer as linear when the `head/tail` position is at the buffer end
    alignas(buff_align) byte_type buff_[size * sizeof(value_type)];

public:
    spsc_ring() noexcept = default;
    ~spsc_ring() noexcept
    {
        // I'm too lazy to write more optimized destructor currently.
        // This should be good enough, performance wise, for 99% of the cases.
        //
        // Don't want to include <tuple> for std::ignore and thus this type.
        struct ignore
        {
            void operator=(T&&) const noexcept {}
        };
        std::array<ignore, std::min(16ul, size)> tmp;
        while (try_pop(tmp.data(), tmp.size()) > 0) {}
    }

    spsc_ring(spsc_ring&&)                 = delete;
    spsc_ring(const spsc_ring&)            = delete;
    spsc_ring& operator=(spsc_ring&&)      = delete;
    spsc_ring& operator=(const spsc_ring&) = delete;

public: // These functions should be called only from the producer thread
    // clang-format off
    template <typename... Args>
    bool try_push(Args&&... args) 
        noexcept(std::is_nothrow_constructible_v<T, Args...>)
        requires(std::is_constructible_v<T, Args...>)
    // clang-format on
    {
        const auto prod_head = atomic_head_.load(std::memory_order_relaxed);
        if (prod_head - prod_tail_ == size) {
            prod_tail_ = atomic_tail_.load(std::memory_order_acquire);
            if (prod_head - prod_tail_ == size) return false;
        }
        auto pos = (prod_head & size_mask) * sizeof(value_type);
        new (&buff_[pos]) value_type(std::forward<Args>(args)...);
        atomic_head_.store(prod_head + 1, std::memory_order_release);
        return true;
    }

    // Don't want to complicate the code with logic for reversion in case of
    // exception and thus the requirements for this function are stronger.
    // Note that the implementation will move from the passed values unless
    // they are passed as const.
    // clang-format off
    struct do_copy_type {};
    struct do_move_type {};
    static constexpr do_copy_type do_copy = {};
    static constexpr do_move_type do_move = {};
    // clang-format on
    template <typename U, typename DoMove = do_move_type>
    size_type try_push(U* from, size_type sz, DoMove = {}) noexcept
        requires((std::is_same_v<DoMove, do_move_type> &&
                  std::is_nothrow_constructible_v<T, U&&>) ||
                 (std::is_same_v<DoMove, do_copy_type> &&
                  std::is_nothrow_constructible_v<T, U&>))
    {
        auto prod_head = atomic_head_.load(std::memory_order_relaxed);
        auto cnt_avail = size - (prod_head - prod_tail_);
        if (cnt_avail < sz) {
            prod_tail_ = atomic_tail_.load(std::memory_order_acquire);
            cnt_avail  = size - (prod_head - prod_tail_);
            if (cnt_avail == 0) return 0;
        }
        sz = std::min(sz, cnt_avail);
        // This loop can be optimized with 1-2 memcpy for trivially copyable
        // types. However, this will complicate the code here because we need
        // to take into account when the tail wraps and starts from the begin.
        // Thus, for now this optimization is not implemented.
        for (auto end = from + sz; from != end; ++prod_head, ++from) {
            auto pos = (prod_head & size_mask) * sizeof(value_type);
            if constexpr (std::is_same_v<DoMove, do_move_type>) {
                new (&buff_[pos]) value_type(std::move(*from));
            } else {
                new (&buff_[pos]) value_type(*from);
            }
        }
        atomic_head_.store(prod_head, std::memory_order_release);
        return sz;
    }

public: // These functions should be called only from the consumer thread
    // clang-format off
    template <typename Fn>
    bool try_consume(Fn&& fn)
        noexcept(std::is_nothrow_invocable_v<Fn, value_type&&>)
        requires(std::is_invocable_r_v<bool, Fn, value_type&&>)
    // clang-format on
    {
        const auto cons_tail = atomic_tail_.load(std::memory_order_relaxed);
        if (cons_head_ - cons_tail == 0) {
            cons_head_ = atomic_head_.load(std::memory_order_acquire);
            if (cons_head_ - cons_tail == 0) return false;
        }
        auto pos  = (cons_tail & size_mask) * sizeof(value_type);
        auto* val = std::launder(reinterpret_cast<value_type*>(&buff_[pos]));
        if (!std::forward<Fn>(fn)(std::move(*val))) return false;
        // The compiler should remove this destructor if the type is
        // trivially destructible. Haven't looked at the assembly though.
        val->~value_type();
        atomic_tail_.store(cons_tail + 1, std::memory_order_release);
        return true;
    }
    // clang-format off
    template <typename Fn>
    size_type try_consume(Fn&& fn)
        noexcept(std::is_nothrow_invocable_v<Fn, value_type*, size_type>)
        requires(std::is_invocable_r_v<size_type, Fn, value_type*, size_type>)
    // clang-format on
    {
        auto cons_tail = atomic_tail_.load(std::memory_order_relaxed);
        auto cnt_avail = cons_head_ - cons_tail;
        if (cnt_avail == 0) {
            cons_head_ = atomic_head_.load(std::memory_order_acquire);
            cnt_avail  = cons_head_ - cons_tail;
            if (cnt_avail == 0) return 0;
        }

        // This should be removed by the compiler if the type is trivial.
        auto destroy_consumed = [](value_type* v, size_type s) {
            for (auto end = v + s; v != end; ++v) v->~value_type();
        };

        size_type consumed = 0;
        const auto beg     = cons_tail & size_mask;
        const auto end     = (cons_tail + cnt_avail) & size_mask;
        auto pos           = beg * sizeof(value_type);
        auto* v = std::launder(reinterpret_cast<value_type*>(&buff_[pos]));
        if (end > beg) {
            // No wrap-around means that we have only single range of entries
            consumed += std::forward<Fn>(fn)(v, cnt_avail);
            destroy_consumed(v, consumed);
        } else {
            auto cnt = size - beg;
            consumed += std::forward<Fn>(fn)(v, cnt);
            destroy_consumed(v, consumed);
            if (cnt == consumed) {
                v   = std::launder(reinterpret_cast<value_type*>(buff_));
                cnt = std::forward<Fn>(fn)(v, end);
                destroy_consumed(v, cnt);
                consumed += cnt;
            }
        }

        atomic_tail_.store(cons_tail + consumed, std::memory_order_release);
        return consumed;
    }

    // clang-format off
    template <typename U>
    bool try_pop(U& to) 
        noexcept(std::is_nothrow_assignable_v<U&, T&&>)
        requires(std::is_assignable_v<U&, T&&>)
    // clang-format on
    {
        return try_consume(
            [&to](T&& v) noexcept(std::is_nothrow_assignable_v<U&, T&&>) {
                to = std::move(v);
                return true;
            });
    }

    // Don't want to complicate the code with logic for reversion in case of
    // exception and thus the requirements for this function are stronger.
    template <typename U>
    size_type try_pop(U* to, size_type sz) noexcept
        requires(std::is_nothrow_assignable_v<U&, T &&>)
    {
        return try_consume(
            [to, sz](value_type* v, size_type s) mutable noexcept {
                const auto cnt = std::min(sz, s);
                using dst_type = std::remove_cvref_t<U>;
                if constexpr (std::is_trivially_copyable_v<value_type> &&
                              std::is_same_v<value_type, dst_type>) {
                    ::memcpy(to, v, cnt * sizeof(value_type));
                    to += cnt;
                } else {
                    for (auto end = to + cnt; to != end; ++to, ++v) {
                        *to = std::move(*v);
                    }
                }
                sz -= cnt;
                return cnt;
            });
    }

public: // These functions can be called from any thread
    size_type count_pop_slots() const noexcept
    {
        const auto prod_head = atomic_head_.load(std::memory_order_acquire);
        const auto cons_tail = atomic_tail_.load(std::memory_order_acquire);
        return (prod_head - cons_tail);
    }
    size_type count_push_slots() const noexcept
    {
        return (size - count_pop_slots());
    }
    size_type capacity() const noexcept { return size; }
};

} // namespace put
