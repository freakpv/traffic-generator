#pragma once

#include "put/throw.h"

namespace dpdk
{
// Single-producer-single-consumer ring
template <typename T>
    requires std::is_trivially_constructible_v<T> &&
             std::is_trivially_copyable_v<T> && ((sizeof(T) % 4u) == 0)
class spsc_ring
{
public:
    using elem_type           = T;
    using elem_destroyer_type = void (*)(elem_type*);

    static constexpr uint32_t elem_size = sizeof(T);

private:
    using ring_destroyer_type = void (*)(rte_ring*);
    using impl_type           = std::unique_ptr<rte_ring, ring_destroyer_type>;

    impl_type impl_;
    elem_destroyer_type elem_destroyer_;

    // Needed when an existing ring is opened during construction
    static void no_ring_deleter(rte_ring*) noexcept {}

public:
    spsc_ring(const spsc_ring&)            = delete;
    spsc_ring& operator=(const spsc_ring&) = delete;

    spsc_ring() noexcept
    : impl_(nullptr, &no_ring_deleter), elem_destroyer_(nullptr)
    {
    }

    ~spsc_ring() noexcept
    {
        if (impl_) destroy_elems();
    }

    spsc_ring(spsc_ring&& rhs) noexcept
    : impl_(std::move(rhs.impl_))
    , elem_destroyer_(std::exchange(rhs.elem_destroyer_, nullptr))
    {
    }

    spsc_ring& operator=(spsc_ring&& rhs) noexcept
    {
        if (&rhs != this) {
            if (impl_) destroy_elems();
            impl_           = std::move(rhs.impl_);
            elem_destroyer_ = std::exchange(rhs.elem_destroyer_, nullptr);
        }
        return *this;
    }

    // The constructor creates new ring
    spsc_ring(const char* name,
              size_t capacity,
              uint32_t cpu_socket_id,
              elem_destroyer_type elem_destroyer)
    : impl_(rte_ring_create_elem(name,
                                 elem_size,
                                 capacity,
                                 cpu_socket_id,
                                 RING_F_SP_ENQ | RING_F_SC_DEQ),
            &rte_ring_free)
    , elem_destroyer_(elem_destroyer)
    {
        if (!impl_) {
            put::throw_dpdk_error(
                rte_errno,
                "Failed to create single-producer-single-consumer ring with "
                "name:{} and capacity:{}",
                name, capacity);
        }
    }

    // The constructor lookup for an existing ring.
    explicit spsc_ring(const char* name)
    : impl_(rte_ring_lookup(name), &no_deleter), elem_destroyer_(nullptr)
    {
        if (!impl_) {
            put::throw_dpdk_error(
                rte_errno,
                "Failed to find single-producer-single-consumer ring with "
                "name:{}",
                name);
        }
    }

    // Returns true if the element has been enqueued. False otherwise.
    bool enqueue(const eleme_type& elem) noexcept
    {
        return (rte_ring_sp_enqueue_elem(impl_.get(), &elem, elem_size) == 0);
    }

    // Returns the number of enqueued items. Could be less than the requested.
    size_t enqueue(std::span<const elem_type> from) noexcept
    {
        return rte_ring_sp_enqueue_burst_elem(
            impl_.get(), static_cast<const void*>(from.data()), elem_size,
            from.size(), nullptr);
    }

    // Returns the number of dequeued items. Could be 0.
    size_t dequeue(std::span<elem_type> to) noexcept
    {
        return rte_ring_sc_dequeue_burst_elem(impl_.get(),
                                              static_cast<void*>(to.data()),
                                              elem_size, to.size(), nullptr);
    }

    uint32_t cnt_used_slots() const noexcept
    {
        return rte_ring_count(impl_.get());
    }
    uint32_t cnt_free_slots() const noexcept
    {
        // same as capacity - used_slots
        return rte_ring_free_count(impl_.get());
    }
    uint32_t capacity() const noexcept
    {
        return rte_ring_get_capacity(impl_.get());
    }

private:
    void destroy_elems() noexcept
    {
        for (elem_type elems[8];;) {
            const size_t cnte = dequeue(elems);
            if (cnte == 0) break;
            for (size_t i = 0; i < cnte; ++i) elem_destroyer_(&elems[i]);
        }
    }
};

} // namespace dpdk
