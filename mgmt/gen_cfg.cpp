#include "mgmt/gen_cfg.h"
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

namespace mgmt
{

gen_cfg::gen_cfg(std::string_view cfg_info)
{
    bsys::error_code ec;
    bjson::parser parser;
    parser.write(cfg_info);

    const auto json_val   = parser.release();
    const auto& json_obj  = json_val.as_object();
    const auto dur_num    = json_obj.at("duration_secs").as_double();
    const auto& ether_str = json_obj.at("dut_ether_addr").as_string();
    const auto& captures  = json_obj.at("captures").as_array();

    auto load_opt_u64 = [](const auto& json_obj, std::string_view name) {
        if (auto* p = json.at(name).if_uint64(); p) return *p;
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

        // TODO
    }
}

gen_cfg::~gen_cfg() noexcept                         = default;
gen_cfg::gen_cfg(const gen_cfg&) noexcept            = default;
gen_cfg& gen_cfg::operator=(const gen_cfg&) noexcept = default;

} // namespace mgmt
