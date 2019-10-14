#include <stdio.h>

#pragma once

#define CHECK_ERROR(expr, code, msg, ...) ({                        \
    if (!(expr))                                                    \
    {                                                               \
        fprintf(stderr, "[%d] %s: ", __LINE__, __func__);          \
        fprintf(stderr, #expr " failed: " msg "\n", ##__VA_ARGS__); \
        result = code;                                              \
        goto done;                                                  \
    }                                                               \
})

#define ERROR(msg, ...) ({                            \
    fprintf(stderr, "[%d] %s: ", __LINE__, __func__); \
    fprintf(stderr, msg "\n", ##__VA_ARGS__);         \
})

#define INFO(msg, ...) ({                             \
    fprintf(stdout, "[%d] %s: ", __LINE__, __func__); \
    fprintf(stdout, msg "\n", ##__VA_ARGS__);         \
})
