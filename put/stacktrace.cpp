#include "stacktrace.h"

namespace put // project utilities
{

static bool write_str(int fd, const char* str) noexcept
{
    const ssize_t len = strlen(str);
    return (write(fd, str, len) == len);
}

// The conversion functions are needed because sprintf/snprintf are not
// listed as async-signal safe functions because of their locale usage.
static char* u32_to_dec(uint32_t val, char* buf, size_t cap) noexcept
{
    constexpr size_t max_len = 10;
    assert(cap >= (max_len + 1)); // 10 digits + NULL terminator

    int i = max_len;
    do {
        buf[--i] = "0123456789"[val % 10];
        val /= 10;
    } while (val);
    assert(i >= 0);
    memmove(buf, buf + i, max_len - i);
    buf[max_len - i] = 0;

    return buf;
}

static char* u64_to_hex(uint64_t val, char* buf, size_t cap) noexcept
{
    assert(cap >= 19); // 0x + 16 hex chars + NULL terminator

    buf[18] = 0;
    for (int i = 17; i > 1; --i, val /= 16) {
        buf[i] = "0123456789ABCDEF"[val % 16];
    }
    buf[1] = 'x';
    buf[0] = '0';

    return buf;
}

////////////////////////////////////////////////////////////////////////////////
// SIGQUIT - is a user generated signal (kind of)
#define X3ME_FATAL_SIGNALS(MACRO) \
    MACRO(SIGABRT)                \
    MACRO(SIGBUS)                 \
    MACRO(SIGFPE)                 \
    MACRO(SIGILL)                 \
    MACRO(SIGSEGV)                \
    MACRO(SIGSYS)                 \
    MACRO(SIGTRAP)

struct stacktrace_ctx
{
    static constexpr int invalid_fd = -1;

    using notification_type = void (*)();

    notification_type notification;

    // The descriptor is never closed.
    int output_fd_ = invalid_fd;
};
static stacktrace_ctx g_st_ctx;

static void dump_signal_info(int fd, const siginfo_t& inf) noexcept
{
    auto dump_u32_info = [](int fd, const char* lbl, uint32_t u32) noexcept {
        char buf[32];
        write_str(fd, lbl);
        write_str(fd, u32_to_dec(u32, buf, sizeof(buf)));
        write_str(fd, "\n");
    };
    auto dump_faulty_addr = [](int fd, const void* addr) noexcept {
        char buf[32];
        write_str(fd, "Fault memory address: ");
        write_str(fd, u64_to_hex(reinterpret_cast<uintptr_t>(addr), buf,
                                 sizeof(buf)));
        write_str(fd, "\n");
    };

    write_str(fd, "--- SIGNAL INFO BEG ---\n");

    switch (inf.si_signo) {
#define XXX(signame) \
    case signame: write_str(fd, "Signal: " #signame "\n"); break;
        X3ME_FATAL_SIGNALS(XXX)
#undef XXX
    default: {
        dump_u32_info(fd, "Signal: ", inf.si_signo);
    } break;
    }
    dump_u32_info(fd, "Sending PID: ", inf.si_pid);
    dump_u32_info(fd, "Sending UID: ", inf.si_uid);
    switch (inf.si_signo) {
    case SIGABRT: write_str(fd, "Info: Abort signal from abort(3)\n"); break;
    case SIGBUS:
        write_str(fd, "Info: Bus error (bad memory access)\n");
        dump_faulty_addr(fd, inf.si_addr);
        switch (inf.si_code) {
        case BUS_ADRALN:
            write_str(fd, "Additional info: Invalid address alignment\n");
            break;
        case BUS_ADRERR:
            write_str(fd, "Additional info: Nonexistent physical address\n");
            break;
        case BUS_OBJERR:
            write_str(fd, "Additional info: Object specific hardware error\n");
            break;
        case BUS_MCEERR_AR:
            write_str(fd, "Additional info: Hardware memory error consumed on "
                          "a machine check; action required\n");
            break;
        case BUS_MCEERR_AO:
            write_str(fd, "Additional info: Hardware memory error detected in "
                          "process but not consumed; action optional\n");
            break;
        }
        break;
    case SIGFPE:
        write_str(fd, "Info: Floating-point exception\n");
        dump_faulty_addr(fd, inf.si_addr);
        switch (inf.si_code) {
        case FPE_INTDIV:
            write_str(fd, "Additional info: Integer divide by zero\n");
            break;
        case FPE_INTOVF:
            write_str(fd, "Additional info: Integer overflow\n");
            break;
        case FPE_FLTDIV:
            write_str(fd, "Additional info: Floating-point divide by zero\n");
            break;
        case FPE_FLTOVF:
            write_str(fd, "Additional info: Floating-point overflow\n");
            break;
        case FPE_FLTUND:
            write_str(fd, "Additional info: Floating-point underflow\n");
            break;
        case FPE_FLTRES:
            write_str(fd, "Additional info: Floating-point inexact result\n");
            break;
        case FPE_FLTINV:
            write_str(fd,
                      "Additional info: Floating-point invalid operation\n");
            break;
        case FPE_FLTSUB:
            write_str(fd, "Additional info: Subscript out of range\n");
            break;
        }
        break;
    case SIGILL:
        write_str(fd, "Info: Illegal Instruction\n");
        dump_faulty_addr(fd, inf.si_addr);
        switch (inf.si_code) {
        case ILL_ILLOPC:
            write_str(fd, "Additional info: Illegal opcode\n");
            break;
        case ILL_ILLOPN:
            write_str(fd, "Additional info: Illegal operand\n");
            break;
        case ILL_ILLADR:
            write_str(fd, "Additional info: Illegal addressing mode\n");
            break;
        case ILL_ILLTRP:
            write_str(fd, "Additional info: Illegal trap\n");
            break;
        case ILL_PRVOPC:
            write_str(fd, "Additional info: Privileged opcode\n");
            break;
        case ILL_PRVREG:
            write_str(fd, "Additional info: Privileged register\n");
            break;
        case ILL_COPROC:
            write_str(fd, "Additional info: Coprocessor error\n");
            break;
        case ILL_BADSTK:
            write_str(fd, "Additional info: Internal stack error\n");
            break;
        }
        break;
    case SIGSEGV:
        write_str(fd, "Info: Invalid memory reference\n");
        dump_faulty_addr(fd, inf.si_addr);
        switch (inf.si_code) {
        case SEGV_MAPERR:
            write_str(fd, "Additional info: Address not mapped to object\n");
            break;
        case SEGV_ACCERR:
            write_str(fd, "Additional info: Invalid permissions for "
                          "mapped object\n");
            break;
        case SEGV_BNDERR:
            write_str(fd, "Additional info: Failed address bound checks\n");
            break;
        case SEGV_PKUERR:
            write_str(fd, "Additional info: Access was denied by memory "
                          "protection keys\n");
            break;
        }
        break;
    case SIGSYS:
        write_str(fd, "Info: Bad system call\n");
        switch (inf.si_code) {
#if defined(SYS_SECCOMP)
        case SYS_SECCOMP:
            write_str(
                fd, "Additional info: Triggered by a seccomp(2) filter rule\n");
            break;
#endif
        }
        break;
    case SIGTRAP:
        write_str(fd, "Info: Trace/breakpoint trap\n");
        dump_faulty_addr(fd, inf.si_addr);
        switch (inf.si_code) {
        case TRAP_BRKPT:
            write_str(fd, "Additional info: Process breakpoint\n");
            break;
        case TRAP_TRACE:
            write_str(fd, "Additional info: Process trace trap\n");
            break;
#if defined(TRAP_BRANCH)
        case TRAP_BRANCH:
            write_str(fd, "Additional info: Process taken branch trap\n");
            break;
#endif
#if defined(TRAP_HWPKPT)
        case TRAP_HWBKPT:
            write_str(fd, "Additional info: Hardware breakpoint/watchpoint\n");
            break;
#endif
        }
        break;
    }
    if (inf.si_code == SI_USER) {
        write_str(fd, "Additional info: Sent by user\n");
    } else if (inf.si_code == SI_KERNEL) {
        write_str(fd, "Additional info: Sent by the kernel\n");
    }

    // Intentionally don't check the result of the end call because
    // it's not important.
    write_str(fd, "--- SIGNAL INFO END ---\n");
}

static bool dump_registers(int fd, const ucontext_t& uctx) noexcept
{
    if (!write_str(fd, "--- CPU REGISTERS BEG ---\n")) return false;

    auto write_reg_info = [&](const char* name, int reg, bool new_line) {
        char buf[32];
        u64_to_hex(uctx.uc_mcontext.gregs[reg], buf, sizeof(buf));
        if (!write_str(fd, name) || !write_str(fd, buf)) return false;
        return new_line ? write_str(fd, "\n") : write_str(fd, " ");
    };

#if defined(__x86_64__)
    if (!write_reg_info("RAX:", REG_RAX, false)) return false;
    if (!write_reg_info("RDI:", REG_RDI, false)) return false;
    if (!write_reg_info("RSI:", REG_RSI, false)) return false;
    if (!write_reg_info("RDX:", REG_RDX, true)) return false;
    if (!write_reg_info("RCX:", REG_RCX, false)) return false;
    if (!write_reg_info("R8:", REG_R8, false)) return false;
    if (!write_reg_info("R9:", REG_R9, false)) return false;
    if (!write_reg_info("R10:", REG_R10, true)) return false;
    if (!write_reg_info("R11:", REG_R11, false)) return false;
    if (!write_reg_info("R12:", REG_R12, false)) return false;
    if (!write_reg_info("R13:", REG_R13, false)) return false;
    if (!write_reg_info("R14:", REG_R14, true)) return false;
    if (!write_reg_info("R15:", REG_R15, false)) return false;
    if (!write_reg_info("RBP:", REG_RBP, false)) return false;
    if (!write_reg_info("RBX:", REG_RBX, false)) return false;
    if (!write_reg_info("RSP:", REG_RSP, true)) return false;
    if (!write_reg_info("RIP:", REG_RIP, false)) return false;
    if (!write_reg_info("EFL:", REG_EFL, false)) return false;
    if (!write_reg_info("CSGSFS:", REG_CSGSFS, false)) return false;
    if (!write_reg_info("ERR:", REG_ERR, true)) return false;
    if (!write_reg_info("TRAPNO:", REG_TRAPNO, false)) return false;
    if (!write_reg_info("OLDMASK:", REG_OLDMASK, false)) return false;
    if (!write_reg_info("CR2:", REG_CR2, false)) return false;
#else
#error "Unsupported architecture"
#endif

    // Intentionally don't check the result of the end call because
    // it's not important.
    write_str(fd, "\n--- CPU REGISTERS END ---\n");

    return true;
}

int dump_stacktrace(int fd) noexcept
{
    if (!write_str(fd, "--- STACK TRACE BEG ---\n")) return errno;

    constexpr int max_levels     = 128;
    void* stacktrace[max_levels] = {0};
    int cnt_levels               = backtrace(stacktrace, max_levels);
    if (cnt_levels <= 0) return errno;

    backtrace_symbols_fd(stacktrace, cnt_levels, fd);

    // Currently we don't dump a stack-trace with demangled symbols.
    // The problem is that in order to demangle the symbols we need to get
    // the stack-trace in a memory buffer without using malloc because the
    // latter is not async-signal safe.
    // `backtrace_symbols` can't be used because it uses malloc internally.
    // Possible implementation would be to read the stack-trace back from the
    // `fd` we just wrote it to a stack allocated buffer.
    // However, we won't gain much from adding this complexity here and thus
    // it's not currently implemented.

    // Intentionally don't check the result of the end call because
    // it's not important.
    write_str(fd, "--- STACK TRACE END ---\n");

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

static void fatal_signal_handler(int sig, siginfo_t* inf, void* ctx)
{
    // Atomics which are guaranteed to be lock-free can be used from
    // inside signal handlers.
    // The std::atomic_flag is the only atomic which is always guaranteed to be
    // lock-free.
    // Since C++20 the default constructor of the std::atomic_flag puts it in
    // clear state and ATOMIC_FLAG_INIT is no longer needed and deprecated.
    static std::atomic_flag called;
    if (called.test_and_set()) {
        // We enter here in case more than one thread execute the signal handler
        // simultaneously e.g. 2 or more threads hit call to abort.
        // The current implementation allows the first signal handling to
        // finish while the other handlers will end up here pausing. The pause
        // call here will actually never return because after the first handler
        // finishes it raises the deadly signal again and this kills the app.
        pause();
    }

    dump_signal_info(g_st_ctx.output_fd_, *inf);

    dump_stacktrace(g_st_ctx.output_fd_);

    dump_registers(g_st_ctx.output_fd_, *static_cast<const ucontext_t*>(ctx));

    fflush(stdout);
    fflush(stderr);

    // Set the default handler for the received signal.
    // In this way the subsequent raise calls will call the default handler
    signal(sig, SIG_DFL);

    raise(sig); // Now re-raise the signal to its default handler.
}

int dump_stacktrace_on_fatal_signal(int output_fd) noexcept
{
    assert(output_fd != stacktrace_ctx::invalid_fd);
    // A successful call to this function must be done only once
    assert(g_st_ctx.output_fd_ == stacktrace_ctx::invalid_fd);

    struct sigaction sact = {};
    sact.sa_sigaction     = fatal_signal_handler;
    sact.sa_flags         = SA_SIGINFO;
    sigfillset(&sact.sa_mask); // Block every signal during the handler

#define XXX(sig) \
    if (sigaction(sig, &sact, nullptr) < 0) return errno;
    X3ME_FATAL_SIGNALS(XXX)
#undef XXX

    // This call to `backtrace` here is needed to ensure that libgcc is loaded.
    // See this note from `man backtrace`:
    // backtrace()  and  backtrace_symbols_fd()  don't  call malloc()
    // explicitly, but they are part of libgcc, which gets loaded dynamically
    // when first used. Dynamic loading usually triggers a call to malloc(3). If
    // you need certain calls  to  these two  functions  to  not allocate memory
    // (in signal handlers, for example), you need to make sure libgcc is loaded
    // before‐hand.
    constexpr int max_levels = 64;
    void* stacktrace[max_levels];
    if (backtrace(stacktrace, max_levels) <= 0) return errno;

    // Everything, which could have failed, has succeeded.
    // Set the context variables.
    g_st_ctx.output_fd_ = output_fd;

    return 0;
}

} // namespace put

#undef X3ME_FATAL_SIGNALS
