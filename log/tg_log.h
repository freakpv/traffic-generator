#pragma once

// Very simple and very inefficient logging functionality
// Would be good if time-stamping is added ... at least!

// #define TG_LOG_DEBUG_ENABLED

#ifndef TG_LOG_DEBUG_ENABLED
// All this macros are needed to mark the arguments of TG_LOG_DEBUG as unused.
// Currently it supports up to 5 arguments but can be easily extended.
#define TG_LOG_UNUSED0()
#define TG_LOG_UNUSED1(a)             (void)(a)
#define TG_LOG_UNUSED2(a, b)          (void)(a), TG_LOG_UNUSED1(b)
#define TG_LOG_UNUSED3(a, b, c)       (void)(a), TG_LOG_UNUSED2(b, c)
#define TG_LOG_UNUSED4(a, b, c, d)    (void)(a), TG_LOG_UNUSED3(b, c, d)
#define TG_LOG_UNUSED5(a, b, c, d, e) (void)(a), TG_LOG_UNUSED4(b, c, d, e)

#define TG_LOG_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, N, ...) N
#define TG_LOG_NUM_ARGS(...) \
    TG_LOG_NUM_ARGS_IMPL(42, ##__VA_ARGS__, 5, 4, 3, 2, 1, 0)

#define TG_LOG_ALL_UNUSED_IMPL2(nargs) TG_LOG_UNUSED##nargs
#define TG_LOG_ALL_UNUSED_IMPL(nargs)  TG_LOG_ALL_UNUSED_IMPL2(nargs)
#define TG_LOG_ALL_UNUSED(...) \
    TG_LOG_ALL_UNUSED_IMPL(TG_LOG_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

#define TG_LOG_DEBUG(fmtstr, ...) TG_LOG_ALL_UNUSED(fmtstr, ##__VA_ARGS__)

#else

#define TG_LOG_DEBUG(fmtstr, ...) \
    fmt::print(stdout, "DEBUG: " fmtstr, ##__VA_ARGS__)

#endif // TG_LOG_DEBUG_ENABLED

#define TG_LOG_INFO(fmtstr, ...) \
    fmt::print(stdout, "INFO: " fmtstr, ##__VA_ARGS__)
#define TG_LOG_ERROR(fmtstr, ...) \
    fmt::print(stderr, "ERROR: " fmtstr, ##__VA_ARGS__)

