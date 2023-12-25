#pragma once

namespace mgmt
{

class gen_cfg
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
    explicit gen_cfg(std::string_view);
    ~gen_cfg() noexcept;

    // Intentionally `noexcept` even if these are copy operation and may fail
    gen_cfg(const gen_cfg&) noexcept;
    gen_cfg& operator=(const gen_cfg&) noexcept;

    gen_cfg()                     = delete;
    gen_cfg(gen_cfg&&)            = delete;
    gen_cfg& operator=(gen_cfg&&) = delete;

    stdcr::milliseconds duration() const noexcept { return duration_; }
    rte_ether_addr dut_address() const noexcept { return dut_addr_; }
    std::span<const cap_cfg> cap_configs() const noexcept { return cap_cfgs_; }
};

} // namespace mgmt
