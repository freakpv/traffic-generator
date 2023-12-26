#pragma once

namespace app
{

class application
{
    std::unique_ptr<class application_impl> impl_;

public:
    explicit application(const stdfs::path&);
    ~application() noexcept;

    application()                              = delete;
    application(application&&)                 = delete;
    application(const application&)            = delete;
    application& operator=(application&)       = delete;
    application& operator=(const application&) = delete;

    void run() noexcept;
};

} // namespace app
