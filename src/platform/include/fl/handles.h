// 32-bit handles for pointers.
//
// Retail's interfaces pass objects as 32-bit values (`int`, DWORD, `unsigned long`): a FileSystem handle is
// the pointer to its node, a material parameter payload holds a `char*`, etc. Pointers are 64-bit here, so
// such values go through a process-wide table instead of being truncated.
//
//   PtrToHandle : null -> 0, the sentinel (void*)-1 -> 0xFFFFFFFF, small integers (< 0x10000, used by the
//                 engine for ids) pass through; any other pointer is registered and gets a stable id.
//   HandleToPtr : the exact inverse.
// Entries are never reused while the process runs (handles may be copied freely by game code).
#pragma once
#include <stdint.h>

namespace fl
{
    uint32_t PtrToHandle(const void* p);
    void* HandleToPtr(uint32_t h);

    template <class T> inline T* HandleTo(uint32_t h) { return static_cast<T*>(HandleToPtr(h)); }

    // A pointer that lives in a 4-byte slot of a retail-layout record (descriptors, wire structs): holds the handle,
    // reads and writes like a T*.
    template <class T> struct Slot32
    {
        uint32_t h;
        Slot32() : h(0) {}
        Slot32(T* p) : h(PtrToHandle(p)) {}
        Slot32& operator=(T* p) { h = PtrToHandle(p); return *this; }
        operator T*() const { return HandleTo<T>(h); }
        T* operator->() const { return HandleTo<T>(h); }
    };
}
