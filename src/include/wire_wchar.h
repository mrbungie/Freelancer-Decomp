// Wire/file formats carry 16-bit characters (retail wchar_t is 2 bytes); the host wchar_t is wider.
// FlWire16Out(p, n) / FlWire16In(p, n) convert at the buffer boundary of a (Put|Get)Bytes call:
//   Stream.PutBytes(FlWire16Out(Name, Count), Count * 2u);   // narrows for the duration of the call
//   Stream.GetBytes(FlWire16In(Name, Count), Count * 2u);    // widens into Name when the call finishes
#pragma once
#include <cstdint>
#include <vector>

class FlWire16Out
{
    std::vector<uint16_t> Buf;
public:
    FlWire16Out(const wchar_t* P, size_t N) : Buf(N + 1) { for (size_t I = 0; P && I < N; ++I) Buf[I] = static_cast<uint16_t>(P[I]); }
    operator const void*() const { return Buf.data(); }
    operator void*() { return Buf.data(); }
};

class FlWire16In
{
    wchar_t* Dst; size_t Count; std::vector<uint16_t> Buf;
public:
    FlWire16In(wchar_t* P, size_t N) : Dst(P), Count(N), Buf(N + 1) {}
    operator void*() { return Buf.data(); }
    ~FlWire16In() { for (size_t I = 0; Dst && I < Count; ++I) Dst[I] = static_cast<wchar_t>(Buf[I]); }
};
