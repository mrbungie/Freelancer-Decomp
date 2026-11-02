// Shim for <shlobj.h>: SHGetFolderPathA forwards to fl::ISystemFolders.
#ifndef FL_PLATFORM_SHLOBJ_H
#define FL_PLATFORM_SHLOBJ_H
#include "win_compat.h"
#define CSIDL_FLAG_CREATE 0x8000
#define CSIDL_LOCAL_APPDATA 0x001c
#define CSIDL_APPDATA 0x001a
#define CSIDL_PERSONAL 0x0005
#define CSIDL_MYPICTURES 0x0027
inline HRESULT SHGetFolderPathA(void*, int csidl, void*, DWORD, char* path)
{
    fl::Folder f;
    switch (csidl & 0xff) {
        case 0x1c: f = fl::Folder::LocalAppData; break;
        case 0x1a: f = fl::Folder::RoamingAppData; break;
        case 0x27: f = fl::Folder::Pictures; break;
        default:   f = fl::Folder::Documents; break;
    }
    return fl::GetPlatform().folders->Get(f, (csidl & CSIDL_FLAG_CREATE) != 0, path, 260) ? 0 : -1;
}
#endif
