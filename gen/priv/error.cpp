#include "gen/priv/error.h"

namespace gen::priv
{

class error_category_impl final : public bsys::error_category
{
public:
    const char* name() const noexcept final
    {
        return "generator_error_category";
    }

    std::string message(int err) const noexcept final
    {
        switch (err) {
#define XXX(val, nam) \
    case gen::priv::error::val: return nam;
            TG_GEN_ERRORS(XXX)
#undef XXX
        }
        return fmt::format("Unknown generator error: {}", err);
    }
};

const bsys::error_category& error_category() noexcept
{
    static error_category_impl inst;
    return inst;
}

} // namespace gen::priv
