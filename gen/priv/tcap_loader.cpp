#include "gen/priv/tcap_loader.h"

#include "put/system_error.h"
#include "put/throw.h"

namespace gen::priv
{

tcap_loader::tcap_loader(const stdfs::path& pcap_path)
{
    decltype(file_) file(::fopen(pcap_path.c_str(), "r"));
    if (!file) {
        put::throw_system_error(errno, "Failed to open PCAP file: {}",
                                pcap_path);
    }

    pcap_file_header hdr = {};
    if (::fread(&hdr, sizeof(hdr), 1, file.get()) != 1u) {
        put::throw_system_error(
            errno, "Failed to read PCAP file header from: {}", pcap_path);
    }

    constexpr uint32_t pcap_magic = 0xA1B2C3D4;
    if ((hdr.magic != pcap_magic) ||
        (hdr.version_major != PCAP_VERSION_MAJOR) ||
        (hdr.version_minor != PCAP_VERSION_MINOR)) {
        put::throw_runtime_error("Read invalid PCAP file header from: {}",
                                 pcap_path);
    }

    file_ = std::move(file);
}

tcap_loader::tcap_loader() noexcept                         = default;
tcap_loader::~tcap_loader() noexcept                        = default;
tcap_loader::tcap_loader(tcap_loader&&) noexcept            = default;
tcap_loader& tcap_loader::operator=(tcap_loader&&) noexcept = default;

bout::result<tcap_loader::pkt>
tcap_loader::load_pkt(put::fun_ref<rte_mbuf*()> alloc_mbuf) noexcept
{
    // Can't use the original `pcap_pkthdr` because the timestamp field
    // written to the disk is expected to contain two 32 bit values but
    // `struct timeval` contains two 64 bit values on 64 bit Linux.
    /*
    struct pcap_pkthdr {
        timeval ts;
        uint32_t caplen;
        uint32_t len;
    };
    */
    struct
    {
        uint32_t sec;
        uint32_t usec;
        uint32_t caplen;
        uint32_t len;
    } hdr = {};
    if (::fread(&hdr, sizeof(hdr), 1, file_.get()) != 1) {
        return put::system_error_code(errno);
    }
    // We can't work with partially captured packets
    if (hdr.caplen != hdr.len) return put::system_error_code(EINVAL);
    rte_mbuf* head = alloc_mbuf();
    rte_mbuf* curr = head;
    rte_mbuf* prev = nullptr;
    for (uint32_t rdlen = 0; rdlen < hdr.caplen; prev = curr) {
        if (prev) {
            curr       = alloc_mbuf();
            prev->next = curr;
            head->nb_segs++;
        }
        const auto len =
            std::min<uint32_t>(hdr.caplen - rdlen, rte_pktmbuf_tailroom(curr));
        if (::fread(rte_pktmbuf_mtod(curr, char*), len, 1, file_.get()) != 1) {
            rte_pktmbuf_free(head);
            return put::system_error_code(errno);
        }
        curr->data_len = len;
        head->pkt_len += len;
        rdlen += len;
    }
    return pkt{
        .tstamp = stdcr::microseconds((hdr.sec * 1'000'000ull) + hdr.usec),
        .mbuf   = head,
    };
}

} // namespace gen::priv
