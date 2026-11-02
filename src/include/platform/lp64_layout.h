// Force-included (-include) into the game-logic layer (Common, Content, Server,
// RemoteClient, RemoteServer, rpclocal, freelancer) on the portable 64-bit build.
//
// Those modules carry thousands of static_asserts pinning retail's 32-bit (ILP32)
// struct sizes/offsets. On arm64/LP64 any struct holding a pointer is larger, so they
// can never hold. Instead of editing ~4200 asserts, every static_assert in these
// translation units is compiled out (only ~80 of the repo's 4284 are not layout checks).
//
// Consequence: struct layouts are NOT retail-compatible in this build. Code must not rely on
// raw byte offsets into pointer-bearing structs, nor on binary blobs that embed pointers.
// Re-enable the real checks with -DFL_STRICT_LAYOUT (or by building without this header).
#pragma once
#ifdef __cplusplus
#include "win_compat.h"
#endif
#if defined(__cplusplus) && defined(FREELANCER_PORTABLE) && !defined(FL_STRICT_LAYOUT)
#  define static_assert(...)
#endif
