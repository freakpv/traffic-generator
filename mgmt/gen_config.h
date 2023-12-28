#pragma once

namespace mgmt
{

class gen_config
{
public:
    struct cap_cfg
    {
        stdfs::path name;
        uint32_t burst;
        uint32_t streams_per_sec;
        std::optional<stdcr::microseconds> inter_pkts_gap;
        baio_ip_net4 cln_ips;
        baio_ip_net4 srv_ips;
        std::optional<uint16_t> cln_port;
    };

private:
    stdcr::milliseconds duration_;
    rte_ether_addr dut_addr_;
    std::vector<cap_cfg> cap_cfgs_;

public:
    explicit gen_config(std::string_view);
    ~gen_config() noexcept;

    // Intentionally `noexcept` even if these are copy operation and may fail
    gen_config(const gen_config&) noexcept;
    gen_config& operator=(const gen_config&) noexcept;

    gen_config()                        = delete;
    gen_config(gen_config&&)            = delete;
    gen_config& operator=(gen_config&&) = delete;

    rte_ether_addr dut_address() const noexcept { return dut_addr_; }
    stdcr::milliseconds duration() const noexcept { return duration_; }
    std::span<const cap_cfg> cap_configs() const noexcept { return cap_cfgs_; }
};

} // namespace mgmt
