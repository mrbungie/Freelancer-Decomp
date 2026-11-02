// SDL version abstraction for the front end.
//
// FL_SDL_VERSION selects the API generation at configure time (-DFL_SDL_MAJOR=2 | 1; CMake
// defines FL_SDL_VERSION from it). The front end only touches SDL through the fl_sdl_* functions below, so a
// platform that ships SDL 1.2 (e.g. an Xbox 360 homebrew SDL port -- unverified) needs only
// the SDL1 branch of fl_sdl.cpp to be correct, nothing in the game or in sdl_main.cpp changes.
//
// Capability macros let shared code ask "can this SDL do X" instead of testing the version:
//   FL_SDL_HAS_MULTI_WINDOW   SDL_Window objects (SDL2). SDL1 has a single implicit surface.
//   FL_SDL_HAS_GAMECONTROLLER SDL_GameController (SDL2). SDL1 exposes raw SDL_Joystick only.
//   FL_SDL_HAS_GLOBAL_MOUSE   SDL_GetGlobalMouseState/WarpMouseGlobal (SDL2).
#pragma once

#ifndef FL_SDL_VERSION
#  define FL_SDL_VERSION 2
#endif

#if FL_SDL_VERSION == 2
#  include <SDL.h>
#  define FL_SDL_HAS_MULTI_WINDOW 1
#  define FL_SDL_HAS_GAMECONTROLLER 1
#  define FL_SDL_HAS_GLOBAL_MOUSE 1
#elif FL_SDL_VERSION == 1
#  include <SDL/SDL.h>
#  define FL_SDL_HAS_MULTI_WINDOW 0
#  define FL_SDL_HAS_GAMECONTROLLER 0
#  define FL_SDL_HAS_GLOBAL_MOUSE 0
#else
#  error "FL_SDL_VERSION must be 1 or 2"
#endif

// Opaque window token handed to the game as HWND and to the render backend.
// SDL2: SDL_Window*.  SDL1: a non-null sentinel (the display surface is global).
typedef void* FlSdlWindow;

bool        fl_sdl_init();                                   // video + events (+ controllers where available)
void        fl_sdl_quit();
// surface: 0 plain, 1 OpenGL-capable, 2 Metal-capable (SDL2 on macOS/iOS).
FlSdlWindow fl_sdl_create_window(const char* title, int w, int h, bool fullscreen, int surface);
void        fl_sdl_set_window_mode(FlSdlWindow, bool fullscreen, int w, int h);
void        fl_sdl_desktop_size(int* w, int* h);
// Events the front end forwards (keyboard is added with the menu key-routing port).
struct FlSdlEvents
{
    virtual ~FlSdlEvents() {}
    virtual void Close() = 0;
    virtual void Focus(bool gained) = 0;
    virtual void MouseMove() = 0;
    virtual void Quit() = 0;
};
void        fl_sdl_poll_events(FlSdlEvents* sink);
void*       fl_sdl_metal_layer(FlSdlWindow);          // CAMetalLayer* (creates the SDL metal view on first use)
void        fl_sdl_metal_drawable_size(FlSdlWindow, int* w, int* h);
bool        fl_sdl_gl_create_context(FlSdlWindow);   // creates + makes current
void        fl_sdl_gl_swap(FlSdlWindow);
void        fl_sdl_gl_drawable_size(FlSdlWindow, int* w, int* h);
void        fl_sdl_destroy_window(FlSdlWindow);
void        fl_sdl_set_window_rect(FlSdlWindow, int x, int y, int w, int h, bool topmost);
void        fl_sdl_show_cursor(bool show);
bool        fl_sdl_warp_cursor(FlSdlWindow, int x, int y);   // window-relative on SDL1
bool        fl_sdl_get_cursor(FlSdlWindow, int* x, int* y);
bool        fl_sdl_key_down(int flKey);                      // FlKey (below), translated per SDL version
const char* fl_sdl_error();
void        fl_sdl_pump_events();                            // pump the OS queue (needed for fresh mouse/key state)
// Mouse: movement since the previous call (relative), wheel clicks since the previous call, and the
// button bitmask (bit0 left, bit1 right, bit2 middle, bit3+ extra).
void        fl_sdl_set_relative_mouse(FlSdlWindow, bool enable);
bool        fl_sdl_window_has_focus(FlSdlWindow);
unsigned    fl_sdl_ticks();
bool        fl_sdl_mouse_state(int* dx, int* dy, int* wheel, unsigned* buttons);

// Keys the front end maps Win32 VK_* onto; defined per version in fl_sdl.cpp.
enum FlKey { FL_KEY_LSHIFT, FL_KEY_RSHIFT, FL_KEY_LCTRL, FL_KEY_RCTRL, FL_KEY_LALT, FL_KEY_RALT,
             FL_KEY_ESCAPE, FL_KEY_RETURN, FL_KEY_SPACE, FL_KEY_TAB, FL_KEY_A /* + 0..25 */, FL_KEY_0 = FL_KEY_A + 26 /* + 0..9 */ };
