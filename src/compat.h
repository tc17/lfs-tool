#pragma once

#if defined(__STDC_LIB_EXT1__)
#if (__STDC_LIB_EXT1__ >= 201112L)
#define USE_EXT1
#define __STDC_WANT_LIB_EXT1__ 1
#endif //__STDC_LIB_EXT1__ >= 201112L
#endif //__STDC_LIB_EXT1__

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#if !defined(USE_EXT1)

typedef int32_t errno_t;
typedef int32_t rsize_t;

static inline errno_t strcpy_s(char *restrict dest, rsize_t destsz, const char *restrict src)
{
	snprintf(dest, destsz, "%s", src);
	return 0;
}

static inline errno_t strcat_s(char *restrict dest, rsize_t destsz, const char *restrict src)
{
	strncat(dest, src, destsz-1);
	return 0;
}

#endif //!USE_EXT1
