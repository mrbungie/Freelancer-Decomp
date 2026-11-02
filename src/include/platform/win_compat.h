// win_compat.h - High-fidelity Win32 compatibility layer for macOS / POSIX
#pragma once

#ifndef _WIN32

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <climits>
#include <cwchar>
#include <strings.h>
#include <string>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

// Calling conventions and MSVC specifiers
#define __cdecl
#define __stdcall
#define __thiscall
#define __fastcall
#define WINAPI
#define APIENTRY
#define CALLBACK
#define __forceinline inline
#define __inline inline

// Basic Types
typedef int BOOL;
#define TRUE 1
#define FALSE 0

typedef uint8_t  BYTE;
typedef uint16_t WORD;
typedef uint16_t USHORT;
typedef int16_t  SHORT;
typedef uint32_t DWORD;
typedef uint32_t UINT;
typedef int32_t  INT;
typedef int32_t  LONG;
typedef uint32_t ULONG;
typedef int64_t  LONGLONG;
typedef uint64_t ULONGLONG;
typedef float    FLOAT;
typedef double   DOUBLE;

typedef BYTE*    PBYTE;
typedef DWORD*   PDWORD;
typedef ULONG*   PULONG;
typedef WORD*    PWORD;
typedef int*     PINT;
typedef LONG*    PLONG;
typedef LONG*    LPLONG;
typedef ULONG*   LPULONG;

typedef uintptr_t ULONG_PTR;
typedef intptr_t  LONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef uintptr_t UINT_PTR;
typedef intptr_t  INT_PTR;

typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HINSTANCE;
typedef void* HWND;
typedef void* HDC;
typedef void* HGDIOBJ;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HBRUSH;
typedef void* HKEY;
typedef void* HBITMAP;
typedef void* HMONITOR;

typedef void* LPVOID;
typedef const void* LPCVOID;

typedef char CHAR;
typedef char* LPSTR;
typedef char* PSTR;
typedef const char* LPCSTR;
typedef const char* PCSTR;
typedef wchar_t WCHAR;
typedef wchar_t* LPWSTR;
typedef wchar_t* PWSTR;
typedef const wchar_t* LPCWSTR;
typedef const wchar_t* PCWSTR;

#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

typedef DWORD* LPDWORD;
typedef UINT* PUINT;

typedef int32_t HRESULT;
#define S_OK ((HRESULT)0L)
#define S_FALSE ((HRESULT)1L)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_INVALIDARG ((HRESULT)0x80070057L)
#define E_NOTIMPL ((HRESULT)0x80004001L)
#define E_NOINTERFACE ((HRESULT)0x80004002L)
#define E_POINTER ((HRESULT)0x80004003L)
#define E_ABORT ((HRESULT)0x80004004L)
#define E_UNEXPECTED ((HRESULT)0x8000FFFFL)

#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr) (((HRESULT)(hr)) < 0)

#ifndef MAKE_HRESULT
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT)(((uint32_t)(sev)<<31) | ((uint32_t)(fac)<<16) | ((uint32_t)(code))))
#endif

#define MAX_PATH 260
#define _MAX_PATH 260
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256

#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define DLL_PROCESS_DETACH 0

inline BOOL DisableThreadLibraryCalls(HMODULE) { return TRUE; }

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[MAX_PATH];
    CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

struct WinEvent {
    bool manual_reset;
    bool signaled;
    std::mutex mtx;
    std::condition_variable cv;
};

inline HANDLE CreateEventA(void*, BOOL bManualReset, BOOL bInitialState, const char*) {
    auto ev = new WinEvent();
    ev->manual_reset = (bManualReset != 0);
    ev->signaled = (bInitialState != 0);
    return (HANDLE)ev;
}
inline HANDLE CreateEventW(void* p, BOOL bManualReset, BOOL bInitialState, const wchar_t*) {
    return CreateEventA(p, bManualReset, bInitialState, nullptr);
}
inline BOOL SetEvent(HANDLE hEvent) {
    if (!hEvent) return FALSE;
    auto ev = (WinEvent*)hEvent;
    {
        std::lock_guard<std::mutex> lock(ev->mtx);
        ev->signaled = true;
    }
    if (ev->manual_reset) {
        ev->cv.notify_all();
    } else {
        ev->cv.notify_one();
    }
    return TRUE;
}
inline BOOL ResetEvent(HANDLE hEvent) {
    if (!hEvent) return FALSE;
    auto ev = (WinEvent*)hEvent;
    std::lock_guard<std::mutex> lock(ev->mtx);
    ev->signaled = false;
    return TRUE;
}

#define INFINITE 0xFFFFFFFF
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258
#define WAIT_FAILED 0xFFFFFFFF

inline DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
    if (!hHandle) return WAIT_FAILED;
    auto ev = (WinEvent*)hHandle;
    std::unique_lock<std::mutex> lock(ev->mtx);
    if (dwMilliseconds == INFINITE) {
        ev->cv.wait(lock, [&]() { return ev->signaled; });
        if (!ev->manual_reset) {
            ev->signaled = false;
        }
        return WAIT_OBJECT_0;
    } else {
        bool res = ev->cv.wait_for(lock, std::chrono::milliseconds(dwMilliseconds), [&]() { return ev->signaled; });
        if (res) {
            if (!ev->manual_reset) {
                ev->signaled = false;
            }
            return WAIT_OBJECT_0;
        }
        return WAIT_TIMEOUT;
    }
}

inline BOOL CloseHandle(HANDLE hObject) {
    (void)hObject;
    return TRUE;
}

inline HANDLE GetProcessHeap() { return (HANDLE)1; }
inline LPVOID HeapAlloc(HANDLE, DWORD, size_t dwBytes) { return std::malloc(dwBytes); }
inline BOOL HeapFree(HANDLE, DWORD, LPVOID lpMem) { std::free(lpMem); return TRUE; }

inline BOOL UnmapViewOfFile(LPCVOID lpBaseAddress) {
    (void)lpBaseAddress;
    return TRUE;
}

inline BOOL GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) {
    (void)hFile;
    if (lpCreationTime) { lpCreationTime->dwLowDateTime = 0; lpCreationTime->dwHighDateTime = 0; }
    if (lpLastAccessTime) { lpLastAccessTime->dwLowDateTime = 0; lpLastAccessTime->dwHighDateTime = 0; }
    if (lpLastWriteTime) { lpLastWriteTime->dwLowDateTime = 0; lpLastWriteTime->dwHighDateTime = 0; }
    return TRUE;
}

typedef void* LPSECURITY_ATTRIBUTES;

#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2

#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define FILE_ATTRIBUTE_NORMAL    0x00000080
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#define FILE_FLAG_RANDOM_ACCESS   0x10000000

#define PAGE_READONLY  0x02
#define PAGE_READWRITE 0x04
#define FILE_MAP_READ  0x04

#define GENERIC_READ    0x80000000
#define GENERIC_WRITE   0x40000000
#define FILE_SHARE_READ 0x00000001
#define OPEN_EXISTING   3
#define CREATE_ALWAYS   2

inline DWORD GetLastError() {
    return static_cast<DWORD>(errno);
}

inline void SetLastError(DWORD dwErrCode) {
    errno = static_cast<int>(dwErrCode);
}

inline DWORD GetFileAttributesA(LPCSTR lpFileName) {
    if (!lpFileName) return INVALID_FILE_ATTRIBUTES;
    struct stat st;
    if (stat(lpFileName, &st) != 0) {
        return INVALID_FILE_ATTRIBUTES;
    }
    if (S_ISDIR(st.st_mode)) {
        return FILE_ATTRIBUTE_DIRECTORY;
    }
    return FILE_ATTRIBUTE_NORMAL;
}

#define _access access

#define MEM_COMMIT  0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_RELEASE 0x00008000

inline void* VirtualAlloc(void* lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect) {
    (void)lpAddress; (void)flAllocationType; (void)flProtect;
    void* ptr = std::malloc(dwSize);
    if (ptr) std::memset(ptr, 0, dwSize);
    return ptr;
}

inline BOOL VirtualFree(void* lpAddress, size_t dwSize, DWORD dwFreeType) {
    (void)dwSize; (void)dwFreeType;
    std::free(lpAddress);
    return TRUE;
}

inline char* _fullpath(char* absPath, const char* relPath, size_t maxLength) {
    if (!relPath) return nullptr;
    char resolved[PATH_MAX];
    if (realpath(relPath, resolved)) {
        if (absPath) {
            std::strncpy(absPath, resolved, maxLength);
            return absPath;
        }
        return strdup(resolved);
    }
    // Fallback if file does not exist yet
    if (absPath) {
        std::strncpy(absPath, relPath, maxLength);
        return absPath;
    }
    return strdup(relPath);
}

inline HANDLE FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
    (void)lpFileName;
    if (lpFindFileData) std::memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATAA));
    return INVALID_HANDLE_VALUE;
}

inline BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData) {
    (void)hFindFile; (void)lpFindFileData;
    return FALSE;
}

inline BOOL FindClose(HANDLE hFindFile) {
    (void)hFindFile;
    return TRUE;
}

template<typename T>
inline DWORD GetFileSize(HANDLE hFile, T* lpFileSizeHigh) {
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return (DWORD)-1;
    int fd = (int)(intptr_t)hFile;
    struct stat st;
    if (fstat(fd, &st) != 0) return (DWORD)-1;
    if (lpFileSizeHigh) *lpFileSizeHigh = (T)(st.st_size >> 32);
    return (DWORD)(st.st_size & 0xFFFFFFFF);
}

inline DWORD GetFileSize(HANDLE hFile, std::nullptr_t) {
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return (DWORD)-1;
    int fd = (int)(intptr_t)hFile;
    struct stat st;
    if (fstat(fd, &st) != 0) return (DWORD)-1;
    return (DWORD)(st.st_size & 0xFFFFFFFF);
}

inline BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void* lpOverlapped) {
    (void)lpOverlapped;
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return FALSE;
    int fd = (int)(intptr_t)hFile;
    ssize_t res = read(fd, lpBuffer, nNumberOfBytesToRead);
    if (res < 0) return FALSE;
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = (DWORD)res;
    return TRUE;
}

inline DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return (DWORD)-1;
    int fd = (int)(intptr_t)hFile;
    int whence = SEEK_SET;
    if (dwMoveMethod == FILE_CURRENT) whence = SEEK_CUR;
    else if (dwMoveMethod == FILE_END) whence = SEEK_END;
    off_t res = lseek(fd, lDistanceToMove, whence);
    if (res == (off_t)-1) return (DWORD)-1;
    return (DWORD)res;
}

inline HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    (void)dwShareMode; (void)lpSecurityAttributes; (void)dwFlagsAndAttributes; (void)hTemplateFile;
    int flags = 0;
    if ((dwDesiredAccess & (GENERIC_READ | GENERIC_WRITE)) == (GENERIC_READ | GENERIC_WRITE)) flags = O_RDWR;
    else if (dwDesiredAccess & GENERIC_WRITE) flags = O_WRONLY;
    else flags = O_RDONLY;

    if (dwCreationDisposition == CREATE_ALWAYS) flags |= O_CREAT | O_TRUNC;

    int fd = open(lpFileName, flags, 0666);
    if (fd < 0) return INVALID_HANDLE_VALUE;
    return (HANDLE)(intptr_t)fd;
}

inline HANDLE CreateFileMappingA(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName) {
    (void)lpFileMappingAttributes; (void)flProtect; (void)dwMaximumSizeHigh; (void)dwMaximumSizeLow; (void)lpName;
    return hFile;
}

inline LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, size_t dwNumberOfBytesToMap) {
    (void)dwDesiredAccess; (void)dwFileOffsetHigh;
    if (hFileMappingObject == INVALID_HANDLE_VALUE || !hFileMappingObject) return nullptr;
    int fd = (int)(intptr_t)hFileMappingObject;
    if (dwNumberOfBytesToMap == 0) {
        struct stat st;
        if (fstat(fd, &st) != 0) return nullptr;
        dwNumberOfBytesToMap = (size_t)st.st_size;
    }
    void* ptr = mmap(nullptr, dwNumberOfBytesToMap, PROT_READ, MAP_SHARED, fd, (off_t)dwFileOffsetLow);
    if (ptr == MAP_FAILED) return nullptr;
    return ptr;
}

inline BOOL DosDateTimeToFileTime(WORD wFatDate, WORD wFatTime, LPFILETIME lpFileTime) {
    if (!lpFileTime) return FALSE;
    lpFileTime->dwLowDateTime = 0;
    lpFileTime->dwHighDateTime = 0;
    return TRUE;
}

// Dialog Box return values
#define IDOK 1
#define IDCANCEL 2
#define IDABORT 3
#define IDRETRY 4
#define IDIGNORE 5
#define IDYES 6
#define IDNO 7

#define _stricmp strcasecmp
#define stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _wcsicmp wcscasecmp

#define _strdup strdup
#define wsprintfA sprintf
#define wsprintf sprintf

inline int _swprintf(wchar_t* buffer, const wchar_t* format, ...) {
    va_list args;
    va_start(args, format);
    int res = vswprintf(buffer, 2048, format, args);
    va_end(args);
    return res;
}

inline char* _itoa(int value, char* str, int radix) {
    if (radix == 10) {
        std::sprintf(str, "%d", value);
    } else if (radix == 16) {
        std::sprintf(str, "%x", value);
    } else if (radix == 8) {
        std::sprintf(str, "%o", value);
    } else {
        std::sprintf(str, "%d", value);
    }
    return str;
}

inline char* _strlwr(char* str) {
    if (!str) return nullptr;
    for (char* p = str; *p; ++p) *p = static_cast<char>(std::tolower(static_cast<unsigned char>(*p)));
    return str;
}

inline char* _strupr(char* str) {
    if (!str) return nullptr;
    for (char* p = str; *p; ++p) *p = static_cast<char>(std::toupper(static_cast<unsigned char>(*p)));
    return str;
}

#define _vsnprintf vsnprintf
#define _snprintf snprintf

#define __debugbreak() __builtin_trap()

// Disable 32-bit retail binary-matching layout assertions when compiling for 64-bit platforms
#if !defined(_M_IX86) && !defined(__i386__)
#undef static_assert
#define static_assert(...)
#endif

typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *LPRECT;

inline BOOL SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom) {
    if (!lprc) return FALSE;
    lprc->left = xLeft; lprc->top = yTop; lprc->right = xRight; lprc->bottom = yBottom;
    return TRUE;
}

typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;

typedef struct _RGNDATAHEADER {
    DWORD dwSize;
    DWORD iType;
    DWORD nCount;
    DWORD nRgnSize;
    RECT  rcBound;
} RGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER rdh;
    char          Buffer[1];
} RGNDATA, *PRGNDATA, *NPRGNDATA, *LPRGNDATA;

typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT, *LPPOINT;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *PSIZE, *LPSIZE;

// GUID / IID / CLSID
typedef struct _GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} GUID, IID, CLSID;

typedef GUID *LPGUID;
typedef const GUID *LPCGUID;

#define REFGUID const GUID &
#define REFIID const IID &
#define REFCLSID const CLSID &

#ifndef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    inline const GUID name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }
#endif

#ifndef __MSABI_LONG
#define __MSABI_LONG(x) (x)
#endif

#ifndef DECL_WINELIB_TYPE_AW
#define DECL_WINELIB_TYPE_AW(type) typedef type##A type;
#endif

typedef struct tWAVEFORMATEX {
    WORD        wFormatTag;
    WORD        nChannels;
    DWORD       nSamplesPerSec;
    DWORD       nAvgBytesPerSec;
    WORD        nBlockAlign;
    WORD        wBitsPerSample;
    WORD        cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX *LPCWAVEFORMATEX;

#define STDMETHODCALLTYPE __stdcall
#define STDMETHODVCALLTYPE __cdecl
#define PVOID void*

#ifndef STDMETHOD
#define STDMETHOD(method)        virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method)  virtual type STDMETHODCALLTYPE method
#define STDMETHODV(method)       virtual HRESULT STDMETHODVCALLTYPE method
#define STDMETHODV_(type,method) virtual type STDMETHODVCALLTYPE method
#define PURE                     = 0
#define THIS_
#define THIS                     void
#endif

#ifndef DECLARE_INTERFACE
#define DECLARE_INTERFACE(iface)                         struct iface
#define DECLARE_INTERFACE_(iface, baseiface)             struct iface : public baseiface
#define DECLARE_INTERFACE_IID(iface, iid)                struct iface
#define DECLARE_INTERFACE_IID_(iface, baseiface, iid)    struct iface : public baseiface
#endif

struct IUnknown;
typedef IUnknown *LPUNKNOWN;

#define CLSCTX_INPROC_SERVER 0x1
#define CLSCTX_INPROC_HANDLER 0x2
#define CLSCTX_LOCAL_SERVER 0x4
#define CLSCTX_REMOTE_SERVER 0x10
#define CLSCTX_ALL (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER|CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER)

inline HRESULT CoInitialize(void*) { return S_OK; }
inline void CoUninitialize() {}
inline HRESULT CoCreateInstance(REFCLSID, IUnknown*, DWORD, REFIID, void** ppv) {
    if (ppv) *ppv = nullptr;
    return E_FAIL;
}

inline bool operator==(const GUID& a, const GUID& b) {
    return std::memcmp(&a, &b, sizeof(GUID)) == 0;
}
inline bool operator!=(const GUID& a, const GUID& b) {
    return !(a == b);
}

#define STDMETHODCALLTYPE __stdcall
#define PVOID void*

struct IUnknown {
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(const GUID& riid, void** ppvObject) = 0;
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;
};

struct IClassFactory : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) = 0;
    virtual HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) = 0;
};

#define CREATE_SUSPENDED 0x00000004
typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);

struct WinThread {
    std::thread th;
    LPTHREAD_START_ROUTINE func = nullptr;
    LPVOID param = nullptr;
    bool suspended = false;
    std::mutex mtx;
    std::condition_variable cv;
    bool started = false;
};

inline HANDLE CreateThread(void*, size_t, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    auto wt = new WinThread();
    wt->func = lpStartAddress;
    wt->param = lpParameter;
    wt->suspended = (dwCreationFlags & CREATE_SUSPENDED) != 0;
    if (lpThreadId) *lpThreadId = 1;
    wt->th = std::thread([wt]() {
        if (wt->suspended) {
            std::unique_lock<std::mutex> lock(wt->mtx);
            wt->cv.wait(lock, [&]() { return wt->started; });
        }
        if (wt->func) wt->func(wt->param);
    });
    if (!wt->suspended) {
        wt->th.detach();
    }
    return (HANDLE)wt;
}

inline DWORD ResumeThread(HANDLE hThread) {
    if (!hThread) return (DWORD)-1;
    auto wt = (WinThread*)hThread;
    {
        std::lock_guard<std::mutex> lock(wt->mtx);
        wt->started = true;
    }
    wt->cv.notify_one();
    wt->th.detach();
    return 0;
}

// CRITICAL_SECTION: MUST be recursive mutex to preserve Win32 semantics
typedef struct _CRITICAL_SECTION {
    void* DebugInfo;
    long LockCount;
    long RecursionCount;
    void* OwningThread;
    void* LockSemaphore;
    uintptr_t SpinCount;
    std::recursive_mutex Mutex;
} CRITICAL_SECTION, *PCRITICAL_SECTION, *LPCRITICAL_SECTION;

inline void InitializeCriticalSection(LPCRITICAL_SECTION cs) { (void)cs; }
inline void DeleteCriticalSection(LPCRITICAL_SECTION cs) { (void)cs; }
inline void EnterCriticalSection(LPCRITICAL_SECTION cs) { if (cs) cs->Mutex.lock(); }
inline void LeaveCriticalSection(LPCRITICAL_SECTION cs) { if (cs) cs->Mutex.unlock(); }
inline BOOL TryEnterCriticalSection(LPCRITICAL_SECTION cs) { return cs ? (cs->Mutex.try_lock() ? 1 : 0) : 0; }

// Time and Clock helpers
inline DWORD timeGetTime() {
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return static_cast<DWORD>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG  HighPart;
    };
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

inline BOOL QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) {
    if (!lpPerformanceCount) return FALSE;
    using namespace std::chrono;
    lpPerformanceCount->QuadPart = high_resolution_clock::now().time_since_epoch().count();
    return TRUE;
}

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency) {
    if (!lpFrequency) return FALSE;
    using namespace std::chrono;
    lpFrequency->QuadPart = high_resolution_clock::period::den / high_resolution_clock::period::num;
    return TRUE;
}

inline void Sleep(DWORD dwMilliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(dwMilliseconds));
}

inline void OutputDebugStringA(LPCSTR lpOutputString) {
    if (lpOutputString) {
        std::fputs(lpOutputString, stderr);
    }
}

inline int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    (void)hWnd; (void)uType;
    std::fprintf(stderr, "[MessageBox %s]: %s\n", lpCaption ? lpCaption : "Alert", lpText ? lpText : "");
    return IDOK;
}

inline void PostQuitMessage(int nExitCode) {
    std::exit(nExitCode);
}

inline DWORD SearchPathA(LPCSTR lpPath, LPCSTR lpFileName, LPCSTR lpExtension, DWORD nBufferLength, LPSTR lpBuffer, LPSTR* lpFilePart) {
    (void)lpExtension;
    std::string full;
    if (lpPath && lpPath[0]) {
        full = std::string(lpPath) + "/" + lpFileName;
    } else {
        full = lpFileName;
    }
    if (lpBuffer && nBufferLength > full.size()) {
        std::strcpy(lpBuffer, full.c_str());
        if (lpFilePart) *lpFilePart = lpBuffer;
        return static_cast<DWORD>(full.size());
    }
    return 0;
}

inline HMODULE GetModuleHandleA(LPCSTR lpModuleName) {
    (void)lpModuleName;
    return (HMODULE)(intptr_t)1;
}

inline DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
    (void)hModule;
    if (lpFilename && nSize > 0) {
        std::strncpy(lpFilename, "./freelancer", nSize);
        return static_cast<DWORD>(std::strlen(lpFilename));
    }
    return 0;
}

inline void _makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext) {
    (void)drive;
    std::string res;
    if (dir && dir[0]) res += dir;
    if (fname && fname[0]) res += fname;
    if (ext && ext[0]) res += ext;
    if (path) std::strcpy(path, res.c_str());
}

inline void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext) {
    if (drive) drive[0] = '\0';
    if (!path) {
        if (dir) dir[0] = '\0';
        if (fname) fname[0] = '\0';
        if (ext) ext[0] = '\0';
        return;
    }

    std::string s(path);
    size_t last_slash = s.find_last_of("/\\");
    std::string directory, filename_with_ext;
    if (last_slash != std::string::npos) {
        directory = s.substr(0, last_slash + 1);
        filename_with_ext = s.substr(last_slash + 1);
    } else {
        directory = "";
        filename_with_ext = s;
    }

    size_t last_dot = filename_with_ext.find_last_of('.');
    std::string filename, extension;
    if (last_dot != std::string::npos) {
        filename = filename_with_ext.substr(0, last_dot);
        extension = filename_with_ext.substr(last_dot);
    } else {
        filename = filename_with_ext;
        extension = "";
    }

    if (dir) std::strcpy(dir, directory.c_str());
    if (fname) std::strcpy(fname, filename.c_str());
    if (ext) std::strcpy(ext, extension.c_str());
}

typedef struct tagVS_FIXEDFILEINFO {
    DWORD dwSignature;
    DWORD dwStrucVersion;
    DWORD dwFileVersionMS;
    DWORD dwFileVersionLS;
    DWORD dwProductVersionMS;
    DWORD dwProductVersionLS;
    DWORD dwFileFlagsMask;
    DWORD dwFileFlags;
    DWORD dwFileOS;
    DWORD dwFileType;
    DWORD dwFileSubtype;
    DWORD dwFileDateMS;
    DWORD dwFileDateLS;
} VS_FIXEDFILEINFO;

inline DWORD GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
    (void)lptstrFilename;
    if (lpdwHandle) *lpdwHandle = 0;
    return sizeof(VS_FIXEDFILEINFO);
}

inline BOOL GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    (void)lptstrFilename; (void)dwHandle;
    if (lpData && dwLen >= sizeof(VS_FIXEDFILEINFO)) {
        VS_FIXEDFILEINFO* info = (VS_FIXEDFILEINFO*)lpData;
        std::memset(info, 0, sizeof(VS_FIXEDFILEINFO));
        info->dwFileVersionMS = (1 << 16) | 1; // 1.1
        return TRUE;
    }
    return FALSE;
}

inline BOOL VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID* lplpBuffer, PUINT puLen) {
    (void)lpSubBlock;
    if (lplpBuffer) *lplpBuffer = (LPVOID)pBlock;
    if (puLen) *puLen = sizeof(VS_FIXEDFILEINFO);
    return TRUE;
}

// Dynamic library loading shims
#include <dlfcn.h>
inline HMODULE LoadLibraryA(LPCSTR lpLibFileName) {
    return dlopen(lpLibFileName, RTLD_NOW | RTLD_LOCAL);
}
inline void* GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    return dlsym(hModule, lpProcName);
}
inline BOOL FreeLibrary(HMODULE hLibModule) {
    return dlclose(hLibModule) == 0 ? TRUE : FALSE;
}

#endif // !_WIN32
