// Shim for <rpc.h> (UuidCreate only) -> fl::ISystemInfo.
#ifndef FL_PLATFORM_RPC_H
#define FL_PLATFORM_RPC_H
#include "win_compat.h"
typedef GUID UUID;
inline long UuidCreate(UUID* u) {
    unsigned char b[16]; fl::GetPlatform().system->NewUuid(b);
    memcpy(u, b, sizeof(b) < sizeof(*u) ? sizeof(b) : sizeof(*u)); return 0;
}
#endif
