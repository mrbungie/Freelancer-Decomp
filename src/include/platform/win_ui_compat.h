// Win32 windowing / messaging / misc types for the client layer. Included by win_compat.h.
// Window services forward to fl::IWindowHost (installed by the SDL front end).
#ifndef FL_WIN_UI_COMPAT_H
#define FL_WIN_UI_COMPAT_H
#include <cstdint>
#include "fl/handles.h"
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <malloc/malloc.h>

typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void* HRSRC;
typedef void* HGLOBAL;
typedef void* HLOCAL;
typedef void* HMENU;
typedef long LSTATUS;
typedef void* HKL;
typedef DWORD REGSAM;
typedef HKEY* PHKEY;

// registry
#define REG_BINARY 3
#define REG_MULTI_SZ 7
#define KEY_ALL_ACCESS 0xF003F
#define KEY_WRITE 0x20006
#define HKEY_CLASSES_ROOT ((HKEY)(intptr_t)3)

// messages
#define WM_INITDIALOG 0x0110
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define PM_NOREMOVE 0x0000
inline BOOL TranslateMessage(const MSG*) { return FALSE; }
inline BOOL GetMessageA(MSG*, void*, UINT, UINT) { return FALSE; }

// virtual keys
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_DELETE 0x2E

// resources / cursors / GDI misc
#define MAKEINTRESOURCEA(i) ((const char*)(uintptr_t)((WORD)(i)))
#define MAKEINTRESOURCEW(i) ((const wchar_t*)(uintptr_t)((WORD)(i)))
#define IMAGE_CURSOR 2
#define IMAGE_ICON 1
#define LR_SHARED 0x8000
#define MB_OK 0x00000000L
#define CW_USEDEFAULT ((int)0x80000000)
#define GWL_STYLE (-16)
#define SW_SHOWNORMAL 1
#define HWND_TOPMOST ((HWND)(intptr_t)-1)
#define DIB_RGB_COLORS 0
#define TA_LEFT 0
#define TA_TOP 0
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | ((WORD)((BYTE)(g)) << 8) | (((DWORD)(BYTE)(b)) << 16)))
#define SEVERITY_ERROR 1
#define VER_PLATFORM_WIN32_NT 2
#define CP_ACP 0
#define CP_UTF8 65001
typedef struct tagRGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;
typedef struct tagBITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1]; } BITMAPINFO;
typedef struct tagCURSORINFO { DWORD cbSize, flags; HCURSOR hCursor; POINT ptScreenPos; } CURSORINFO;
typedef struct _OSVERSIONINFOW { DWORD dwOSVersionInfoSize, dwMajorVersion, dwMinorVersion, dwBuildNumber, dwPlatformId; wchar_t szCSDVersion[128]; } OSVERSIONINFOW;
typedef struct _OSVERSIONINFOEXA { OSVERSIONINFOA base; WORD wServicePackMajor, wServicePackMinor, wSuiteMask; BYTE wProductType, wReserved; } OSVERSIONINFOEXA;
typedef struct _SYSTEM_INFO { DWORD dwOemId; DWORD dwPageSize; void* lpMinimumApplicationAddress; void* lpMaximumApplicationAddress; DWORD_PTR dwActiveProcessorMask; DWORD dwNumberOfProcessors, dwProcessorType, dwAllocationGranularity; WORD wProcessorLevel, wProcessorRevision; } SYSTEM_INFO;
typedef struct _MEMORYSTATUS { DWORD dwLength, dwMemoryLoad, dwTotalPhys, dwAvailPhys, dwTotalPageFile, dwAvailPageFile, dwTotalVirtual, dwAvailVirtual; } MEMORYSTATUS, *LPMEMORYSTATUS;
typedef struct _STARTUPINFOA { DWORD cb; char* lpReserved; char* lpDesktop; char* lpTitle; DWORD fields[8]; WORD wShowWindow, cbReserved2; BYTE* lpReserved2; HANDLE h[3]; } STARTUPINFOA;
typedef struct _PROCESS_INFORMATION { HANDLE hProcess, hThread; DWORD dwProcessId, dwThreadId; } PROCESS_INFORMATION;
struct _EXCEPTION_RECORD;
typedef struct _EXCEPTION_POINTERS { struct _EXCEPTION_RECORD* ExceptionRecord; void* ContextRecord; } EXCEPTION_POINTERS;
typedef LONG (*LPTOP_LEVEL_EXCEPTION_FILTER)(EXCEPTION_POINTERS*);

// strings / encoding
inline wchar_t* CharUpperW(wchar_t* s) { for (wchar_t* p = s; p && *p; ++p) *p = (wchar_t)towupper((wint_t)*p); return s; }
inline int lstrlenW(const wchar_t* s) { return (int)wcslen(s); }
inline int _wcsicoll(const wchar_t* a, const wchar_t* b) { return wcscasecmp(a, b); }
#define _snwprintf swprintf
inline int MultiByteToWideChar(UINT, DWORD, const char* s, int n, wchar_t* out, int cap) {
    size_t len = n < 0 ? strlen(s) + 1 : (size_t)n;
    if (!out) return (int)len;
    int i = 0; for (; (size_t)i < len && i < cap; ++i) out[i] = (unsigned char)s[i]; return i;   // Latin-1/ASCII
}
inline int WideCharToMultiByte(UINT, DWORD, const wchar_t* s, int n, char* out, int cap, const char*, BOOL*) {
    size_t len = n < 0 ? wcslen(s) + 1 : (size_t)n;
    if (!out) return (int)len;
    int i = 0; for (; (size_t)i < len && i < cap; ++i) out[i] = s[i] < 256 ? (char)s[i] : '?'; return i;
}
inline int LCMapStringW(DWORD, DWORD flags, const wchar_t* s, int n, wchar_t* out, int cap) {
    size_t len = n < 0 ? wcslen(s) + 1 : (size_t)n; if (!out) return (int)len;
    for (size_t i = 0; i < len && (int)i < cap; ++i) out[i] = (flags & 0x200) ? (wchar_t)towupper((wint_t)s[i]) : (flags & 0x100) ? (wchar_t)towlower((wint_t)s[i]) : s[i];
    return (int)len;
}
inline int LCMapStringA(DWORD, DWORD flags, const char* s, int n, char* out, int cap) {
    size_t len = n < 0 ? strlen(s) + 1 : (size_t)n; if (!out) return (int)len;
    for (size_t i = 0; i < len && (int)i < cap; ++i) out[i] = (flags & 0x200) ? (char)toupper((unsigned char)s[i]) : (flags & 0x100) ? (char)tolower((unsigned char)s[i]) : s[i];
    return (int)len;
}
inline BOOL GetTextExtentPoint32A(HDC, const char*, int, SIZE* sz) { sz->cx = sz->cy = 0; return FALSE; }
inline int MessageBoxW(HWND, const wchar_t* t, const wchar_t* c, UINT) {
    fwprintf(stderr, L"[MessageBox %ls]: %ls\n", c ? c : L"", t ? t : L"");
    if (getenv("FL_TRACE_MESSAGEBOX")) { void* frames[24]; int n = backtrace(frames, 24); backtrace_symbols_fd(frames, n, 2); }
    return 1;
}

// memory / interlocked
inline BOOL IsBadWritePtr(void*, size_t) { return FALSE; }
inline LONG InterlockedIncrement(volatile LONG* p) { return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
inline LONG InterlockedDecrement(volatile LONG* p) { return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST); }
inline HLOCAL LocalAlloc(UINT flags, size_t n) { return (flags & 0x40) ? calloc(1, n) : malloc(n); }
inline void* HeapReAlloc(HANDLE, DWORD, void* p, size_t n) { return realloc(p, n); }

// window / cursor services -> fl::IWindowHost
inline int ShowCursor(BOOL show) { return fl::GetWindowHost()->ShowCursor(show != 0); }
inline BOOL SetCursorPos(int x, int y) { return fl::GetWindowHost()->SetCursorPosition(x, y); }
inline HCURSOR SetCursor(HCURSOR c) { fl::GetWindowHost()->SetCursorShape(c); return nullptr; }
inline BOOL DestroyWindow(HWND w) { fl::GetWindowHost()->DestroyWindow(w); return TRUE; }
inline BOOL SetWindowPos(HWND w, HWND after, int x, int y, int cx, int cy, UINT) {
    return fl::GetWindowHost()->SetWindowRect(w, x, y, cx, cy, after == HWND_TOPMOST);
}
inline BOOL ScreenToClient(HWND, POINT*) { return TRUE; }   // window origin == screen origin in the SDL host
inline HBITMAP CreateCompatibleBitmap(HDC, int, int) { fl::ReportUnimplemented("CreateCompatibleBitmap"); return nullptr; }
inline LONG RegDeleteKeyA(HKEY, const char*) { return 0; }

// ---- more windowing / process / resource bits --------------------------------------------
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_ERASEBKGND 0x0014
#define WM_ACTIVATEAPP 0x001C
#define WM_NCHITTEST 0x0084
#define WM_COMMAND 0x0111
#define HTCLIENT 1
#define SW_HIDE 0
#define GWL_EXSTYLE (-20)
#define HWND_NOTOPMOST ((HWND)(intptr_t)-2)
#define MAPVK_VK_TO_VSC 0
#define FILE_MAP_WRITE 0x0002
#define FACILITY_WIN32 7
typedef struct _SECURITY_ATTRIBUTES { DWORD nLength; void* lpSecurityDescriptor; BOOL bInheritHandle; } SECURITY_ATTRIBUTES;
inline SHORT GetKeyState(int vk) { return (SHORT)fl::GetWindowHost()->GetKeyState(vk); }
inline UINT MapVirtualKeyA(UINT, UINT) { return 0; }
inline int GetKeyNameTextW(LONG, wchar_t* b, int) { if (b) b[0] = 0; return 0; }
inline HWND GetDlgItem(HWND, int) { return nullptr; }
inline BOOL SetWindowTextW(HWND, const wchar_t*) { return TRUE; }
inline BOOL EndDialog(HWND, intptr_t) { return TRUE; }
inline BOOL ShowWindow(HWND, int) { return TRUE; }
inline BOOL InvalidateRect(HWND, const RECT*, BOOL) { return TRUE; }
inline LONG SetWindowLongA(HWND, int, LONG v) { return v; }
inline LONG GetWindowLongA(HWND, int) { return 0; }
inline HWND GetDesktopWindow() { return nullptr; }
inline HWND GetForegroundWindow() { return nullptr; }
inline HMENU GetMenu(HWND) { return nullptr; }
inline BOOL GetClientRect(HWND, RECT* r) { memset(r, 0, sizeof(*r)); return FALSE; }
inline BOOL GetCursorPos(POINT* p) { int x = 0, y = 0; BOOL ok = fl::GetWindowHost()->GetCursorPosition(&x, &y); p->x = x; p->y = y; return ok; }
inline BOOL GetCursorInfo(CURSORINFO* c) { memset(c, 0, sizeof(*c)); return FALSE; }
inline HCURSOR GetCursor() { return nullptr; }
inline UINT GetDoubleClickTime() { return 500; }
inline int FillRect(HDC, const RECT*, void*) { return 0; }
inline int GetDIBits(HDC, HBITMAP, UINT, UINT, void*, BITMAPINFO*, UINT) { fl::ReportUnimplemented("GetDIBits"); return 0; }
inline BOOL GetTextExtentPointW(HDC, const wchar_t*, int, SIZE* sz) { sz->cx = sz->cy = 0; return FALSE; }
inline HANDLE GetCurrentProcess() { return (HANDLE)(intptr_t)-1; }
inline DWORD GetCurrentProcessId() { return (DWORD)getpid(); }
inline DWORD GetCurrentThreadId() { return (DWORD)fl::PtrToHandle(pthread_self()); }
inline void GlobalMemoryStatus(MEMORYSTATUS* m) { memset(m, 0, sizeof(*m)); m->dwLength = sizeof(*m); }
inline void GetSystemInfo(SYSTEM_INFO* s) { memset(s, 0, sizeof(*s)); s->dwPageSize = 4096; s->dwNumberOfProcessors = (DWORD)sysconf(_SC_NPROCESSORS_ONLN); }
inline BOOL GetVersionExW(OSVERSIONINFOW* v) { v->dwMajorVersion = 5; v->dwMinorVersion = 1; v->dwPlatformId = VER_PLATFORM_WIN32_NT; return TRUE; }
inline HMODULE GetModuleHandleW(const wchar_t*) { return nullptr; }
inline DWORD GetTempPathA(DWORD cap, char* out) { const char* t = getenv("TMPDIR"); snprintf(out, cap, "%s", t ? t : "/tmp/"); return (DWORD)strlen(out); }
inline DWORD GetEnvironmentVariableA(const char* n, char* out, DWORD cap) { const char* v = getenv(n); if (!v) return 0; snprintf(out, cap, "%s", v); return (DWORD)strlen(v); }
inline BOOL GetComputerNameA(char* out, DWORD* n) { if (gethostname(out, *n) != 0) return FALSE; *n = (DWORD)strlen(out); return TRUE; }
inline BOOL GetDiskFreeSpaceA(const char*, DWORD* a, DWORD* b, DWORD* c, DWORD* d) { *a = 8; *b = 512; *c = 1u << 20; *d = 1u << 20; return TRUE; }
inline HANDLE GetStdHandle(DWORD) { return nullptr; }
inline BOOL WriteFile(HANDLE, const void*, DWORD n, DWORD* w, void*) { if (w) *w = n; return TRUE; }
inline HANDLE HeapCreate(DWORD, size_t, size_t) { return (HANDLE)1; }
inline BOOL HeapDestroy(HANDLE) { return TRUE; }

// ---- batch 3: threads/TLS/process/window procs -------------------------------------------
#include <map>
#define WM_SYSCOMMAND 0x0112
#define WM_MOUSEMOVE 0x0200
#define WM_DESTROY 0x0002
#define WM_CLOSE 0x0010
#define SC_CLOSE 0xF060
#define SC_SCREENSAVE 0xF140
#define WAIT_ABANDONED 0x80
#define EXCEPTION_EXECUTE_HANDLER 1
#define EXCEPTION_CONTINUE_SEARCH 0
#define ETO_OPAQUE 0x0002
#define ERROR_READ_FAULT 30L
#define CURSOR_SHOWING 0x00000001
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
inline DWORD TlsAlloc() { static DWORD n = 0; return ++n; }
inline thread_local std::map<DWORD, void*> FlTlsSlots;
inline BOOL TlsSetValue(DWORD i, void* v) { FlTlsSlots[i] = v; return TRUE; }
inline void* TlsGetValue(DWORD i) { auto it = FlTlsSlots.find(i); return it == FlTlsSlots.end() ? nullptr : it->second; }
inline BOOL ReleaseMutex(HANDLE) { fl::ReportUnimplemented("ReleaseMutex (named mutex handles)"); return TRUE; }
inline BOOL DuplicateHandle(HANDLE, HANDLE s, HANDLE, HANDLE* d, DWORD, BOOL, DWORD) { *d = s; return TRUE; }
inline BOOL CreateProcessA(const char*, char*, void*, void*, BOOL, DWORD, void*, const char*, STARTUPINFOA*, PROCESS_INFORMATION*) { fl::ReportUnimplemented("CreateProcessA"); return FALSE; }
inline BOOL TerminateProcess(HANDLE, UINT code) { _exit((int)code); }
inline void ExitProcess(UINT code) { exit((int)code); }
inline LPTOP_LEVEL_EXCEPTION_FILTER SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER) { return nullptr; }
inline BOOL DeviceIoControl(HANDLE, DWORD, void*, DWORD, void*, DWORD, DWORD*, void*) { return FALSE; }
inline UINT GetCaretBlinkTime() { return 500; }
inline BOOL GetWindowRect(HWND, RECT* r) { memset(r, 0, sizeof(*r)); return FALSE; }
inline BOOL ClientToScreen(HWND, POINT*) { return TRUE; }
inline BOOL EnableWindow(HWND, BOOL) { return TRUE; }
inline LRESULT DefWindowProcA(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline HWND CreateWindowExA(DWORD, const char*, const char*, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void*) {
    fl::ReportUnimplemented("CreateWindowExA (the SDL front end owns the window)"); return nullptr;
}
typedef intptr_t (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
inline intptr_t DialogBoxParamA(HINSTANCE, const char*, HWND, DLGPROC, LPARAM) { fl::ReportUnimplemented("DialogBoxParamA"); return 0; }
inline void* CreateSolidBrush(COLORREF) { return nullptr; }

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(t, f) ((LONG)__builtin_offsetof(t, f))
#endif
typedef intptr_t (*FARPROC)();
typedef struct _EXCEPTION_RECORD { DWORD ExceptionCode, ExceptionFlags; struct _EXCEPTION_RECORD* ExceptionRecord; void* ExceptionAddress; DWORD NumberParameters; DWORD_PTR ExceptionInformation[15]; } EXCEPTION_RECORD;
typedef struct tagWNDCLASSA { UINT style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; void* hbrBackground; const char* lpszMenuName; const char* lpszClassName; } WNDCLASSA;
#define CS_DBLCLKS 0x0008
#define IDC_ARROW MAKEINTRESOURCEA(32512)
#define BLACK_BRUSH 4
#define BI_RGB 0
inline LRESULT SendMessageA(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline BOOL SetCurrentDirectoryA(const char* p) { return chdir(FlPathBuf(p)) == 0; }
inline HICON LoadIconA(HINSTANCE, const char*) { return nullptr; }
inline size_t HeapSize(HANDLE, DWORD, const void* p) { return p ? malloc_size(p) : 0; }
inline HANDLE CreateMutexA(void*, BOOL, const char*) { fl::ReportUnimplemented("CreateMutexA (named/cross-process mutex)"); return (HANDLE)1; }
inline BOOL AdjustWindowRect(RECT*, DWORD, BOOL) { return TRUE; }
inline BOOL AdjustWindowRectEx(RECT*, DWORD, BOOL, DWORD) { return TRUE; }
inline void* GetStockObject(int) { return nullptr; }
inline wchar_t* _ultow(unsigned long v, wchar_t* b, int r) { swprintf(b, 34, r == 16 ? L"%lx" : L"%lu", v); return b; }
inline wchar_t* _ltow(long v, wchar_t* b, int r) { swprintf(b, 34, r == 16 ? L"%lx" : L"%ld", v); return b; }
inline wchar_t* _itow(int v, wchar_t* b, int r) { swprintf(b, 34, r == 16 ? L"%x" : L"%d", v); return b; }
inline char* _ltoa(long v, char* b, int r) { snprintf(b, 34, r == 16 ? "%lx" : "%ld", v); return b; }
inline unsigned int _clearfp() { return 0; }

#define WM_PAINT 0x000F
#define WM_CREATE 0x0001
#define WM_SIZE 0x0005
#define SWP_NOZORDER 0x0004
#define GWL_USERDATA (-21)
#define WS_VISIBLE 0x10000000L
#define WS_CHILD 0x40000000L
#define VK_INSERT 0x2D
#define VK_HOME 0x24
#define VK_END 0x23
#define SW_MINIMIZE 6
#define SW_MAXIMIZE 3
#define LOCALE_USER_DEFAULT 0x0400
#define C1_CNTRL 0x0020
#define _P_WAIT 0
typedef struct tagWNDCLASSEXA { UINT cbSize; UINT style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor; void* hbrBackground; const char* lpszMenuName; const char* lpszClassName; HICON hIconSm; } WNDCLASSEXA;
typedef struct tagPAINTSTRUCT { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore, fIncUpdate; BYTE rgbReserved[32]; } PAINTSTRUCT;
inline WORD RegisterClassA(const WNDCLASSA*) { fl::ReportUnimplemented("RegisterClassA"); return 0; }
inline WORD RegisterClassExA(const WNDCLASSEXA*) { fl::ReportUnimplemented("RegisterClassExA"); return 0; }
inline BOOL UnregisterClassA(const char*, HINSTANCE) { return TRUE; }
inline HCURSOR LoadCursorA(HINSTANCE, const char*) { return nullptr; }
inline wchar_t* lstrcpynW(wchar_t* d, const wchar_t* s, int n) { if (n > 0) { wcsncpy(d, s, (size_t)n - 1); d[n - 1] = 0; } return d; }
inline BOOL IsDBCSLeadByte(BYTE) { return FALSE; }
inline BOOL IsBadCodePtr(const void*) { return FALSE; }
inline BOOL GetUpdateRect(HWND, RECT*, BOOL) { return FALSE; }
inline HWND GetFocus() { return nullptr; }
inline DWORD FormatMessageW(DWORD, const void*, DWORD, DWORD, wchar_t* b, DWORD, void*) { if (b) b[0] = 0; return 0; }
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_APPS 0x5D
#define VK_PROCESSKEY 0xE5
#define WM_ACTIVATE 0x0006
#define LANG_CHINESE 0x04
#define PRIMARYLANGID(l) ((WORD)(l) & 0x3ff)
inline BOOL IsWindowUnicode(HWND) { return TRUE; }
inline int GetKeyboardLayoutList(int, HKL*) { return 0; }
inline BOOL GetStringTypeExW(DWORD, DWORD, const wchar_t* s, int n, WORD* out) {
    for (int i = 0; i < n; ++i) out[i] = (iswspace((wint_t)s[i]) ? 0x8 : 0) | (iswcntrl((wint_t)s[i]) ? 0x20 : 0) | (iswalpha((wint_t)s[i]) ? 0x100 : 0);
    return TRUE;
}
inline int wsprintfW(wchar_t* b, const wchar_t* f, ...) { va_list a; va_start(a, f); int r = vswprintf(b, 1024, f, a); va_end(a); return r; }

#define NI_CLOSECANDIDATE 0x0011
#define LANG_JAPANESE 0x11
#define LANG_KOREAN 0x12
#define LOCALE_IDEFAULTANSICODEPAGE 0x1004
#define ATTR_TARGET_CONVERTED 0x01
#define WM_INPUTLANGCHANGE 0x0051
#define NORM_IGNORECASE 0x00000001
#define CSTR_EQUAL 2
#define SUBLANGID(l) ((WORD)(l) >> 10)
inline LRESULT SendMessageW(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline BOOL PostMessageA(HWND, UINT, WPARAM, LPARAM) { return FALSE; }
inline HKL GetKeyboardLayout(DWORD) { return nullptr; }
inline BOOL IsDBCSLeadByteEx(UINT, BYTE) { return FALSE; }
inline SHORT GetAsyncKeyState(int vk) { return (SHORT)fl::GetWindowHost()->GetKeyState(vk); }
inline char* lstrcpyA(char* d, const char* s) { return strcpy(d, s); }
inline int CompareStringW(DWORD, DWORD flags, const wchar_t* a, int an, const wchar_t* b, int bn) {
    std::wstring x(a, an < 0 ? wcslen(a) : (size_t)an), y(b, bn < 0 ? wcslen(b) : (size_t)bn);
    if (flags & NORM_IGNORECASE) { for (auto& c : x) c = (wchar_t)towlower((wint_t)c); for (auto& c : y) c = (wchar_t)towlower((wint_t)c); }
    return x < y ? 1 : (x == y ? 2 : 3);
}

// Structured exception handling does not exist off MSVC: map to C++ try/catch (hardware faults
// such as null derefs are NOT caught; they terminate the process).
#if !defined(_MSC_VER)
#define __try try
#define __except(x) catch (...)
#endif
#define LOCALE_SENGLANGUAGE 0x1001
inline int GetLocaleInfoA(DWORD, DWORD, char* b, int n) { if (b && n > 0) { b[0] = '0'; if (n > 1) b[1] = 0; } return 2; }
inline int CompareStringA(DWORD, DWORD flags, const char* a, int an, const char* b, int bn) {
    std::string x(a, an < 0 ? strlen(a) : (size_t)an), y(b, bn < 0 ? strlen(b) : (size_t)bn);
    if (flags & NORM_IGNORECASE) { for (auto& c : x) c = (char)tolower((unsigned char)c); for (auto& c : y) c = (char)tolower((unsigned char)c); }
    return x < y ? 1 : (x == y ? 2 : 3);
}
inline char* lstrcatA(char* d, const char* s) { return strcat(d, s); }
inline int lstrlenA(const char* s) { return (int)strlen(s); }
inline intptr_t _spawnl(int, const char*, const char*, ...) { fl::ReportUnimplemented("_spawnl"); return -1; }
#define _isnan(x) std::isnan(x)
#define _finite(x) std::isfinite(x)
#endif
