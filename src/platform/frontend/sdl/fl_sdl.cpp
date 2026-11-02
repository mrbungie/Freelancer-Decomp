// The only file that talks to SDL. One branch per FL_SDL_VERSION; see fl_sdl.h.
#include "fl_sdl.h"
#include <stdio.h>

#if FL_SDL_VERSION == 2
// ============================================================================ SDL 2
bool fl_sdl_init()
{
    return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) == 0;
}
void fl_sdl_quit() { SDL_Quit(); }
const char* fl_sdl_error() { return SDL_GetError(); }

FlSdlWindow fl_sdl_create_window(const char* title, int w, int h, bool fullscreen, int surface)
{
    const bool glContext = surface == 1;
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | (fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
                   (glContext ? SDL_WINDOW_OPENGL : 0) | (surface == 2 ? SDL_WINDOW_METAL : 0);
    if (glContext)
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    }
    SDL_Window* win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
    // Text input stays off until a text field asks for it: SDL's Cocoa text-input view (an NSRemoteView) crashes
    // on focus loss in a plain (non-bundled) process.
    if (win) SDL_StopTextInput();
    return win;
}
void fl_sdl_set_window_mode(FlSdlWindow w, bool fullscreen, int cw, int ch)
{
    SDL_Window* win = static_cast<SDL_Window*>(w);
    if (!win) return;
    SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    if (!fullscreen) SDL_SetWindowSize(win, cw, ch);
    SDL_ShowWindow(win);
}
void fl_sdl_desktop_size(int* w, int* h)
{
    SDL_DisplayMode m;
    if (SDL_GetDesktopDisplayMode(0, &m) == 0) { *w = m.w; *h = m.h; } else { *w = 1920; *h = 1080; }
}
static int g_wheelAccum = 0;
static unsigned g_latchedDown = 0;   // buttons pressed since the last poll (a click shorter than a frame must not be lost)
static unsigned ButtonBit(int b) { switch (b) { case SDL_BUTTON_LEFT: return 1; case SDL_BUTTON_RIGHT: return 2; case SDL_BUTTON_MIDDLE: return 4; case SDL_BUTTON_X1: return 8; case SDL_BUTTON_X2: return 16; } return 0; }
void fl_sdl_poll_events(FlSdlEvents* sink)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
        case SDL_QUIT: sink->Quit(); break;
        case SDL_WINDOWEVENT:
            if (ev.window.event == SDL_WINDOWEVENT_CLOSE) sink->Close();
            else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) sink->Focus(true);
            else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) sink->Focus(false);
            break;
        case SDL_MOUSEMOTION: sink->MouseMove(); break;
        case SDL_MOUSEWHEEL: g_wheelAccum += ev.wheel.y; break;
        case SDL_MOUSEBUTTONDOWN: g_latchedDown |= ButtonBit(ev.button.button); break;
        default: break;
        }
    }
}
void fl_sdl_destroy_window(FlSdlWindow w) { if (w) SDL_DestroyWindow(static_cast<SDL_Window*>(w)); }
void fl_sdl_set_window_rect(FlSdlWindow w, int x, int y, int cw, int ch, bool topmost)
{
    SDL_Window* win = static_cast<SDL_Window*>(w);
    if (!win) return;
    SDL_SetWindowPosition(win, x, y);
    SDL_SetWindowSize(win, cw, ch);
    SDL_SetWindowAlwaysOnTop(win, topmost ? SDL_TRUE : SDL_FALSE);
}
void fl_sdl_show_cursor(bool show) { SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE); }
bool fl_sdl_warp_cursor(FlSdlWindow, int x, int y) { SDL_WarpMouseGlobal(x, y); return true; }
bool fl_sdl_get_cursor(FlSdlWindow, int* x, int* y) { SDL_GetGlobalMouseState(x, y); return true; }

void fl_sdl_set_relative_mouse(FlSdlWindow w, bool enable)
{
    SDL_SetRelativeMouseMode(enable ? SDL_TRUE : SDL_FALSE);
    if (w) SDL_SetWindowGrab(static_cast<SDL_Window*>(w), enable ? SDL_TRUE : SDL_FALSE);
}
bool fl_sdl_window_has_focus(FlSdlWindow w)
{
    return w ? (SDL_GetWindowFlags(static_cast<SDL_Window*>(w)) & SDL_WINDOW_INPUT_FOCUS) != 0 : true;
}
unsigned fl_sdl_ticks() { return SDL_GetTicks(); }
void fl_sdl_pump_events()
{
    SDL_PumpEvents();
    SDL_Event ev;
    // Only the wheel is consumed here; other events stay queued for the (future) window-message bridge.
    while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEWHEEL, SDL_MOUSEWHEEL) > 0)
        g_wheelAccum += ev.wheel.y;
    while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONDOWN) > 0)
        g_latchedDown |= ButtonBit(ev.button.button);
}
bool fl_sdl_mouse_state(int* dx, int* dy, int* wheel, unsigned* buttons)
{
    fl_sdl_pump_events();
    SDL_GetRelativeMouseState(dx, dy);
    *wheel = g_wheelAccum;
    g_wheelAccum = 0;
    Uint32 b = SDL_GetMouseState(nullptr, nullptr);
    *buttons = ((b & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1u : 0u) | ((b & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 2u : 0u) |
               ((b & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 4u : 0u) | ((b & SDL_BUTTON(SDL_BUTTON_X1)) ? 8u : 0u) |
               ((b & SDL_BUTTON(SDL_BUTTON_X2)) ? 16u : 0u);
    *buttons |= g_latchedDown;   // reported down for this poll even if already released; up shows on the next one
    g_latchedDown = 0;
    return true;
}

static SDL_MetalView g_metalView = nullptr;
void* fl_sdl_metal_layer(FlSdlWindow w)
{
    if (!g_metalView) g_metalView = SDL_Metal_CreateView(static_cast<SDL_Window*>(w));
    return g_metalView ? SDL_Metal_GetLayer(g_metalView) : nullptr;
}
void fl_sdl_metal_drawable_size(FlSdlWindow w, int* pw, int* ph) { SDL_Metal_GetDrawableSize(static_cast<SDL_Window*>(w), pw, ph); }

bool fl_sdl_gl_create_context(FlSdlWindow w)
{
    SDL_GLContext ctx = SDL_GL_CreateContext(static_cast<SDL_Window*>(w));
    if (!ctx) return false;
    SDL_GL_SetSwapInterval(1);
    return true;
}
void fl_sdl_gl_swap(FlSdlWindow w) { SDL_GL_SwapWindow(static_cast<SDL_Window*>(w)); }
void fl_sdl_gl_drawable_size(FlSdlWindow w, int* pw, int* ph) { SDL_GL_GetDrawableSize(static_cast<SDL_Window*>(w), pw, ph); }

static SDL_Scancode ToScancode(int k)
{
    switch (k)
    {
    case FL_KEY_LSHIFT: return SDL_SCANCODE_LSHIFT;  case FL_KEY_RSHIFT: return SDL_SCANCODE_RSHIFT;
    case FL_KEY_LCTRL:  return SDL_SCANCODE_LCTRL;   case FL_KEY_RCTRL:  return SDL_SCANCODE_RCTRL;
    case FL_KEY_LALT:   return SDL_SCANCODE_LALT;    case FL_KEY_RALT:   return SDL_SCANCODE_RALT;
    case FL_KEY_ESCAPE: return SDL_SCANCODE_ESCAPE;  case FL_KEY_RETURN: return SDL_SCANCODE_RETURN;
    case FL_KEY_SPACE:  return SDL_SCANCODE_SPACE;   case FL_KEY_TAB:    return SDL_SCANCODE_TAB;
    }
    if (k >= FL_KEY_A && k < FL_KEY_A + 26) return (SDL_Scancode)(SDL_SCANCODE_A + (k - FL_KEY_A));
    if (k >= FL_KEY_0 && k < FL_KEY_0 + 10)
        return k == FL_KEY_0 ? SDL_SCANCODE_0 : (SDL_Scancode)(SDL_SCANCODE_1 + (k - FL_KEY_0 - 1));
    return SDL_SCANCODE_UNKNOWN;
}
bool fl_sdl_key_down(int k)
{
    SDL_Scancode sc = ToScancode(k);
    return sc != SDL_SCANCODE_UNKNOWN && SDL_GetKeyboardState(nullptr)[sc] != 0;
}

#else
// ============================================================================ SDL 1.2
// Written against the SDL 1.2 API; NOT compiled or tested in this repo yet. A single implicit
// display surface replaces SDL_Window: FlSdlWindow is just a non-null sentinel.
static int g_sdl1Window;

bool fl_sdl_init() { return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == 0; }
void fl_sdl_quit() { SDL_Quit(); }
const char* fl_sdl_error() { return SDL_GetError(); }

FlSdlWindow fl_sdl_create_window(const char* title, int w, int h, bool fullscreen, int surface)
{
    const bool glContext = surface == 1;     // SDL 1.2 has no Metal
    SDL_WM_SetCaption(title, nullptr);
    Uint32 flags = (glContext ? SDL_OPENGL : SDL_SWSURFACE) | (fullscreen ? SDL_FULLSCREEN : 0);
    if (!SDL_SetVideoMode(w, h, 32, flags)) return nullptr;
    return &g_sdl1Window;
}
void fl_sdl_set_window_mode(FlSdlWindow, bool fullscreen, int w, int h)
{
    SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE | (fullscreen ? SDL_FULLSCREEN : 0));
}
void fl_sdl_desktop_size(int* w, int* h)
{
    const SDL_VideoInfo* vi = SDL_GetVideoInfo();
    *w = vi ? vi->current_w : 1920; *h = vi ? vi->current_h : 1080;
}
void fl_sdl_poll_events(FlSdlEvents* sink)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        switch (ev.type)
        {
        case SDL_QUIT: sink->Quit(); break;
        case SDL_ACTIVEEVENT:
            if (ev.active.state & SDL_APPINPUTFOCUS) sink->Focus(ev.active.gain != 0);
            break;
        case SDL_MOUSEMOTION: sink->MouseMove(); break;
        default: break;
        }
    }
}
void fl_sdl_destroy_window(FlSdlWindow) {}
void fl_sdl_set_window_rect(FlSdlWindow, int, int, int w, int h, bool) { SDL_SetVideoMode(w, h, 32, SDL_SWSURFACE); }
void fl_sdl_show_cursor(bool show) { SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE); }
bool fl_sdl_warp_cursor(FlSdlWindow, int x, int y) { SDL_WarpMouse((Uint16)x, (Uint16)y); return true; }
bool fl_sdl_get_cursor(FlSdlWindow, int* x, int* y) { SDL_GetMouseState(x, y); return true; }

void fl_sdl_set_relative_mouse(FlSdlWindow, bool) {}   // TODO(SDL1): SDL_WM_GrabInput
bool fl_sdl_window_has_focus(FlSdlWindow) { return (SDL_GetAppState() & SDL_APPINPUTFOCUS) != 0; }
unsigned fl_sdl_ticks() { return SDL_GetTicks(); }
void fl_sdl_pump_events() { SDL_PumpEvents(); }
bool fl_sdl_mouse_state(int* dx, int* dy, int* wheel, unsigned* buttons)
{
    SDL_PumpEvents();
    SDL_GetRelativeMouseState(dx, dy);
    *wheel = 0;   // TODO(SDL1): wheel arrives as SDL_BUTTON_WHEELUP/DOWN button events
    Uint8 b = SDL_GetMouseState(nullptr, nullptr);
    *buttons = ((b & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1u : 0u) | ((b & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 2u : 0u) |
               ((b & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 4u : 0u);
    return true;
}

void* fl_sdl_metal_layer(FlSdlWindow) { return nullptr; }
void fl_sdl_metal_drawable_size(FlSdlWindow, int* w, int* h) { *w = *h = 0; }
// SDL 1.2: the GL context belongs to the video mode (SDL_OPENGL); nothing to create.
bool fl_sdl_gl_create_context(FlSdlWindow) { return true; }
void fl_sdl_gl_swap(FlSdlWindow) { SDL_GL_SwapBuffers(); }
void fl_sdl_gl_drawable_size(FlSdlWindow, int* w, int* h) { const SDL_VideoInfo* vi = SDL_GetVideoInfo(); *w = vi ? vi->current_w : 0; *h = vi ? vi->current_h : 0; }

static SDLKey ToKey(int k)
{
    switch (k)
    {
    case FL_KEY_LSHIFT: return SDLK_LSHIFT;  case FL_KEY_RSHIFT: return SDLK_RSHIFT;
    case FL_KEY_LCTRL:  return SDLK_LCTRL;   case FL_KEY_RCTRL:  return SDLK_RCTRL;
    case FL_KEY_LALT:   return SDLK_LALT;    case FL_KEY_RALT:   return SDLK_RALT;
    case FL_KEY_ESCAPE: return SDLK_ESCAPE;  case FL_KEY_RETURN: return SDLK_RETURN;
    case FL_KEY_SPACE:  return SDLK_SPACE;   case FL_KEY_TAB:    return SDLK_TAB;
    }
    if (k >= FL_KEY_A && k < FL_KEY_A + 26) return (SDLKey)(SDLK_a + (k - FL_KEY_A));
    if (k >= FL_KEY_0 && k < FL_KEY_0 + 10) return (SDLKey)(SDLK_0 + (k - FL_KEY_0));
    return SDLK_UNKNOWN;
}
bool fl_sdl_key_down(int k)
{
    SDLKey key = ToKey(k);
    return key != SDLK_UNKNOWN && SDL_GetKeyState(nullptr)[key] != 0;
}
#endif
