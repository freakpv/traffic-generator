#pragma once

#define TG_ASSERT(...) assert(__VA_ARGS__)
#define TG_ENFORCE(...)             \
    do {                            \
        if (__VA_ARGS__) ::abort(); \
    } while (0)

// __builtin_unreachable() could have been used for the below macro but the GCC
// doesn't always produce the "ud2" instruction and thus we can't easily catch
// the wrong assumptions in our code without it
#define TG_UNREACHABLE()                                               \
    do {                                                               \
        fmt::print(stderr, "{}:{}, {}. Unreachable code.\n", __FILE__, \
                   __LINE__, __PRETTY_FUNCTION__);                     \
        ::abort();                                                     \
    } while (0)
