#pragma once

namespace gen::priv
{

// These operations are used during the generation from the flows generator
// functionality. As a general rule virtual calls are slow and should be avoided
// if possible. However, I think we won't see performance impact for our usage
// because there is only single instance of the implementation of these ops and
// thus the vtable pointer and the function pointers will always be resolved to
// the same address and should be hot in the cache.
// This interface also decouples the things in the `gen` module and reduces
// the configuration parameters passed to the flows generator instances.
class generation_ops
{
public:
    virtual ~generation_ops() noexcept = default;

    virtual rte_mbuf* alloc_mbuf() noexcept        = 0;
    virtual rte_mbuf* copy_pkt(rte_mbuf*) noexcept = 0;
    virtual void send_pkt(rte_mbuf*) noexcept      = 0;
};

} // namespace gen::priv
