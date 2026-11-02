// Portable shim for MSVC <intrin.h> (arm64/x86_64 macOS).
#ifndef FL_PLATFORM_INTRIN_H
#define FL_PLATFORM_INTRIN_H
#include <stdint.h>
#include <string.h>

#ifndef __debugbreak
#define __debugbreak() __builtin_trap()
#endif

static inline unsigned long long __rdtsc(void)
{
#if defined(__aarch64__)
    unsigned long long v;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
#else
    unsigned int lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;
#endif
}

// REP MOVS equivalents: copy Count elements; return nothing (MSVC advances the pointers).
static inline void __movsd(uint32_t* Dst, const uint32_t* Src, size_t Count)
{
    // retail "unsigned long" is 32-bit
    memcpy((void*)Dst, (const void*)Src, Count * 4);
}
static inline void __movsb(unsigned char* Dst, const unsigned char* Src, size_t Count)
{
    memcpy(Dst, Src, Count);
}

static inline unsigned char _BitScanForward(unsigned long* Index, unsigned long Mask)
{
    uint32_t m = (uint32_t)Mask;
    if (m == 0) return 0;
    *Index = (unsigned long)__builtin_ctz(m);
    return 1;
}
static inline unsigned char _BitScanReverse(unsigned long* Index, unsigned long Mask)
{
    uint32_t m = (uint32_t)Mask;
    if (m == 0) return 0;
    *Index = (unsigned long)(31 - __builtin_clz(m));
    return 1;
}
#endif
