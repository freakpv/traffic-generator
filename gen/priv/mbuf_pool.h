#pragma once

namespace gen::priv
{

class mbuf_pool
{
    struct mempool_free
    {
        void operator()(rte_mempool* p) const noexcept { rte_mempool_free(p); }
    };
    std::unique_ptr<rte_mempool, mempool_free> pool_;

public:
    struct config
    {
        size_t cnt_mbufs;
        uint32_t socket_id;
    };

public:
    explicit mbuf_pool(config);

    ~mbuf_pool() noexcept;

    mbuf_pool(mbuf_pool&&) noexcept;
    mbuf_pool& operator=(mbuf_pool&&) noexcept;

    mbuf_pool()                            = delete;
    mbuf_pool(const mbuf_pool&)            = delete;
    mbuf_pool& operator=(const mbuf_pool&) = delete;

    rte_mempool* pool() noexcept { return pool_.get(); }
    bool is_valid() const noexcept { return !!pool_; }
};

} // namespace gen::priv
