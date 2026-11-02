// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#pragma once

#include <cmath>

#if defined(_M_IX86) && defined(_MSC_VER)

// FLD; FSIN; FSTP qword.
inline double X87Sin(double Angle)
{
    double Result;
    __asm
    {
        fld qword ptr [Angle]
        fsin
        fstp qword ptr [Result]
    }
    return Result;
}

// FLD; FSIN; FSTP dword.
inline float X87SinF(double Angle)
{
    float Result;
    __asm
    {
        fld qword ptr [Angle]
        fsin
        fstp dword ptr [Result]
    }
    return Result;
}

// FLD; FCOS; FSTP qword.
inline double X87Cos(double Angle)
{
    double Result;
    __asm
    {
        fld qword ptr [Angle]
        fcos
        fstp qword ptr [Result]
    }
    return Result;
}

// FLD; FCOS; FSTP dword.
inline float X87CosF(double Angle)
{
    float Result;
    __asm
    {
        fld qword ptr [Angle]
        fcos
        fstp dword ptr [Result]
    }
    return Result;
}

// FLD; FPTAN; FSTP ST0 (drops the 1.0 FPTAN pushes); FSTP qword.
inline double X87Tan(double Angle)
{
    double Result;
    __asm
    {
        fld qword ptr [Angle]
        fptan
        fstp st(0)
        fstp qword ptr [Result]
    }
    return Result;
}

// FLD; FPTAN; FSTP ST0; FSTP dword.
inline float X87TanF(double Angle)
{
    float Result;
    __asm
    {
        fld qword ptr [Angle]
        fptan
        fstp st(0)
        fstp dword ptr [Result]
    }
    return Result;
}

// atan(X) as VC6 inlines it: FLD X; FLD1; FPATAN (= atan(ST1 / ST0)); FSTP qword.
inline double X87Atan(double X)
{
    double Result;
    __asm
    {
        fld qword ptr [X]
        fld1
        fpatan
        fstp qword ptr [Result]
    }
    return Result;
}

// FLD X; FLD1; FPATAN; FSTP dword.
inline float X87AtanF(double X)
{
    float Result;
    __asm
    {
        fld qword ptr [X]
        fld1
        fpatan
        fstp dword ptr [Result]
    }
    return Result;
}

// atan2(Y, X) as VC6 inlines it: FLD Y; FLD X; FPATAN (= atan(ST1 / ST0), quadrant from both
// signs); FSTP qword.
inline double X87Atan2(double Y, double X)
{
    double Result;
    __asm
    {
        fld qword ptr [Y]
        fld qword ptr [X]
        fpatan
        fstp qword ptr [Result]
    }
    return Result;
}

// FLD Y; FLD X; FPATAN; FSTP dword.
inline float X87Atan2F(double Y, double X)
{
    float Result;
    __asm
    {
        fld qword ptr [Y]
        fld qword ptr [X]
        fpatan
        fstp dword ptr [Result]
    }
    return Result;
}

#else

// Portable standard C++ math fallback
inline double X87Sin(double Angle) { return std::sin(Angle); }
inline float  X87SinF(double Angle) { return static_cast<float>(std::sin(Angle)); }

inline double X87Cos(double Angle) { return std::cos(Angle); }
inline float  X87CosF(double Angle) { return static_cast<float>(std::cos(Angle)); }

inline double X87Tan(double Angle) { return std::tan(Angle); }
inline float  X87TanF(double Angle) { return static_cast<float>(std::tan(Angle)); }

inline double X87Atan(double X) { return std::atan(X); }
inline float  X87AtanF(double X) { return static_cast<float>(std::atan(X)); }

inline double X87Atan2(double Y, double X) { return std::atan2(Y, X); }
inline float  X87Atan2F(double Y, double X) { return static_cast<float>(std::atan2(Y, X)); }

#endif
