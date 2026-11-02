// win_compat.h - High-fidelity Win32 compatibility layer for macOS / POSIX
#pragma once

#ifndef _WIN32

#include <cstdint>
#if defined(__APPLE__)
#  include <mach-o/dyld.h>
#endif
#include "fl/handles.h"
#include "fl/platform.h"
#include <execinfo.h>
#include <vector>
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
#ifndef THREAD_PRIORITY_HIGHEST
#define THREAD_PRIORITY_LOWEST (-2)
#define THREAD_PRIORITY_BELOW_NORMAL (-1)
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_HIGHEST 2
#endif
typedef BYTE* LPBYTE;
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

// Polling implementation; the handles passed in are WinEvent objects.
inline DWORD WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) {
    auto start = std::chrono::steady_clock::now();
    for (;;) {
        DWORD signaledCount = 0;
        for (DWORD i = 0; i < nCount; ++i) {
            auto ev = (WinEvent*)lpHandles[i];
            std::unique_lock<std::mutex> lock(ev->mtx);
            if (ev->signaled) {
                ++signaledCount;
                if (!bWaitAll) {
                    if (!ev->manual_reset) ev->signaled = false;
                    return WAIT_OBJECT_0 + i;
                }
            }
        }
        if (bWaitAll && signaledCount == nCount) {
            for (DWORD i = 0; i < nCount; ++i) {
                auto ev = (WinEvent*)lpHandles[i];
                std::unique_lock<std::mutex> lock(ev->mtx);
                if (!ev->manual_reset) ev->signaled = false;
            }
            return WAIT_OBJECT_0;
        }
        if (dwMilliseconds != INFINITE &&
            std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(dwMilliseconds))
            return WAIT_TIMEOUT;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

// Win32 paths -> host paths (backslashes, case) via fl::IFileSystem::ResolvePath.
struct FlPathBuf {
    char s[1024];
    FlPathBuf(const char* in) { fl::GetPlatform().files->ResolvePath(in ? in : "", s, sizeof(s)); }
    operator const char*() const { return s; }
};
inline int FlAccess(const char* p, int mode) { return access(FlPathBuf(p), mode); }
#define _access FlAccess

inline DWORD GetFileAttributesA(LPCSTR lpFileName) {
    if (!lpFileName) return INVALID_FILE_ATTRIBUTES;
    struct stat st;
    if (stat(FlPathBuf(lpFileName), &st) != 0) {
        return INVALID_FILE_ATTRIBUTES;
    }
    if (S_ISDIR(st.st_mode)) {
        return FILE_ATTRIBUTE_DIRECTORY;
    }
    return FILE_ATTRIBUTE_NORMAL;
}


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

struct FlFindCtx { void* h; };
inline void FlFillFindData(const fl::DirEntry& e, LPWIN32_FIND_DATAA fd) {
    memset(fd, 0, sizeof(*fd));
    fd->dwFileAttributes = e.isDirectory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    fd->nFileSizeLow = (DWORD)e.size;
    strncpy(fd->cFileName, e.name, sizeof(fd->cFileName) - 1);
}
inline HANDLE FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
    fl::DirEntry e;
    FlPathBuf spec(lpFileName);
    void* h = fl::GetPlatform().files->FindFirst(spec, &e);
    if (!h) return INVALID_HANDLE_VALUE;
    FlFillFindData(e, lpFindFileData);
    return new FlFindCtx{h};
}
inline BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData) {
    fl::DirEntry e;
    if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE) return FALSE;
    if (!fl::GetPlatform().files->FindNext(((FlFindCtx*)hFindFile)->h, &e)) return FALSE;
    FlFillFindData(e, lpFindFileData);
    return TRUE;
}
inline BOOL FindClose(HANDLE hFindFile) {
    if (!hFindFile || hFindFile == INVALID_HANDLE_VALUE) return FALSE;
    fl::GetPlatform().files->FindClose(((FlFindCtx*)hFindFile)->h);
    delete (FlFindCtx*)hFindFile;
    return TRUE;
}

template<typename T>
inline DWORD GetFileSize(HANDLE hFile, T* lpFileSizeHigh) {
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return (DWORD)-1;
    int fd = (int)fl::PtrToHandle(hFile);
    struct stat st;
    if (fstat(fd, &st) != 0) return (DWORD)-1;
    if (lpFileSizeHigh) *lpFileSizeHigh = (T)(st.st_size >> 32);
    return (DWORD)(st.st_size & 0xFFFFFFFF);
}

inline DWORD GetFileSize(HANDLE hFile, std::nullptr_t) {
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return (DWORD)-1;
    int fd = (int)fl::PtrToHandle(hFile);
    struct stat st;
    if (fstat(fd, &st) != 0) return (DWORD)-1;
    return (DWORD)(st.st_size & 0xFFFFFFFF);
}

inline BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, void* lpOverlapped) {
    (void)lpOverlapped;
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return FALSE;
    int fd = (int)fl::PtrToHandle(hFile);
    ssize_t res = read(fd, lpBuffer, nNumberOfBytesToRead);
    if (res < 0) return FALSE;
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = (DWORD)res;
    return TRUE;
}

inline DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    if (hFile == INVALID_HANDLE_VALUE || !hFile) return (DWORD)-1;
    int fd = (int)fl::PtrToHandle(hFile);
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

    int fd = open(FlPathBuf(lpFileName), flags, 0666);
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
    int fd = (int)fl::PtrToHandle(hFileMappingObject);
    if (dwNumberOfBytesToMap == 0) {
        struct stat st;
        if (fstat(fd, &st) != 0) return nullptr;
        dwNumberOfBytesToMap = (size_t)st.st_size;
    }
    void* ptr = mmap(nullptr, dwNumberOfBytesToMap, PROT_READ, MAP_SHARED, fd, (off_t)dwFileOffsetLow);
    if (ptr == MAP_FAILED) { if (getenv("FL_TRACE_FILES")) fprintf(stderr, "[fl] mmap(fd=%d,len=%zu) failed: %s\n", fd, dwNumberOfBytesToMap, strerror(errno)); return nullptr; }
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
#define _wcsnicmp wcsncasecmp
#define wcsicmp wcscasecmp
#define _wcsdup wcsdup

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

#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif
// ---- MSVC "secure" CRT (no-op size checks beyond truncation) --------------------------
#include <string>
typedef int errno_t;
inline int sprintf_s(char* buf, size_t n, const char* fmt, ...) {
    va_list a; va_start(a, fmt); int r = vsnprintf(buf, n, fmt, a); va_end(a); return r;
}
template <size_t N>
inline int sprintf_s(char (&buf)[N], const char* fmt, ...) {
    va_list a; va_start(a, fmt); int r = vsnprintf(buf, N, fmt, a); va_end(a); return r;
}
inline errno_t strncpy_s(char* dst, size_t size, const char* src, size_t count) {
    if (!dst || size == 0) return EINVAL;
    size_t n = count == _TRUNCATE ? size - 1 : count;
    if (n > size - 1) n = size - 1;
    size_t i = 0;
    for (; i < n && src[i]; ++i) dst[i] = src[i];
    dst[i] = 0;
    return 0;
}
template <size_t N>
inline errno_t strncpy_s(char (&dst)[N], const char* src, size_t count) { return strncpy_s(dst, N, src, count); }
inline errno_t strcpy_s(char* dst, size_t size, const char* src) { return strncpy_s(dst, size, src, _TRUNCATE); }
template <size_t N>
inline errno_t strcpy_s(char (&dst)[N], const char* src) { return strncpy_s(dst, N, src, _TRUNCATE); }
inline errno_t strcat_s(char* dst, size_t size, const char* src) {
    size_t l = strlen(dst); if (l >= size) return EINVAL;
    return strncpy_s(dst + l, size - l, src, _TRUNCATE);
}
// NOTE: MSVC sscanf_s takes an extra size argument after %s/%c; callers using those
// conversions need individual attention. Numeric conversions are identical.
#define sscanf_s sscanf
inline errno_t fopen_s(FILE** f, const char* name, const char* mode) {
    *f = fopen(FlPathBuf(name), mode); return *f ? 0 : errno;
}
inline FILE* FlFopen(const char* name, const char* mode) { return fopen(FlPathBuf(name), mode); }
namespace std { using ::FlFopen; }
#define fopen(name, mode) FlFopen(name, mode)
inline int FlUnlink(const char* p) { return unlink(FlPathBuf(p)); }
#define _unlink FlUnlink
inline int _memicmp(const void* A, const void* B, size_t Count) {
    const unsigned char* PA = static_cast<const unsigned char*>(A);
    const unsigned char* PB = static_cast<const unsigned char*>(B);
    for (size_t i = 0; i < Count; ++i) {
        int ca = PA[i], cb = PB[i];
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return ca < cb ? -1 : 1;
    }
    return 0;
}
inline errno_t _makepath_s(char* path, size_t size, const char* drive, const char* dir,
                           const char* fname, const char* ext) {
    std::string s;
    if (drive && *drive) s += drive;
    if (dir && *dir) { s += dir; char c = s.back(); if (c != '/' && c != '\\') s += '\\'; }
    if (fname) s += fname;
    if (ext && *ext) { if (*ext != '.') s += '.'; s += ext; }
    return strncpy_s(path, size, s.c_str(), _TRUNCATE);
}

// ---- _findfirst/_findnext (io.h) -> fl::IFileSystem ------------------------------------
#define _A_SUBDIR 0x10
#define _A_NORMAL 0x00
struct _finddata_t { unsigned attrib; long long time_create, time_access, time_write; unsigned long size; char name[260]; };
inline void FlFillFindData(const fl::DirEntry& e, _finddata_t* fd) {
    strncpy_s(fd->name, sizeof(fd->name), e.name, _TRUNCATE);
    fd->attrib = e.isDirectory ? _A_SUBDIR : _A_NORMAL;
    fd->size = (unsigned long)e.size;
}
struct FlFindHandle { void* h; };
inline intptr_t _findfirst(const char* spec, _finddata_t* fd) {
    fl::DirEntry e;
    void* h = fl::GetPlatform().files->FindFirst(spec, &e);
    if (!h) return -1;
    FlFillFindData(e, fd);
    return (intptr_t)h;
}
inline int _findnext(intptr_t h, _finddata_t* fd) {
    fl::DirEntry e;
    if (!fl::GetPlatform().files->FindNext((void*)h, &e)) return -1;
    FlFillFindData(e, fd);
    return 0;
}
inline int _findclose(intptr_t h) { fl::GetPlatform().files->FindClose((void*)h); return 0; }

// ---- SYSTEMTIME ---------------------------------------------------------------------
typedef struct _SYSTEMTIME { WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds; } SYSTEMTIME, *LPSYSTEMTIME;
inline void FlToSystemTime(const fl::DateTime& d, SYSTEMTIME* st) {
    st->wYear = d.year; st->wMonth = d.month; st->wDayOfWeek = d.dayOfWeek; st->wDay = d.day;
    st->wHour = d.hour; st->wMinute = d.minute; st->wSecond = d.second; st->wMilliseconds = d.millisecond;
}
inline void GetLocalTime(SYSTEMTIME* st) { fl::DateTime d; fl::GetPlatform().clock->LocalTime(&d); FlToSystemTime(d, st); }
inline void GetSystemTime(SYSTEMTIME* st) { fl::DateTime d; fl::GetPlatform().clock->UtcTime(&d); FlToSystemTime(d, st); }

// ---- x87 control word (no-op: arm64 has no precision control) -------------------------
#define _MCW_PC 0x00030000
#define _PC_24 0x00020000
#define _PC_53 0x00010000
#define _PC_64 0x00000000
#define _MCW_RC 0x00000300
#define _MCW_EM 0x0008001F
inline unsigned int _control87(unsigned int, unsigned int) { return 0; }
inline unsigned int _controlfp(unsigned int, unsigned int) { return 0; }



// ---- misc CRT / Win32 bits used by the game-logic layer -------------------------------
#include <ctime>
#include <wctype.h>
#include <fstream>
#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif
#define MB_ABORTRETRYIGNORE 0x00000002L
#define ERROR_FILE_NOT_FOUND 2L
#define CT_CTYPE1 1
#define C1_SPACE 0x0008
typedef void* HGDIOBJ;
inline HGDIOBJ SelectObject(void*, HGDIOBJ) { return nullptr; }
inline BOOL DeleteObject(HGDIOBJ) { return TRUE; }
inline BOOL GetStringTypeW(DWORD, const wchar_t* s, int n, WORD* out) {
    for (int i = 0; i < n; ++i) out[i] = iswspace((wint_t)s[i]) ? C1_SPACE : 0;
    return TRUE;
}
#define _copysign copysign
inline char* _ultoa(unsigned long value, char* str, int radix) {
    if (radix == 16) std::sprintf(str, "%lx", value);
    else if (radix == 8) std::sprintf(str, "%lo", value);
    else std::sprintf(str, "%lu", value);
    return str;
}
inline wchar_t* _wcslwr(wchar_t* s) { for (wchar_t* p = s; p && *p; ++p) *p = (wchar_t)towlower((wint_t)*p); return s; }
inline wchar_t* _wcsupr(wchar_t* s) { for (wchar_t* p = s; p && *p; ++p) *p = (wchar_t)towupper((wint_t)*p); return s; }
inline int _vswprintf(wchar_t* buffer, const wchar_t* format, va_list args) { return vswprintf(buffer, 2048, format, args); }
#define _tzset tzset
inline char* _strtime(char* buf) { fl::DateTime d; fl::GetPlatform().clock->LocalTime(&d); snprintf(buf, 9, "%02u:%02u:%02u", d.hour, d.minute, d.second); return buf; }
inline char* _strdate(char* buf) { fl::DateTime d; fl::GetPlatform().clock->LocalTime(&d); snprintf(buf, 9, "%02u/%02u/%02u", d.month, d.day, d.year % 100); return buf; }
inline BOOL CopyFileA(const char* from, const char* to, BOOL failIfExists) {
    return fl::GetPlatform().files->CopyFile(FlPathBuf(from), FlPathBuf(to), failIfExists != 0) ? TRUE : FALSE;
}


// Disable 32-bit retail binary-matching layout assertions when compiling for 64-bit platforms
#if !defined(_M_IX86) && !defined(__i386__) && !defined(FL_STRICT_LAYOUT)
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

// GDI font types used (by value/pointer only) in common_text.h
struct HFONT__;
typedef HFONT__* HFONT;
typedef struct tagTEXTMETRICA {
    LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading,
         tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang,
         tmDigitizedAspectX, tmDigitizedAspectY;
    BYTE tmFirstChar, tmLastChar, tmDefaultChar, tmBreakChar;
    BYTE tmItalic, tmUnderlined, tmStruckOut, tmPitchAndFamily, tmCharSet;
} TEXTMETRICA, *LPTEXTMETRICA;
typedef struct tagLOGFONTA {
    LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
    BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision, lfClipPrecision,
         lfQuality, lfPitchAndFamily;
    char lfFaceName[32];
} LOGFONTA, *LPLOGFONTA;

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

#ifndef ERROR_INVALID_ACCESS
#define ERROR_INVALID_ACCESS 12L
#endif
#ifndef REGDB_E_CLASSNOTREG
#define REGDB_E_CLASSNOTREG ((HRESULT)0x80040154L)
#endif
#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#endif
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
inline BOOL SetThreadPriority(HANDLE, int) { return TRUE; }

inline DWORD GetTickCount() {
    return (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

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
    if (getenv("FL_TRACE_MESSAGEBOX")) {          // porting aid: who raised this dialog?
        void* frames[24];
        int n = backtrace(frames, 24);
        backtrace_symbols_fd(frames, n, 2);
    }
    return IDOK;
}

inline void PostQuitMessage(int nExitCode) {
    fl::GetWindowHost()->RequestQuit(nExitCode);
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

inline const char* FlModulePath(HMODULE hModule);   // defined with LoadLibraryA
inline DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
    if (!lpFilename || nSize == 0) return 0;
    if (const char* own = FlModulePath(hModule)) {
        std::strncpy(lpFilename, own, nSize);
        lpFilename[nSize - 1] = 0;
        return static_cast<DWORD>(std::strlen(lpFilename));
    }
    char buf[1024]; uint32_t sz = sizeof(buf);
#if defined(__APPLE__)
    if (_NSGetExecutablePath(buf, &sz) != 0) { std::strncpy(buf, "./freelancer", sizeof(buf)); }
#else
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) { std::strncpy(buf, "./freelancer", sizeof(buf)); } else buf[n] = 0;
#endif
    std::strncpy(lpFilename, buf, nSize);
    lpFilename[nSize - 1] = 0;
    return static_cast<DWORD>(std::strlen(lpFilename));
}

// CRT semantics: drive (+ ':' if missing) + dir (+ '\\' if it doesn't end in a separator) + fname + ext (+ '.' if missing).
inline void _makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext) {
    std::string res;
    if (drive && drive[0]) { res += drive; if (res.back() != ':') res += ':'; }
    if (dir && dir[0]) { res += dir; char c = res.back(); if (c != '/' && c != '\\') res += '\\'; }
    if (fname && fname[0]) res += fname;
    if (ext && ext[0]) { if (ext[0] != '.') res += '.'; res += ext; }
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

// ---- Modules: retail PE resource DLLs (Resources.dll, InfoCards.dll, ...) and native libraries ----
// A module is either a Windows PE file opened read-only for its resources (fl::PeModule) or a
// native shared library (dlopen). PE code is never executed.
#include <dlfcn.h>
#include <deque>
#include <map>
#include <set>
#include "fl/pe_module.h"
typedef void* HRSRC;
typedef void* HGLOBAL;
struct FlResource { const uint8_t* data; uint32_t size; };
struct FlModule {
    fl::PeModule* pe = nullptr;
    void* dl = nullptr;
    std::string path;     // absolute path of the shared library (GetModuleFileName)
    std::deque<FlResource> resources;
};
inline std::set<FlModule*>& FlLiveModules() { static std::set<FlModule*> s; return s; }
inline const char* FlModulePath(HMODULE hModule) {
    FlModule* m = static_cast<FlModule*>(hModule);
    if (!m || !FlLiveModules().count(m)) return nullptr;
    return (m->dl && !m->path.empty()) ? m->path.c_str() : nullptr;
}
// A module is native (our shared library for a former DLL: ./<Name>.dylib, dlopen'ed, DllMain run once)
// or a retail PE file opened read-only for its resources. Native wins; the PE search order mirrors
// Windows' "application directory first", then FL_RESOURCE_DIR, then the retail layout in this
// repository (Cab1/exe).
inline HMODULE LoadLibraryA(LPCSTR lpLibFileName) {
    if (!lpLibFileName) return nullptr;
    std::string base = lpLibFileName;
    size_t cut = base.find_last_of("\\/");
    if (cut != std::string::npos) base = base.substr(cut + 1);
    std::string stem = base;
    if (stem.size() > 4 && strcasecmp(stem.c_str() + stem.size() - 4, ".dll") == 0) stem.resize(stem.size() - 4);
    else if (stem.size() > 6 && strcasecmp(stem.c_str() + stem.size() - 6, ".dylib") == 0) stem.resize(stem.size() - 6);

    static std::map<std::string, FlModule*> loaded;
    auto it = loaded.find(stem);
    if (it != loaded.end()) return it->second;

    // 0. the running executable itself (retail's server "loads" freelancer.exe to reach its GetClientInterface export)
    if (strcasecmp(stem.c_str(), "freelancer") == 0) {
        FlModule* m = new FlModule();
        m->dl = dlopen(nullptr, RTLD_NOW);
        loaded[stem] = m;
        return m;
    }
    // 1. native shared library
    for (const char* dir : {"", "./", "../bin/"}) {
        std::string path = std::string(dir) + stem + ".dylib";
        if (access(path.c_str(), F_OK) != 0) continue;
        if (void* dl = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL)) {
            FlModule* m = new FlModule();
            m->dl = dl;
            FlLiveModules().insert(m);
            {
                char abs[PATH_MAX];
                if (realpath(path.c_str(), abs)) m->path = abs;
            }
            loaded[stem] = m;
            typedef int (*DllMainFn)(void*, unsigned int, void*);
            // Modules define DllMain with C++ linkage (Itanium mangling of DllMain(void*, unsigned, void*)).
            DllMainFn dm = (DllMainFn)dlsym(dl, "_Z7DllMainPvjS_");
            if (!dm) dm = (DllMainFn)dlsym(dl, "DllMain");
            if (dm) dm(m, 1 /*DLL_PROCESS_ATTACH*/, nullptr);
            return m;
        }
        fprintf(stderr, "[fl] dlopen(%s) failed: %s\n", path.c_str(), dlerror());
    }
    // 2. retail PE file (resources only)
    std::vector<std::string> candidates;
    candidates.push_back(lpLibFileName);
    const char* env = getenv("FL_RESOURCE_DIR");
    if (env && *env) candidates.push_back(std::string(env) + "/" + base);
    candidates.push_back("../Cab1/exe/" + base);
    candidates.push_back("Cab1/exe/" + base);
    for (const std::string& c : candidates) {
        FlPathBuf resolved(c.c_str());
        if (fl::PeModule* pe = fl::PeModule::Open(resolved)) {
            FlModule* m = new FlModule();
            m->pe = pe;
            loaded[stem] = m;
            return m;
        }
    }
    errno = ENOENT;
    fprintf(stderr, "[fl] LoadLibrary('%s') failed: not a native module (bin/%s.dylib) nor a PE file in the search path\n", lpLibFileName, stem.c_str());
    return nullptr;
}
inline HMODULE LoadLibraryW(const wchar_t* name) {
    std::string n; for (const wchar_t* p = name; p && *p; ++p) n += (char)*p;
    return LoadLibraryA(n.c_str());
}
inline void* GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    FlModule* m = (FlModule*)hModule;
    return (m && m->dl) ? dlsym(m->dl, lpProcName) : nullptr;   // PE modules export nothing runnable
}
inline BOOL FreeLibrary(HMODULE hLibModule) {
    FlModule* m = (FlModule*)hLibModule;
    if (!m) return FALSE;
    if (m->dl) return TRUE;   // native modules stay resident (other modules may hold pointers into them)
    return TRUE;
    return TRUE;
}
typedef FlResource* HRSRC_FL;
inline HRSRC FindResourceExA(HMODULE hModule, LPCSTR type, LPCSTR name, WORD lang) {
    FlModule* m = (FlModule*)hModule;
    if (!m || !m->pe) return nullptr;
    bool tId = (uintptr_t)type < 0x10000, nId = (uintptr_t)name < 0x10000;
    uint32_t size = 0;
    const uint8_t* p = m->pe->Find(tId ? (uint32_t)fl::PtrToHandle(type) : 0, tId ? nullptr : type,
                                   nId ? (uint32_t)fl::PtrToHandle(name) : 0, nId ? nullptr : name, lang, &size);
    if (!p) return nullptr;
    m->resources.push_back({p, size});
    return &m->resources.back();
}
inline HRSRC FindResourceA(HMODULE h, LPCSTR name, LPCSTR type) { return FindResourceExA(h, type, name, 0); }
inline HGLOBAL LoadResource(HMODULE, HRSRC r) { return r ? (HGLOBAL)((FlResource*)r)->data : nullptr; }
inline void* LockResource(HGLOBAL h) { return h; }
inline DWORD SizeofResource(HMODULE, HRSRC r) { return r ? ((FlResource*)r)->size : 0; }
inline int LoadStringW(HINSTANCE h, UINT id, wchar_t* buf, int cap) {
    FlModule* m = (FlModule*)h;
    if (!m || !m->pe || !buf || cap <= 0) return 0;
    std::vector<uint16_t> tmp((size_t)cap);
    size_t n = m->pe->LoadString16(id, tmp.data(), (size_t)cap);
    for (size_t i = 0; i < n; ++i) buf[i] = (wchar_t)tmp[i];
    buf[n] = 0;
    return (int)n;
}
inline int LoadStringA(HINSTANCE h, UINT id, char* buf, int cap) {
    FlModule* m = (FlModule*)h;
    if (!m || !m->pe || !buf || cap <= 0) return 0;
    std::vector<uint16_t> tmp((size_t)cap);
    size_t n = m->pe->LoadString16(id, tmp.data(), (size_t)cap);
    for (size_t i = 0; i < n; ++i) buf[i] = tmp[i] < 256 ? (char)tmp[i] : '?';   // ACP approximated as Latin-1
    buf[n] = 0;
    return (int)n;
}


// ---- GDI / shell / OLE stubs: COMPILE-ONLY ---------------------------------------------
// common_textrender.cpp rasterises text with GDI into DIB sections. None of that exists
// here; these stubs let the module build and make every call fail/no-op. A real text
// backend (CoreText) must replace them before any in-game text can render.
#define TRANSPARENT 1
#define OPAQUE 2
#define ETO_CLIPPED 0x0004
#define IMAGE_BITMAP 0
#define LR_LOADFROMFILE 0x0010
#define LR_CREATEDIBSECTION 0x2000
#define MB_ICONHAND 0x00000010L
#define LANG_NEUTRAL 0x00
#define SUBLANG_DEFAULT 0x01
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_IGNORE_INSERTS 0x00000200
#define FORMAT_MESSAGE_FROM_SYSTEM 0x00001000
typedef DWORD COLORREF;
typedef void* HDC;
typedef struct _OSVERSIONINFOA { DWORD dwOSVersionInfoSize, dwMajorVersion, dwMinorVersion, dwBuildNumber, dwPlatformId; char szCSDVersion[128]; } OSVERSIONINFOA, *LPOSVERSIONINFOA;
typedef void* HRGN;
#define SRCCOPY 0x00CC0020
#define MAKELANGID(p, s) ((((WORD)(s)) << 10) | (WORD)(p))
typedef struct tagBITMAP { LONG bmType, bmWidth, bmHeight, bmWidthBytes; WORD bmPlanes, bmBitsPixel; void* bmBits; } BITMAP;
typedef struct tagBITMAPINFOHEADER { DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount; DWORD biCompression, biSizeImage; LONG biXPelsPerMeter, biYPelsPerMeter; DWORD biClrUsed, biClrImportant; } BITMAPINFOHEADER;
typedef struct tagDIBSECTION { BITMAP dsBm; BITMAPINFOHEADER dsBmih; DWORD dsBitfields[3]; void* dshSection; DWORD dsOffset; } DIBSECTION;
inline BOOL OffsetRect(RECT* r, int dx, int dy) { r->left += dx; r->right += dx; r->top += dy; r->bottom += dy; return TRUE; }
inline HRGN CreateRectRgn(int, int, int, int) { return nullptr; }
inline HRGN CreateRectRgnIndirect(const RECT*) { return nullptr; }
inline int SelectClipRgn(HDC, HRGN) { return 0; }
inline int ReleaseDC(void*, HDC) { return 1; }
inline int GetObjectA(void*, int, void* p) { fl::ReportUnimplemented("GDI GetObjectA (text renderer backend)"); return 0; }
inline BOOL StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD) { fl::ReportUnimplemented("GDI StretchBlt (text renderer backend)"); return FALSE; }
inline int GetClipRgn(HDC, HRGN) { return 0; }
inline int GetRgnBox(HRGN, RECT* r) { memset(r, 0, sizeof(*r)); return 0; }
inline void* LoadImageA(void*, const char*, UINT, int, int, UINT) { return nullptr; }
inline BOOL GetVersionExA(OSVERSIONINFOA* v) { unsigned a, b; fl::GetPlatform().system->WindowsLikeVersion(&a, &b); v->dwMajorVersion = a; v->dwMinorVersion = b; return TRUE; }
inline BOOL FileTimeToSystemTime(const FILETIME* ft, SYSTEMTIME* st) {
    unsigned long long v = ((unsigned long long)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
    time_t t = (time_t)(v / 10000000ULL - 11644473600ULL); struct tm tmv; gmtime_r(&t, &tmv);
    st->wYear = (WORD)(tmv.tm_year + 1900); st->wMonth = (WORD)(tmv.tm_mon + 1); st->wDayOfWeek = (WORD)tmv.tm_wday;
    st->wDay = (WORD)tmv.tm_mday; st->wHour = (WORD)tmv.tm_hour; st->wMinute = (WORD)tmv.tm_min;
    st->wSecond = (WORD)tmv.tm_sec; st->wMilliseconds = 0; return TRUE;
}
inline int CompareFileTime(const FILETIME* a, const FILETIME* b) {
    unsigned long long x = ((unsigned long long)a->dwHighDateTime << 32) | a->dwLowDateTime;
    unsigned long long y = ((unsigned long long)b->dwHighDateTime << 32) | b->dwLowDateTime;
    return x < y ? -1 : (x > y ? 1 : 0);
}
inline BOOL SystemTimeToFileTime(const SYSTEMTIME* st, FILETIME* ft) {
    struct tm t = {}; t.tm_year = st->wYear - 1900; t.tm_mon = st->wMonth - 1; t.tm_mday = st->wDay;
    t.tm_hour = st->wHour; t.tm_min = st->wMinute; t.tm_sec = st->wSecond;
    unsigned long long v = ((unsigned long long)timegm(&t) + 11644473600ULL) * 10000000ULL;
    ft->dwLowDateTime = (DWORD)v; ft->dwHighDateTime = (DWORD)(v >> 32); return TRUE;
}
inline COLORREF SetTextColor(HDC, COLORREF) { return 0; }
inline COLORREF SetBkColor(HDC, COLORREF) { return 0; }
inline int SetBkMode(HDC, int) { return 0; }
inline UINT SetTextAlign(HDC, UINT) { return 0; }
inline HDC GetDC(void*) { return nullptr; }
inline HDC CreateCompatibleDC(HDC) { return nullptr; }
inline BOOL DeleteDC(HDC) { return TRUE; }
inline HFONT CreateFontIndirectA(const LOGFONTA*) { return nullptr; }
inline BOOL GetTextMetricsA_unused() { return FALSE; }
inline BOOL GetTextMetricsA(HDC, TEXTMETRICA* tm) { memset(tm, 0, sizeof(*tm)); return FALSE; }
inline BOOL GetTextExtentPoint32W(HDC, const wchar_t*, int, SIZE* sz) { sz->cx = sz->cy = 0; return FALSE; }
inline BOOL GetTextExtentExPointW(HDC, const wchar_t*, int, int, int*, int*, SIZE* sz) { sz->cx = sz->cy = 0; return FALSE; }
inline BOOL GetCharWidthW(HDC, UINT, UINT, int* w) { *w = 0; return FALSE; }
inline BOOL ExtTextOutW(HDC, int, int, UINT, const void*, const wchar_t*, UINT, const int*) { return FALSE; }
inline void LocalFree(void* p) { free(p); }
inline int _snprintf_s(char* buf, size_t size, size_t count, const char* fmt, ...) {
    va_list a; va_start(a, fmt);
    size_t n = count == _TRUNCATE || count >= size ? size : count + 1;
    int r = vsnprintf(buf, n, fmt, a); va_end(a); return r;
}


// ---- window-message pump / locale formatting / FormatMessage ---------------------------
// The game-logic layer has no window of its own on this platform: the host (SDL front end)
// owns the event loop, so there is never a Win32 message to pump here.
#define WM_QUIT 0x0012
#define PM_REMOVE 0x0001
typedef struct tagMSG { void* hwnd; UINT message; uintptr_t wParam; intptr_t lParam; DWORD time; } MSG;
inline BOOL PeekMessageA(MSG*, void*, UINT, UINT, UINT) { return FALSE; }
inline intptr_t DispatchMessageA(const MSG*) { return 0; }
inline DWORD MsgWaitForMultipleObjects(DWORD n, const HANDLE* h, BOOL all, DWORD ms, DWORD) {
    return WaitForMultipleObjects(n, h, all, ms);
}
inline int GetTimeFormatA(DWORD, DWORD, const SYSTEMTIME* t, const char*, char* out, int cap) {
    return snprintf(out, (size_t)cap, "%02u:%02u", t->wHour, t->wMinute) + 1;
}
inline int GetDateFormatA(DWORD, DWORD, const SYSTEMTIME* t, const char*, char* out, int cap) {
    return snprintf(out, (size_t)cap, "%u/%u/%04u", t->wMonth, t->wDay, t->wYear) + 1;
}
inline DWORD FormatMessageA(DWORD, const void*, DWORD, DWORD, char* buf, DWORD, void*) {
    if (buf) buf[0] = 0; return 0;   // no system message table on this platform
}

#if !defined(FL_WIN_COMPAT_REGISTRY)
#define FL_WIN_COMPAT_REGISTRY
// ---- registry -> fl::IRegistry; user name -> fl::ISystemInfo ---------------------------
struct FlRegKey { fl::RegistryRoot root; char path[260]; };
#define HKEY_LOCAL_MACHINE ((HKEY)(intptr_t)1)
#define HKEY_CURRENT_USER ((HKEY)(intptr_t)2)
#define ERROR_SUCCESS 0L
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE 0x0002
#define KEY_READ 0x20019
#define REG_SZ 1
#define REG_DWORD 4
inline BOOL GetUserNameA(char* buf, DWORD* size) {
    if (!fl::GetPlatform().system->UserName(buf, *size)) return FALSE;
    *size = (DWORD)strlen(buf) + 1; return TRUE;
}
inline LONG RegOpenKeyExA(HKEY root, const char* sub, DWORD, DWORD, HKEY* out) {
    FlRegKey* k = new FlRegKey;
    std::string path = sub ? sub : "";
    if ((uintptr_t)root > 0xff) {            // parent is an opened key: compose the path
        const FlRegKey* parent = (const FlRegKey*)root;
        k->root = parent->root;
        path = path.empty() ? std::string(parent->path) : std::string(parent->path) + "\\" + path;
    } else {
        k->root = root == HKEY_CURRENT_USER ? fl::RegistryRoot::CurrentUser : fl::RegistryRoot::LocalMachine;
    }
    strncpy_s(k->path, sizeof(k->path), path.c_str(), _TRUNCATE);
    if (!fl::GetPlatform().registry->KeyExists(k->root, k->path)) { delete k; *out = nullptr; return 2; /* ERROR_FILE_NOT_FOUND */ }
    *out = (HKEY)k; return ERROR_SUCCESS;
}
inline LONG RegOpenKeyA(HKEY root, const char* sub, HKEY* out) { return RegOpenKeyExA(root, sub, 0, 0, out); }
inline LONG RegCreateKeyExA(HKEY root, const char* sub, DWORD, char*, DWORD, DWORD, void*, HKEY* out, DWORD* disp) {
    fl::RegistryRoot r = root == HKEY_CURRENT_USER ? fl::RegistryRoot::CurrentUser : fl::RegistryRoot::LocalMachine;
    bool existed = fl::GetPlatform().registry->KeyExists(r, sub);
    if (!existed) fl::GetPlatform().registry->WriteString(r, sub, "", "");   // marker so the key now exists
    if (disp) *disp = existed ? 2 : 1;   // REG_OPENED_EXISTING_KEY / REG_CREATED_NEW_KEY
    return RegOpenKeyExA(root, sub, 0, 0, out);
}
inline LONG RegQueryValueExA(HKEY key, const char* name, DWORD*, DWORD* type, BYTE* data, DWORD* cb) {
    FlRegKey* k = (FlRegKey*)key;
    if (!k || !data || !cb) return 2;
    if (!fl::GetPlatform().registry->ReadString(k->root, k->path, name, (char*)data, *cb)) return 2;
    if (type) *type = REG_SZ;
    *cb = (DWORD)strlen((char*)data) + 1; return ERROR_SUCCESS;
}
inline LONG RegSetValueExA(HKEY key, const char* name, DWORD, DWORD type, const BYTE* data, DWORD cb) {
    FlRegKey* k = (FlRegKey*)key;
    if (!k || type != REG_SZ) return 87;
    std::string v((const char*)data, cb); while (!v.empty() && v.back() == 0) v.pop_back();
    return fl::GetPlatform().registry->WriteString(k->root, k->path, name, v.c_str()) ? ERROR_SUCCESS : 5;
}
inline LONG RegCloseKey(HKEY key) {
    if ((uintptr_t)key > 0xff) delete (FlRegKey*)key;   // predefined roots (HKEY_CURRENT_USER, ...) are small constants
    return ERROR_SUCCESS;
}
#endif

#include "win_ui_compat.h"

#endif // !_WIN32
