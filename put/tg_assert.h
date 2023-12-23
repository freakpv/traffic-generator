#pragma once

#define TG_ASSERT(...) assert(__VA_ARGS__)
#define TG_ENFORCE(...)             \
    do {                            \
        if (__VA_ARGS__) ::abort(); \
    } while (0)
