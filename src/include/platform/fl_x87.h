// Portable replacements for the small x87 idioms used by the decompiled RPC codecs.
// Intermediate 80-bit x87 precision is modelled with double (accepted loss).
#ifndef FL_PLATFORM_FL_X87_H
#define FL_PLATFORM_FL_X87_H

#include <cstdint>
#include <cstring>

// FLD dword / FSTP dword: bit-exact copy, except a signalling NaN comes out quiet.
inline void FlX87CopyFloat(const float* Source, float* Destination)
{
    std::uint32_t Bits;
    std::memcpy(&Bits, Source, sizeof(Bits));
    if ((Bits & 0x7f800000u) == 0x7f800000u && (Bits & 0x007fffffu) != 0u)
        Bits |= 0x00400000u;
    std::memcpy(Destination, &Bits, sizeof(Bits));
}

// FLD qword / FSTP qword: bit-exact copy, except a signalling NaN comes out quiet.
inline void FlX87CopyDouble(const double* Source, double* Destination)
{
    std::uint64_t Bits;
    std::memcpy(&Bits, Source, sizeof(Bits));
    if ((Bits & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL &&
        (Bits & 0x000fffffffffffffULL) != 0ULL)
        Bits |= 0x0008000000000000ULL;
    std::memcpy(Destination, &Bits, sizeof(Bits));
}

// x87 FISTP with round-toward-zero as used by retail's chop control word.
// Out-of-range/NaN inputs produce the x86 "integer indefinite" value.
inline long long PortableChopToInt64(double Value)
{
    if (!(Value > -9223372036854775808.0 && Value < 9223372036854775808.0))
        return static_cast<long long>(0x8000000000000000ULL);
    return static_cast<long long>(Value);
}

#endif // FL_PLATFORM_FL_X87_H
