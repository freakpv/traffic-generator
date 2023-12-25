#include "mgmt/gen_cfg.h"

#include "put/ether_addr.h"
#include "put/num_utils.h"
#include "put/throw.h"

/*
 * The expected format of the given string data is the following.
 * `duration_secs` - is the duration of the whole generation test, in seconds
 * `dut_ether_addr` - is the Ethernet address of the Device Under Test (DUT)
 * `captures` - is an array of different captures which will be used for
 * generating streams of packets.
 * `name` - a path to the capture file. The path is relative to the working
 * directory given in the generator configuration file.
 * `burst` - value 1 means that no burst will be generated, if value > 1 so many
 * streams will be generated in a burst
 * `sps` - started/generated streams per second. Should be 1 or more
 * `ipg` - inter packet gaps in micro-seconds. If not present the time-
 * stamps from the capture file will be used
 * `cln_ips` - range of IPv4 addresses to be used for the "client" packets
 * `srv_ips` - range of IPv4 addresses to be used for the "server" packets
 * `cln_port` = client port to be set to the TCP/UDP packets, If not present the
 * port won't be replaced.
{
    "duration_secs": 10,
    "dut_ether_addr": "e4:8d:8c:20:fb:bc",
    "captures": [
        {
            "name": "test.pcap",
            "burst": 1,
            "sps": 1,
            "ipg": 10000,
            "cln_ips": "16.0.0.1/29",
            "srv_ips": "48.0.0.1/29",
            "cln_port": 1024
        },
        {
            "name": "test2.pcap",
            "burst": 1,
            "sps": 1,
            "ipg": 10000,
            "cln_ips": "16.0.0.1/29",
            "srv_ips": "48.0.0.1/29",
            "cln_port": 1024
        }
        ...
    ]
}
*/
////////////////////////////////////////////////////////////////////////////////

template <>
struct fmt::formatter<bjson::string> : fmt::ostream_formatter
{
};

namespace mgmt
{

gen_cfg::gen_cfg(std::string_view cfg_info)
{
    bjson::parser parser;
    parser.write(cfg_info);

    const auto json_val   = parser.release();
    const auto& json_obj  = json_val.as_object();
    const auto dur_num    = json_obj.at("duration_secs").as_double();
    const auto& ether_str = json_obj.at("dut_ether_addr").as_string();
    const auto& captures  = json_obj.at("captures").as_array();

    const auto dut_addr = put::parse_ether_addr(ether_str);
    if (!dut_addr) {
        put::throw_runtime_error("Invalid `dut_ether_addr`: {}", dut_addr);
    }

    auto load_opt_u64 = [](const auto& json_obj,
                           std::string_view name) -> std::optional<uint64_t> {
        if (auto* p = json_obj.at(name).if_uint64(); p) return *p;
        return std::nullopt;
    };

    std::vector<cap_cfg> cap_cfgs;
    for (const auto& cap : captures) {
        const auto& cap_obj     = cap.as_object();
        const auto& name_str    = cap_obj.at("name").as_string();
        const auto burst_num    = cap_obj.at("burst").as_uint64();
        const auto sps_num      = cap_obj.at("sps").as_uint64();
        const auto ipg_num      = load_opt_u64(cap_obj, "ipg");
        const auto& cln_ips_str = cap_obj.at("cln_ips").as_string();
        const auto& srv_ips_str = cap_obj.at("cln_ips").as_string();
        const auto cln_port_num = load_opt_u64(cap_obj, "cln_port");

        // The limits are kind of arbitrary but there should be some limits
        if (!put::in_range_inclusive(burst_num, 1ul, 5ul)) {
            put::throw_runtime_error(
                "The `burst` value must be between 1 and 5");
        }
        if (!put::in_range_inclusive(sps_num, 1ul, 1'000'000ul)) {
            put::throw_runtime_error("The `streams_per_second (sps)` value "
                                     "must be between 1 and 1'000'000");
        }
        if (ipg_num && !put::in_range_inclusive(*ipg_num, 1ul, 100'000'000ul)) {
            put::throw_runtime_error("The `streams_per_second (sps)` value "
                                     "must be between 1 and 100'000'000");
        }
        if (cln_port_num &&
            !put::in_range_inclusive(*cln_port_num, 1024ul, 65535ul)) {
            put::throw_runtime_error(
                "The `cln_port` value must be between 1024 and 65535");
        }
        bsys::error_code ec;
        const auto cln_ips = baio::ip::make_network_v4(cln_ips_str, ec);
        if (ec) {
            put::throw_runtime_error("Invalid `cln_ips` network: {}",
                                     cln_ips_str);
        }
        const auto srv_ips = baio::ip::make_network_v4(srv_ips_str, ec);
        if (ec) {
            put::throw_runtime_error("Invalid `srv_ips` network: {}",
                                     srv_ips_str);
        }

        using ipg_type = std::optional<stdcr::microseconds>;
        cap_cfgs.push_back(cap_cfg{
            .name            = std::string_view(name_str),
            .burst           = static_cast<uint32_t>(burst_num),
            .streams_per_sec = static_cast<uint32_t>(sps_num),
            .inter_pkts_gap  = ipg_num ? ipg_type(*ipg_num) : ipg_type{},
            .cln_ips         = cln_ips,
            .srv_ips         = srv_ips,
            .cln_port        = cln_port_num,
        });
    }

    duration_ = stdcr::milliseconds(static_cast<uint64_t>(dur_num * 1000));
    dut_addr_ = *dut_addr;
    cap_cfgs_ = std::move(cap_cfgs);
}

gen_cfg::~gen_cfg() noexcept                         = default;
gen_cfg::gen_cfg(const gen_cfg&) noexcept            = default;
gen_cfg& gen_cfg::operator=(const gen_cfg&) noexcept = default;

} // namespace mgmt
