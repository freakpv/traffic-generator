#pragma once

// Very simple and very inefficient logging functionality

// #define TGLOG_DEBUG_ENABLED

#ifndef TGLOG_DEBUG_ENABLED
// All this macros are needed to mark the arguments of TGLOG_DEBUG as unused.
// Currently it supports up to 5 arguments but can be easily extended.
#define TGLOG_UNUSED0()
#define TGLOG_UNUSED1(a)             (void)(a)
#define TGLOG_UNUSED2(a, b)          (void)(a), TGLOG_UNUSED1(b)
#define TGLOG_UNUSED3(a, b, c)       (void)(a), TGLOG_UNUSED2(b, c)
#define TGLOG_UNUSED4(a, b, c, d)    (void)(a), TGLOG_UNUSED3(b, c, d)
#define TGLOG_UNUSED5(a, b, c, d, e) (void)(a), TGLOG_UNUSED4(b, c, d, e)

#define TGLOG_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, N, ...) N
#define TGLOG_NUM_ARGS(...) \
    TGLOG_NUM_ARGS_IMPL(42, ##__VA_ARGS__, 5, 4, 3, 2, 1, 0)

#define TGLOG_ALL_UNUSED_IMPL2(nargs) TGLOG_UNUSED##nargs
#define TGLOG_ALL_UNUSED_IMPL(nargs)  TGLOG_ALL_UNUSED_IMPL2(nargs)
#define TGLOG_ALL_UNUSED(...) \
    TGLOG_ALL_UNUSED_IMPL(TGLOG_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define TGLOG_DEBUG(fmtstr, ...) TGLOG_ALL_UNUSED(fmtstr, ##__VA_ARGS__)

#else

#define TGLOG_DEBUG(fmtstr, ...) \
    fmt::print(stdout, "DEBUG: " fmtstr, ##__VA_ARGS__)

#endif // TGLOG_DEBUG_ENABLED

#define TGLOG_INFO(fmtstr, ...) \
    fmt::print(stdout, "INFO: " fmtstr, ##__VA_ARGS__)
#define TGLOG_ERROR(fmtstr, ...) \
    fmt::print(stderr, "ERROR: " fmtstr, ##__VA_ARGS__)

