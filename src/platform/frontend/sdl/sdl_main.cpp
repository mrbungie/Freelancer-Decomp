// SDL2 front end for the portable Freelancer client.
//
// Responsibilities (current state / TODO):
//  * SDL access is isolated in fl_sdl.{h,cpp} (FL_SDL_VERSION = 2 | 1).
//  * Render backend registration: fl/render_backend.h (FL_RENDER_BACKEND = null | sokol).
//  * main(): builds a WinMain-style command line and calls the decompiled WinMain.   [done]
//  * fl::IWindowHost: owns the SDL window, cursor and key state.                      [cursor/keys basic]
//  * DirectInput replacement: SDL events -> IDirectInput8-shaped devices.             [NOT implemented: the
//    stubs below fail device creation and report it]
//  * Rendering: an IRenderPipeline (RPModern) backend; the D3D8/9 pipelines are not built. [NOT implemented]
#include <sys/resource.h>
#include "fl/platform.h"
#include "win_compat.h"

#include "fl_sdl.h"
#include "fl/render_backend.h"
#include "dacom.h"

#include <string>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

extern const char* const FlImplicitModules[]; // implicit_modules_generated.cpp
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int); // src/freelancer/Client/main.cpp

fl::IMouse* FlSdlCreateMouse();   // sdl_mouse.cpp

namespace
{
    class SdlSurface : public fl::render::IWindowSurface
    {
    public:
        bool CreateContext(void* w) override
        {
            return fl::render::RequiredWindowKind() == fl::render::WindowKind::Metal ? fl_sdl_metal_layer(w) != nullptr
                                                                                      : fl_sdl_gl_create_context(w);
        }
        void* NativeLayer(void* w) override { return fl_sdl_metal_layer(w); }
        void Present(void* w) override { if (fl::render::RequiredWindowKind() == fl::render::WindowKind::OpenGL) fl_sdl_gl_swap(w); }
        void DrawableSize(void* w, int* pw, int* ph) override
        {
            if (fl::render::RequiredWindowKind() == fl::render::WindowKind::Metal) fl_sdl_metal_drawable_size(w, pw, ph);
            else fl_sdl_gl_drawable_size(w, pw, ph);
        }
    };
}

namespace
{
    // All SDL access goes through fl_sdl.h so the SDL1/SDL2 choice stays in one file.
    class SdlWindowHost : public fl::IWindowHost
    {
        int cursorCounter_ = 0;
        bool quit_ = false;
        int exitCode_ = 0;
    public:
        int ShowCursor(bool show) override
        {
            cursorCounter_ += show ? 1 : -1;
            fl_sdl_show_cursor(cursorCounter_ >= 0);
            return cursorCounter_;
        }
        bool SetCursorPosition(int x, int y) override { return fl_sdl_warp_cursor(nullptr, x, y); }
        bool GetCursorPosition(int* x, int* y) override { return fl_sdl_get_cursor(nullptr, x, y); }
        bool SetWindowRect(void* w, int x, int y, int cw, int ch, bool topmost) override
        {
            fl_sdl_set_window_rect(w, x, y, cw, ch, topmost);
            return w != nullptr;
        }
        void DestroyWindow(void* w) override { fl_sdl_destroy_window(w); quit_ = true; }
        void SetCursorShape(void*) override {}
        void* CreateGameWindow(const char* title, int w, int h, bool fullscreen) override
        {
            FlSdlWindow win = fl_sdl_create_window(title, w, h, fullscreen,
                                                  static_cast<int>(fl::render::RequiredWindowKind()));
            if (!win) fprintf(stderr, "[fl] window creation failed: %s\n", fl_sdl_error());
            if (getenv("FL_TRACE_WINDOW")) fprintf(stderr, "[fl] CreateGameWindow '%s' %dx%d fullscreen=%d -> %p\n", title, w, h, (int)fullscreen, win);
            fl::render::SetNativeWindow(win);
            return win;
        }
        void SetWindowMode(void* win, bool fullscreen, int w, int h) override
        {
            if (getenv("FL_TRACE_WINDOW")) fprintf(stderr, "[fl] SetWindowMode %dx%d fullscreen=%d\n", w, h, (int)fullscreen);
            // Development default: stay windowed. Export FL_FULLSCREEN=1 to honour the game's fullscreen requests.
            if (!getenv("FL_FULLSCREEN")) fullscreen = false;
            fl_sdl_set_window_mode(win, fullscreen, w, h);
        }
        void DesktopSize(int* w, int* h) override { fl_sdl_desktop_size(w, h); }
        void RequestQuit(int code) override { quit_ = true; exitCode_ = code; }
        bool PumpEvents(fl::IWindowEvents* sink, int* exitCode) override
        {
            struct Fwd : FlSdlEvents
            {
                SdlWindowHost& h; fl::IWindowEvents* s;
                Fwd(SdlWindowHost& host, fl::IWindowEvents* sink) : h(host), s(sink) {}
                void Close() override { s->OnCloseRequested(); }
                void Focus(bool g) override { s->OnFocus(g); s->OnActivateApp(g); }
                void MouseMove() override { s->OnMouseMove(); }
                void Quit() override { s->OnCloseRequested(); }
            } fwd(*this, sink);
            fl_sdl_poll_events(&fwd);
            if (quit_) { *exitCode = exitCode_; return false; }
            return true;
        }
        fl::IMouse* CreateMouse() override { return FlSdlCreateMouse(); }
        bool IsWindowForeground(void* w) override { return fl_sdl_window_has_focus(w); }
        unsigned DoubleClickTimeMs() override { return 500; }
        bool IsCursorVisible() override { return cursorCounter_ >= 0; }
        void SetCursorVisible(bool v) override { cursorCounter_ = v ? 0 : -1; fl_sdl_show_cursor(v); }
        bool GetCursorInWindow(void*, int* x, int* y) override { return fl_sdl_get_cursor(nullptr, x, y); }   // TODO: subtract window origin
        bool SetCursorInWindow(void*, int x, int y) override { return fl_sdl_warp_cursor(nullptr, x, y); }
        int GetKeyState(int vk) override
        {
            switch (vk)
            {
            case VK_SHIFT:   return (fl_sdl_key_down(FL_KEY_LSHIFT) || fl_sdl_key_down(FL_KEY_RSHIFT)) ? 0x8000 : 0;
            case VK_CONTROL: return (fl_sdl_key_down(FL_KEY_LCTRL) || fl_sdl_key_down(FL_KEY_RCTRL)) ? 0x8000 : 0;
            case VK_MENU:    return (fl_sdl_key_down(FL_KEY_LALT) || fl_sdl_key_down(FL_KEY_RALT)) ? 0x8000 : 0;
            case VK_ESCAPE:  return fl_sdl_key_down(FL_KEY_ESCAPE) ? 0x8000 : 0;
            case VK_RETURN:  return fl_sdl_key_down(FL_KEY_RETURN) ? 0x8000 : 0;
            case VK_SPACE:   return fl_sdl_key_down(FL_KEY_SPACE) ? 0x8000 : 0;
            case VK_TAB:     return fl_sdl_key_down(FL_KEY_TAB) ? 0x8000 : 0;
            }
            if (vk >= 'A' && vk <= 'Z') return fl_sdl_key_down(FL_KEY_A + (vk - 'A')) ? 0x8000 : 0;
            if (vk >= '0' && vk <= '9') return fl_sdl_key_down(FL_KEY_0 + (vk - '0')) ? 0x8000 : 0;
            return 0;
        }
    };
}

static void CrashHandler(int sig)
{
    void* frames[40];
    int n = backtrace(frames, 40);
    dprintf(2, "[fl] fatal signal %d, backtrace:\n", sig);
    backtrace_symbols_fd(frames, n, 2);
    _exit(128 + sig);
}

int main(int argc, char** argv)
{
    // FL_NO_CRASH_HANDLER=1 leaves SIGSEGV to the debugger / AddressSanitizer.
    if (!getenv("FL_NO_CRASH_HANDLER")) {
    // Alternate stack so the handler also runs on stack overflow.
    static char altStack[1 << 16];
    stack_t ss = { altStack, sizeof(altStack), 0 };
    sigaltstack(&ss, nullptr);
    struct sigaction sa = {};
    sa.sa_handler = CrashHandler;
    sa.sa_flags = SA_ONSTACK;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    }
    dprintf(2, "[fl] main entered\n");
    if (!fl_sdl_init())
    {
        fprintf(stderr, "SDL init failed: %s\n", fl_sdl_error());
        return 1;
    }
    // Modules linked directly into this executable are loaded by dyld, so run their DllMain
    // (DLL_PROCESS_ATTACH) here, as the Windows loader would have.
    for (const char* const* m = FlImplicitModules; *m; ++m)
        LoadLibraryA(*m);

    {   // The game keeps many data files open at once; macOS shells default to 256 descriptors.
        struct rlimit lim;
        if (getrlimit(RLIMIT_NOFILE, &lim) == 0)
        {
            rlim_t want = 65536;
            if (lim.rlim_max != RLIM_INFINITY && lim.rlim_max < want) want = lim.rlim_max;
            if (lim.rlim_cur < want) { lim.rlim_cur = want; setrlimit(RLIMIT_NOFILE, &lim); }
        }
    }

    static SdlWindowHost host;
    fl::SetWindowHost(&host);

    // Render backend hook (see fl/render_backend.h): registers the "DirectX8" pipeline component.
    static SdlSurface surface;
    fl::render::SetWindowSurface(&surface);
    fl::render::RegisterBackend(DACOM_Acquire());

    std::string cmd;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1) cmd += ' ';
        cmd += argv[i];
    }
    int rc = WinMain(nullptr, nullptr, cmd.data(), 1);
    fl_sdl_quit();
    return rc;
}
