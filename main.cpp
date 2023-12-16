#include "aliases.h"
#include "application.h"
#include "version_info.h"

#include "log/log.h"
#include "put/stacktrace.h"
#include "put/throw.h"

template <>
struct fmt::formatter<bpo::options_description> : fmt::ostream_formatter
{
};

int main(int argc, char** argv)
{
    // The secondary options are given only to the secondary child processes.
    bpo::options_description opts("Options");
    // clang-format off
    opts.add_options()
        ("help,H", "This help message")
        ("config,C", bpo::value<stdfs::path>(), "Path to configuration file")
        ("version,V", "Version and other info about the binary");
    // clang-format on

    try {
        bpo::variables_map vm;
        bpo::store(bpo::parse_command_line(argc, argv, opts), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            fmt::print(stdout, "{}\n", opts);
        } else if (vm.count("version")) {
            fmt::print(stdout,
                       "Traffic Generator.\n"
                       "Build date-time: {}.\n"
                       "Git hash: {}.\n",
                       build_datetime, build_githash);
        } else if (vm.count("config")) {
            // Redirect the crash information to the stderr
            if (int err = put::dump_stacktrace_on_fatal_signal(STDERR_FILENO);
                err != 0) {
                put::throw_system_error(
                    err, "Unable to setup the stacktrace dumper");
            }
            application(vm["config"].as<stdfs::path>()).run();
        } else {
            put::throw_runtime_error("No options provided.\n{}", opts);
        }
    } catch (const std::exception& ex) {
        fmt::print(stderr, "ERROR: Failed to run the traffic generator! {}\n",
                   ex.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
