#pragma once

#include "put/fun_ref.h"

namespace gen::priv
{

class tcap_loader
{
    struct fcloser
    {
        void operator()(FILE* f) const noexcept { ::fclose(f); }
    };
    std::unique_ptr<FILE, fcloser> file_;

public:
    explicit tcap_loader(const stdfs::path&);

    tcap_loader() noexcept;
    ~tcap_loader() noexcept;
    tcap_loader(tcap_loader&&) noexcept;
    tcap_loader& operator=(tcap_loader&&) noexcept;

    tcap_loader(const tcap_loader&)            = delete;
    tcap_loader& operator=(const tcap_loader&) = delete;

    struct pkt
    {
        stdcr::microseconds tstamp;
        rte_mbuf* mbuf;
    };
    bout::result<pkt> load_pkt(put::fun_ref<rte_mbuf*()>) noexcept;

    bool is_eof() const noexcept { return ::feof(file_.get()); }

    bool is_valid() const noexcept { return !!file_; }
};

} // namespace gen::priv
