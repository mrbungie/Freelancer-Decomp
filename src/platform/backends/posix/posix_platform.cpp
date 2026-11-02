// POSIX backend (macOS, Linux, BSD). macOS-only behaviour is isolated under __APPLE__.
#include "fl/platform.h"

#include <dirent.h>
#if defined(__APPLE__)
#  include <sys/sysctl.h>
#endif
#include <fnmatch.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <strings.h>
#include <mutex>
#include <set>
#include <string>

#if defined(__APPLE__)
#  include <uuid/uuid.h>
#endif

namespace fl
{
    void ReportUnimplemented(const char* what)
    {
        static std::mutex M;
        static std::set<std::string> Seen;
        std::lock_guard<std::mutex> Lock(M);
        if (Seen.insert(what).second)
            std::fprintf(stderr, "[fl::platform] not implemented: %s\n", what);
    }

    namespace
    {
        void Copy(char* out, size_t cap, const std::string& s)
        {
            if (cap == 0) return;
            size_t n = s.size() < cap - 1 ? s.size() : cap - 1;
            std::memcpy(out, s.data(), n);
            out[n] = 0;
        }

        // ---------------------------------------------------------------- files
        struct FindState { DIR* dir; std::string pattern; };

        class PosixFileSystem : public IFileSystem
        {
            static bool Next(FindState* st, DirEntry* out)
            {
                while (dirent* e = readdir(st->dir))
                {
                    if (fnmatch(st->pattern.c_str(), e->d_name, 0) != 0) continue;
                    Copy(out->name, sizeof(out->name), e->d_name);
                    out->isDirectory = e->d_type == DT_DIR;
                    out->size = 0;
                    return true;
                }
                return false;
            }
        public:
            void* FindFirst(const char* pattern, DirEntry* first) override
            {
                std::string s(pattern);
                for (auto& c : s) if (c == '\\') c = '/';
                size_t slash = s.find_last_of('/');
                std::string dir = slash == std::string::npos ? "." : s.substr(0, slash);
                std::string pat = slash == std::string::npos ? s : s.substr(slash + 1);
                DIR* d = opendir(dir.empty() ? "/" : dir.c_str());
                if (!d) return nullptr;
                FindState* st = new FindState{d, pat};
                if (!Next(st, first)) { closedir(d); delete st; return nullptr; }
                return st;
            }
            bool FindNext(void* h, DirEntry* out) override { return Next(static_cast<FindState*>(h), out); }
            void FindClose(void* h) override
            {
                FindState* st = static_cast<FindState*>(h);
                if (st) { closedir(st->dir); delete st; }
            }
            bool CopyFile(const char* from, const char* to, bool failIfExists) override
            {
                if (failIfExists && access(to, F_OK) == 0) return false;
                std::ifstream in(from, std::ios::binary);
                if (!in) return false;
                std::ofstream out(to, std::ios::binary | std::ios::trunc);
                if (!out) return false;
                out << in.rdbuf();
                return out.good();
            }
            void ResolvePath(const char* in, char* out, size_t cap) override
            {
                std::string path(in ? in : "");
                for (auto& c : path) if (c == '\\') c = '/';
                struct stat st;
                if (path.empty() || stat(path.c_str(), &st) == 0) { Copy(out, cap, path); return; }
                // Case-insensitive component walk.
                std::string cur = path[0] == '/' ? "/" : "";
                size_t pos = path[0] == '/' ? 1 : 0;
                while (pos <= path.size())
                {
                    size_t slash = path.find('/', pos);
                    std::string comp = path.substr(pos, slash == std::string::npos ? std::string::npos : slash - pos);
                    if (!comp.empty() && comp != "." && comp != "..")
                    {
                        std::string trial = cur + comp;
                        if (stat(trial.c_str(), &st) != 0)
                        {
                            DIR* d = opendir(cur.empty() ? "." : cur.c_str());
                            bool found = false;
                            if (d)
                            {
                                while (dirent* e = readdir(d))
                                    if (strcasecmp(e->d_name, comp.c_str()) == 0) { comp = e->d_name; found = true; break; }
                                closedir(d);
                            }
                            if (!found) { Copy(out, cap, path); return; }   // give up: caller sees ENOENT
                        }
                    }
                    cur += comp;
                    if (slash == std::string::npos) break;
                    cur += '/';
                    pos = slash + 1;
                }
                Copy(out, cap, cur);
            }
            bool MakeDirectory(const char* p) override { return mkdir(p, 0755) == 0; }
            bool RemoveDirectory(const char* p) override { return rmdir(p) == 0; }
        };

        // -------------------------------------------------------------- folders
        std::string HomeDir()
        {
            if (const char* h = getenv("HOME")) return h;
            if (passwd* pw = getpwuid(getuid())) return pw->pw_dir;
            return ".";
        }

        class PosixFolders : public ISystemFolders
        {
        public:
            bool Get(Folder which, bool create, char* out, size_t cap) override
            {
                std::string home = HomeDir(), p;
#if defined(__APPLE__)
                switch (which)
                {
                case Folder::LocalAppData:
                case Folder::RoamingAppData: p = home + "/Library/Application Support"; break;
                case Folder::Documents: p = home + "/Documents"; break;
                case Folder::Pictures: p = home + "/Pictures"; break;
                }
#else // XDG
                const char* xdgData = getenv("XDG_DATA_HOME");
                const char* xdgCfg = getenv("XDG_CONFIG_HOME");
                switch (which)
                {
                case Folder::LocalAppData: p = xdgData ? xdgData : home + "/.local/share"; break;
                case Folder::RoamingAppData: p = xdgCfg ? xdgCfg : home + "/.config"; break;
                case Folder::Documents: p = home + "/Documents"; break;
                case Folder::Pictures: p = home + "/Pictures"; break;
                }
#endif
                if (create) mkdir(p.c_str(), 0755);
                Copy(out, cap, p);
                return true;
            }
        };

        // ---------------------------------------------------------------- clock
        void FromTm(const tm& t, DateTime* o)
        {
            o->year = (unsigned short)(t.tm_year + 1900); o->month = (unsigned short)(t.tm_mon + 1);
            o->dayOfWeek = (unsigned short)t.tm_wday;     o->day = (unsigned short)t.tm_mday;
            o->hour = (unsigned short)t.tm_hour;          o->minute = (unsigned short)t.tm_min;
            o->second = (unsigned short)t.tm_sec;         o->millisecond = 0;
        }

        class PosixClock : public IClock
        {
        public:
            void LocalTime(DateTime* o) override { time_t t = time(nullptr); tm v; localtime_r(&t, &v); FromTm(v, o); }
            void UtcTime(DateTime* o) override { time_t t = time(nullptr); tm v; gmtime_r(&t, &v); FromTm(v, o); }
            unsigned long long MonotonicMilliseconds() override
            {
                using namespace std::chrono;
                return (unsigned long long)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
            }
        };

        // --------------------------------------------------------- system info
        class PosixSystemInfo : public ISystemInfo
        {
        public:
            void NewUuid(unsigned char out[16]) override
            {
#if defined(__APPLE__)
                uuid_generate(out);
#else
                std::ifstream r("/dev/urandom", std::ios::binary);
                r.read(reinterpret_cast<char*>(out), 16);
                out[6] = (unsigned char)((out[6] & 0x0f) | 0x40);
                out[8] = (unsigned char)((out[8] & 0x3f) | 0x80);
#endif
            }
            bool UserName(char* out, size_t cap) override
            {
                if (passwd* pw = getpwuid(getuid())) { Copy(out, cap, pw->pw_name); return true; }
                return false;
            }
            void MemoryStatus(unsigned long long* phys, unsigned long long* virt) override
            {
#if defined(__APPLE__)
                uint64_t mem = 0; size_t len = sizeof(mem);
                if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) != 0) mem = 4ull << 30;
                *phys = mem;
#else
                long pages = sysconf(_SC_PHYS_PAGES), sz = sysconf(_SC_PAGE_SIZE);
                *phys = (unsigned long long)pages * (unsigned long long)sz;
#endif
                *virt = 1ull << 40;     // 64-bit address space: effectively unlimited
            }
            void WindowsLikeVersion(unsigned* major, unsigned* minor) override { *major = 5; *minor = 1; }
        };

        // ------------------------------------------------------------- registry
        // Emulated as a flat "root\key\value=data" text file in the user's data folder.
        class PosixRegistry : public IRegistry
        {
            std::mutex M;
            std::string FilePath()
            {
                char dir[512];
                GetPlatform().folders->Get(Folder::LocalAppData, true, dir, sizeof(dir));
                std::string d = std::string(dir) + "/Freelancer";
                mkdir(d.c_str(), 0755);
                return d + "/registry.txt";
            }
            static std::string Id(RegistryRoot r, const char* k, const char* v)
            {
                return std::string(r == RegistryRoot::LocalMachine ? "HKLM" : "HKCU") + "\\" + k + "\\" + v;
            }
        public:
            bool ReadString(RegistryRoot r, const char* key, const char* value, char* out, size_t cap) override
            {
                std::lock_guard<std::mutex> L(M);
                std::ifstream f(FilePath());
                std::string line, id = Id(r, key, value) + "=";
                while (std::getline(f, line))
                    if (line.compare(0, id.size(), id) == 0) { Copy(out, cap, line.substr(id.size())); return true; }
                return false;
            }
            bool KeyExists(RegistryRoot r, const char* key) override
            {
                std::lock_guard<std::mutex> L(M);
                std::ifstream f(FilePath());
                std::string line, prefix = Id(r, key, "");   // "<root>\\<key>\\"
                while (std::getline(f, line))
                    if (line.compare(0, prefix.size(), prefix) == 0) return true;
                return false;
            }
            bool WriteString(RegistryRoot r, const char* key, const char* value, const char* data) override
            {
                std::lock_guard<std::mutex> L(M);
                std::string path = FilePath(), id = Id(r, key, value) + "=", line, all;
                bool replaced = false;
                { std::ifstream f(path);
                  while (std::getline(f, line)) {
                      if (line.compare(0, id.size(), id) == 0) { all += id + data + "\n"; replaced = true; }
                      else all += line + "\n"; } }
                if (!replaced) all += id + data + "\n";
                std::ofstream o(path, std::ios::trunc);
                o << all;
                return o.good();
            }
        };

        PosixFileSystem G_Files;
        PosixFolders G_Folders;
        PosixClock G_Clock;
        PosixSystemInfo G_System;
        PosixRegistry G_Registry;
        Platform G_Platform = { &G_Files, &G_Folders, &G_Clock, &G_System, &G_Registry };
    }

    namespace
    {
        class NullWindowHost : public IWindowHost
        {
        public:
            int ShowCursor(bool) override { ReportUnimplemented("IWindowHost::ShowCursor"); return 0; }
            bool SetCursorPosition(int, int) override { ReportUnimplemented("IWindowHost::SetCursorPosition"); return false; }
            bool GetCursorPosition(int* x, int* y) override { *x = *y = 0; return false; }
            bool SetWindowRect(void*, int, int, int, int, bool) override { ReportUnimplemented("IWindowHost::SetWindowRect"); return false; }
            void DestroyWindow(void*) override { ReportUnimplemented("IWindowHost::DestroyWindow"); }
            void SetCursorShape(void*) override {}
            int GetKeyState(int) override { return 0; }
            // No front end installed: a mouse that never produces input, so game code needs no null checks.
            class NullMouse : public IMouse
            {
            public:
                bool Acquire() override { return false; }
                void Unacquire() override {}
                bool SetExclusive(void*, bool) override { return false; }
                bool SetEventBuffer(unsigned) override { return true; }
                bool ReadEvents(MouseEvent*, unsigned* c) override { *c = 0; return true; }
                bool ReadState(MouseState* s) override { *s = MouseState(); for (auto& d : s->down) d = 0; return true; }
                void Poll() override {}
                void Destroy() override { delete this; }
            };
            void* CreateGameWindow(const char*, int, int, bool) override { ReportUnimplemented("IWindowHost::CreateGameWindow (no front end)"); return nullptr; }
            void SetWindowMode(void*, bool, int, int) override {}
            void DesktopSize(int* w, int* h) override { *w = 1920; *h = 1080; }
            bool PumpEvents(IWindowEvents*, int* code) override { *code = 0; return false; }
            void RequestQuit(int) override {}
            IMouse* CreateMouse() override { ReportUnimplemented("IWindowHost::CreateMouse (no front end)"); return new NullMouse(); }
            bool IsWindowForeground(void*) override { return true; }
            unsigned DoubleClickTimeMs() override { return 500; }
            bool IsCursorVisible() override { return true; }
            void SetCursorVisible(bool) override {}
            bool GetCursorInWindow(void*, int* x, int* y) override { *x = *y = 0; return false; }
            bool SetCursorInWindow(void*, int, int) override { return false; }
        };
        NullWindowHost G_NullHost;
        IWindowHost* G_Host = &G_NullHost;
    }
    void SetWindowHost(IWindowHost* h) { G_Host = h ? h : &G_NullHost; }
    IWindowHost* GetWindowHost() { return G_Host; }

    Platform& GetPlatform() { return G_Platform; }
}
