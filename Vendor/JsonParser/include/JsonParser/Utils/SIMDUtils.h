#pragma once
#include <cstddef>
#include <array>
#include <stdexcept>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#include <immintrin.h>
#define HAS_SSE2
#if defined(__AVX2__) || (defined(_MSC_VER) && defined(__AVX2__))
#define HAS_AVX2
#endif
#endif

#ifdef _MSC_VER
#include <intrin.h>
#define CTZ32(x) _tzcnt_u32(x)
#define CTZ16(x) _tzcnt_u16(x)
#elif defined(__GNUC__) || defined(__clang__)
#define CTZ32(x) __builtin_ctz(x)
#define CTZ16(x) __builtin_ctz(x)
#else
namespace
{
    inline int CTZ16_FUNC(uint16_t x) {
        int count = 0;
        while ((x & 1) == 0 && x != 0) { x >>= 1; count++; }
        return count;
    }
    inline int CTZ32_FUNC(uint32_t x) {
        int count = 0;
        while ((x & 1) == 0 && x != 0) { x >>= 1; count++; }
        return count;
    }
}
#define CTZ32(x) CTZ32_FUNC(x)
#define CTZ16(x) CTZ16_FUNC(x)
#endif

