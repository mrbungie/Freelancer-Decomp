// Shim for <comdef.h>: only _bstr_t (BSTR holder) as used by XMLReader::read_buffer.
#ifndef FL_PLATFORM_COMDEF_H
#define FL_PLATFORM_COMDEF_H
#include "oleauto.h"
class _bstr_t {
    BSTR m_;
public:
    _bstr_t() : m_(nullptr) {}
    _bstr_t(BSTR s, bool fCopy) : m_(s) { (void)fCopy; }  // fCopy=false: take ownership
    _bstr_t(const wchar_t* s) : m_(nullptr) { if (s) m_ = SysAllocStringByteLen((const char*)s, (unsigned)(wcslen(s) * sizeof(wchar_t))); }
    ~_bstr_t() { SysFreeString(m_); }
    operator BSTR() const { return m_; }
};
inline void _com_issue_error(long) {}
#endif
