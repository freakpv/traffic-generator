#pragma once

namespace gen::priv // generator
{

class eth_dev
{
    static constexpr uint16_t invalid_port_id = UINT16_MAX;

    uint16_t port_id_ = invalid_port_id;

public:
    struct config
    {
        uint16_t port_id;
        uint16_t queue_size;
        uint32_t socket_id;
        rte_mempool* mempool;
    };

public:
    explicit eth_dev(config);
    ~eth_dev() noexcept;

    eth_dev(eth_dev&&) noexcept;
    eth_dev& operator=(eth_dev&&) noexcept;

    eth_dev()                          = delete;
    eth_dev(const eth_dev&)            = delete;
    eth_dev& operator=(const eth_dev&) = delete;

    rte_eth_link get_link_info() const noexcept;
    rte_ether_addr get_mac_addr() const noexcept;
    rte_eth_dev_info get_dev_info() const noexcept;

    uint16_t port_id() const noexcept { return port_id_; }
    bool is_valid() const noexcept { return (port_id_ != invalid_port_id); }

private:
    std::pair<rte_eth_dev_info, rte_eth_conf> set_capabilities(const config&);
    void
    configure(const config& cfg, const rte_eth_dev_info&, const rte_eth_conf&);
    void start(const config&);
};

} // namespace gen::priv
