// Force-included into every C++ translation unit of the portable build.
//
// Retail code converts 32-bit integers to pointers with reinterpret_cast (handles, engine ids, playback
// ids). On a 64-bit target that zero-extends garbage; clang does not even warn for reinterpret_cast.
// Here, an integer narrower than a pointer converted to a pointer goes through fl::HandleToPtr
// (src/platform/include/fl/handles.h), the inverse of fl::PtrToHandle that the pointer->int sites use.
// Everything else is a plain reinterpret_cast.
#pragma once
#ifdef __cplusplus
#include <cstdint>
#include <type_traits>
#include "fl/handles.h"

template <class T, class U>
inline T FlReinterpretCast(U u)
{
#pragma push_macro("reinterpret_cast")
#undef reinterpret_cast
    if constexpr (fl::IsSlot32<U>::value)
        return reinterpret_cast<T>(static_cast<typename fl::IsSlot32<U>::Pointee*>(u));   // retail 4-byte pointer slot
    else if constexpr (std::is_pointer<T>::value && !std::is_function<typename std::remove_pointer<T>::type>::value &&
                  std::is_integral<U>::value && sizeof(U) < sizeof(void*))
        return static_cast<T>(fl::HandleToPtr(static_cast<uint32_t>(u)));
    else
        return reinterpret_cast<T>(u);
#pragma pop_macro("reinterpret_cast")
}
#define reinterpret_cast FlReinterpretCast
#endif
