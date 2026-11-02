// Shim for <msctf.h> (Text Services Framework). Only ITfLangBarMgr is referenced (to turn the
// Windows language bar off); with no TSF this is a no-op interface that CoCreateInstance never returns.
#pragma once
#include "win_compat.h"
struct __declspec(uuid("87955690-E627-11D2-8DDB-00105A2799B5")) ITfLangBarMgr : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE ShowFloating(DWORD) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetShowFloatingStatus(DWORD*) = 0;
};
