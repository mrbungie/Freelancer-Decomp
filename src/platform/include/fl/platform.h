// Freelancer platform abstraction layer.
//
// The decompiled game code is written against Win32. Where Win32 offers an OS service
// that has no portable C++ equivalent, the Win32-named wrappers in
// include/platform/win_compat.h forward to the interfaces below. A backend
// (src/platform/backends/<name>/) implements them; CMake picks one with
// -DFL_PLATFORM_BACKEND=<name> (default: posix). Adding a platform means adding a
// backend directory, not touching game code.
#pragma once
#include <stddef.h>
#include "fl/input.h"

namespace fl
{
    // ---- files ----------------------------------------------------------------------
    struct DirEntry
    {
        char name[260];
        bool isDirectory;
        unsigned long long size;
    };

    class IFileSystem
    {
    public:
        virtual ~IFileSystem() {}
        // Wildcard search ('*', '?') in a single directory; either slash style accepted.
        // Returns an opaque handle or nullptr when nothing matches.
        virtual void* FindFirst(const char* pattern, DirEntry* first) = 0;
        virtual bool FindNext(void* handle, DirEntry* next) = 0;
        virtual void FindClose(void* handle) = 0;
        virtual bool CopyFile(const char* from, const char* to, bool failIfExists) = 0;
        // Converts a Win32-style path (backslashes, any letter case) to one that exists on this
        // host: slashes are normalised and, when the exact path is missing, each component is
        // matched case-insensitively. Unresolvable paths come back slash-normalised only.
        virtual void ResolvePath(const char* in, char* out, size_t capacity) = 0;
        virtual bool MakeDirectory(const char* path) = 0;
        virtual bool RemoveDirectory(const char* path) = 0;
    };

    // ---- well-known folders (SHGetFolderPath replacement) ---------------------------
    enum class Folder { LocalAppData, RoamingAppData, Documents, Pictures };

    class ISystemFolders
    {
    public:
        virtual ~ISystemFolders() {}
        virtual bool Get(Folder which, bool create, char* out, size_t capacity) = 0;
    };

    // ---- time -----------------------------------------------------------------------
    struct DateTime
    {
        unsigned short year, month, dayOfWeek, day, hour, minute, second, millisecond;
    };

    class IClock
    {
    public:
        virtual ~IClock() {}
        virtual void LocalTime(DateTime* out) = 0;
        virtual void UtcTime(DateTime* out) = 0;
        virtual unsigned long long MonotonicMilliseconds() = 0;
    };

    // ---- identity / system info -----------------------------------------------------
    class ISystemInfo
    {
    public:
        virtual ~ISystemInfo() {}
        virtual void NewUuid(unsigned char out[16]) = 0;
        virtual bool UserName(char* out, size_t capacity) = 0;
        // Total physical memory and free virtual address space, in bytes.
        virtual void MemoryStatus(unsigned long long* totalPhysical, unsigned long long* availableVirtual) = 0;
        // Win32-style version numbers reported to the game (retail only tests "> NT4").
        virtual void WindowsLikeVersion(unsigned* major, unsigned* minor) = 0;
    };

    // ---- registry replacement: small persistent key/value store ---------------------
    enum class RegistryRoot { LocalMachine, CurrentUser };

    class IRegistry
    {
    public:
        virtual ~IRegistry() {}
        // Returns false when the key/value does not exist.
        virtual bool ReadString(RegistryRoot root, const char* key, const char* value,
                                char* out, size_t capacity) = 0;
        // True when the key exists (it has values, or was created, or has existing subkeys).
        virtual bool KeyExists(RegistryRoot root, const char* key) = 0;
        virtual bool WriteString(RegistryRoot root, const char* key, const char* value,
                                 const char* data) = 0;
    };

    // ---- window / cursor host ---------------------------------------------------------
    // The client's Win32 window calls (ShowCursor, SetCursorPos, DestroyWindow, ...)
    // forward here. The front end (SDL2 in the freelancer executable) installs the real
    // implementation with SetWindowHost(); until then calls are reported and ignored.
    // Callbacks the front end invokes from IWindowHost::PumpEvents (replaces the Win32 WndProc).
    class IWindowEvents
    {
    public:
        virtual ~IWindowEvents() {}
        virtual void OnFocus(bool gained) = 0;
        virtual void OnActivateApp(bool active) = 0;
        virtual void OnCloseRequested() = 0;
        virtual void OnMouseMove() = 0;
    };

    class IWindowHost
    {
    public:
        virtual ~IWindowHost() {}
        virtual int ShowCursor(bool show) = 0;            // returns display counter, like Win32
        virtual bool SetCursorPosition(int x, int y) = 0; // screen coordinates
        virtual bool GetCursorPosition(int* x, int* y) = 0;
        virtual bool SetWindowRect(void* window, int x, int y, int w, int h, bool topmost) = 0;
        virtual void DestroyWindow(void* window) = 0;
        virtual void SetCursorShape(void* cursor) = 0;
        virtual int GetKeyState(int virtualKey) = 0;
        // ---- used directly by game code (no Win32 equivalent layered on top) ----
        virtual void* CreateGameWindow(const char* title, int width, int height, bool fullscreen) = 0;
        virtual void SetWindowMode(void* window, bool fullscreen, int width, int height) = 0;
        virtual void DesktopSize(int* width, int* height) = 0;
        // Handles pending OS events, calling `sink`. Returns false once quit was requested
        // (*exitCode receives the code given to RequestQuit, or 0).
        virtual bool PumpEvents(IWindowEvents* sink, int* exitCode) = 0;
        virtual void RequestQuit(int exitCode) = 0;
        virtual IMouse* CreateMouse() = 0;
        virtual bool IsWindowForeground(void* window) = 0;
        virtual unsigned DoubleClickTimeMs() = 0;
        virtual bool IsCursorVisible() = 0;
        virtual void SetCursorVisible(bool visible) = 0;
        // Pointer position in window client coordinates.
        virtual bool GetCursorInWindow(void* window, int* x, int* y) = 0;
        virtual bool SetCursorInWindow(void* window, int x, int y) = 0;      // Win32 GetKeyState bits (0x8000 down, 1 toggled)
    };

    void SetWindowHost(IWindowHost* host);
    IWindowHost* GetWindowHost(); // never null (falls back to a reporting null host)

    // ---- aggregate ------------------------------------------------------------------
    struct Platform
    {
        IFileSystem* files;
        ISystemFolders* folders;
        IClock* clock;
        ISystemInfo* system;
        IRegistry* registry;
    };

    // Implemented by exactly one backend.
    Platform& GetPlatform();

    // Backends and shims call this for anything not implemented, instead of silently
    // succeeding. Logs once per distinct `what` to stderr.
    void ReportUnimplemented(const char* what);
}
