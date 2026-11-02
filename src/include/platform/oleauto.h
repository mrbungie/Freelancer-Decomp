// Portable shim for <oleauto.h>: BSTR/VARIANT basics. The MSXML SAX COM classes used by
// common_rdlio.cpp are NOT provided (CoCreateInstance fails) -- see PORTING_MACOS.md.
#ifndef FL_PLATFORM_OLEAUTO_H
#define FL_PLATFORM_OLEAUTO_H
#include "win_compat.h"
#include <stdlib.h>
typedef wchar_t* BSTR;
typedef short VARIANT_BOOL;
#define VT_EMPTY 0
#define VT_BSTR 8
struct tagVARIANT { unsigned short vt; unsigned short r1, r2, r3; union { BSTR bstrVal; long lVal; VARIANT_BOOL boolVal; double dblVal; void* byref; }; };
typedef struct tagVARIANT VARIANT;
inline BSTR SysAllocStringByteLen(const char* src, unsigned int bytes) {
    char* mem = (char*)malloc(4 + bytes + 2); if (!mem) return nullptr;
    *(unsigned int*)mem = bytes; if (src) memcpy(mem + 4, src, bytes); else memset(mem + 4, 0, bytes);
    mem[4 + bytes] = mem[5 + bytes] = 0; return (BSTR)(mem + 4);
}
inline unsigned int SysStringByteLen(BSTR b) { return b ? *(unsigned int*)((char*)b - 4) : 0; }
inline void SysFreeString(BSTR b) { if (b) free((char*)b - 4); }
inline long VariantClear(VARIANT* v) { if (v->vt == VT_BSTR) SysFreeString(v->bstrVal); v->vt = VT_EMPTY; return 0; }
#endif
