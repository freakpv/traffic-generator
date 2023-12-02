#pragma once

namespace put // project utilities
{

// The function is async-signal safe i.e. can be safely called from
// inside a signal handler.
// The function returns 0 in case of success and some errno error code otherwise
int dump_stacktrace(int output_fd) noexcept;

// The function enables stacktrace and other information dump on fatal signal.
// It must be called only once in a given program by the main thread.
// The function returns 0 in case of success and some errno error code otherwise
int dump_stacktrace_on_fatal_signal(int output_fd) noexcept;

} // namespace put
