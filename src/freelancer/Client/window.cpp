// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#include "globals.h"
#include "imeui.h"
#include "input_manager.h"
#include "nn_error.h"
#include "nn_exit.h"
#include "nn_network_operation.h"

#include "DALib.h"
#include "common_misc.h" // SinglePlayer
#include "render/IRenderPipeline.h"

#include "fl/platform.h"   // fl::GetWindowHost(), fl::IWindowEvents

DisplaySettings g_display = {};
ClientGlobals g_client = {};

// RenderMgr.cpp state (retail .data/.bss next to the functions below).
HWND G_RenderMgrWindow = nullptr;          // DAT_006679f4, written by FUN_004246e0
unsigned G_RenderSurfaceWidth = 0x280;     // DAT_00610850 (initialised .data)
unsigned G_RenderSurfaceHeight = 0x1e0;    // DAT_00610854 (initialised .data)
bool G_bRenderSurfaceFullscreen = false;   // DAT_006679f0
bool G_bRenderMgrShuttingDown = false;     // DAT_006679f9, FUN_004247c0's re-entry latch
bool G_bRenderPostCreate = true;           // DAT_00610864 (initialised .data 01h)
bool G_bRenderFrameBegun = false;          // DAT_006679f8 (.data, no initialiser)

// FUN_004246e0 (.text:004246e0-00424791), cdecl(Display, Title). A zero width or height in the options
// object is replaced by 640/480 locally only -- retail never writes it back. The window handle is
// also stored in RenderMgr's DAT_006679f4 (0x00424788).
// PORT: the window is created by the front end (fl::IWindowHost); the Win32 class name and the
// AdjustWindowRect/style arithmetic no longer apply -- the requested size is the client area.
HWND CreateGameWindow(DisplaySettings* display, const char* title)
{
    int width = display->width;
    if (width == 0)
        width = 0x280;
    int height = display->height;
    if (height == 0)
        height = 0x1e0;

    G_RenderMgrWindow = fl::GetWindowHost()->CreateGameWindow(title, width, height,
                                                              display->fullscreen != 0);
    return G_RenderMgrWindow;
}

// FUN_00424870 (.text:00424870-004249b8), cdecl(hwnd, fullscreen, width, height): records the
// surface size and mode, then restyles and resizes the window for it.
// PORT: the front end owns window styles; fullscreen is a desktop-resolution borderless window.
void ApplyGameWindowLayout(HWND Hwnd, bool bFullscreen, unsigned Width, unsigned Height)
{
    G_RenderSurfaceWidth = Width;
    G_RenderSurfaceHeight = Height;
    G_bRenderSurfaceFullscreen = bFullscreen;

    if (G_bRenderMgrShuttingDown && !bFullscreen)
        return;   // retail hid the window (SW_HIDE) while shutting the render manager down
    fl::GetWindowHost()->SetWindowMode(Hwnd, bFullscreen, static_cast<int>(Width),
                                       static_cast<int>(Height));
}

namespace
{
    void ReleaseRenderDeviceForDeactivate()
    {
        // DALib::Pipeline slot 14 (+0x38) — what LAB_005b26b0 calls before handing the
        // deactivation on to DefWindowProc.
        if (DALib::Pipeline == nullptr)
            return;
        DALib::Pipeline->DestroyGpuResources();
    }

    // LAB_005b2570 state.
    //
    // DAT_00615ba8 — the WinMain loop's present gate: `if (DAT_00615ba8) FUN_005b2890(dt);`
    // (0x5b0fe8). WM_ACTIVATEAPP is its only writer.
    bool G_bPresentAllowed = true;
    // DAT_00610865 — "application is active", latched by FUN_00424c60.
    bool G_bAppActive = true;
    // DAT_0067ecc8 / DAT_0067ecc9 — WinMain sets the first once InitializeEngine returns, and
    // SC_CLOSE sets the second when it arrives before that.
    bool G_bQuitDialogReady = false;
    bool G_bQuitDialogPending = false;

    // DAT_0067a438 is read exactly once in the whole PE (005b2667) and never written, so the
    // WM_ACTIVATEAPP branch it guards always takes the zero path. Kept as a named constant
    // rather than silently folded away.
    const bool kActivateOverrideByte = false;

    bool IsSinglePlayerSession()
    {
        // COMMON.DLL::SinglePlayer — retail default is true.
        return SinglePlayer();
    }

    void OnActivateAppState(bool bActive)
    {
        // LAB_005b264c. DAT_00615ba9 is the local "active" copy; the derived gate is
        // DAT_00615ba8, which stays 1 whenever the app is not a fullscreen single-player
        // session (there is nothing to release then).
        const bool bFullscreen = g_display.fullscreen != 0;
        if (!IsSinglePlayerSession() || (kActivateOverrideByte && !bFullscreen))
            G_bPresentAllowed = true;
        else
            G_bPresentAllowed = bActive;

        // FUN_00424c60: a fullscreen surface (DAT_006679f0, 0x00424c60) re-activated after it
        // was inactive clears the post-create flag DAT_00610864; then DAT_00610865 latches the
        // new state and FUN_00420350(bActive, w, h, fullscreen) takes or releases the mouse.
        if (G_bRenderSurfaceFullscreen && !G_bAppActive && bActive)
            G_bRenderPostCreate = false;
        G_bAppActive = bActive;
        MouseSetEnabled(bActive, static_cast<int>(G_RenderSurfaceWidth),
                        static_cast<int>(G_RenderSurfaceHeight), G_bRenderSurfaceFullscreen);
    }

    bool OnSysCommandClose()
    {
        // LAB_005b2727: FUN_00424cb0 gates on app-active + post-create + buffers-ready.
        // Then TEST EAX after FUN_00473ca0 (body = FUN_0059d9c0("NN_Error") — nonzero
        // swallows) and FUN_00473be0 ("NN_NetworkOperation"). Then FUN_00471f30 NN_Exit.
        if (!RenderMgrFrameReady())
            return false;
        if (NnErrorFind() != nullptr)      // FUN_00473ca0
            return false;
        if (NnNetworkOperationIsOpen())    // FUN_00473be0
            return false;
        if (!G_bQuitDialogReady)
        {
            G_bQuitDialogPending = true;
            return true;
        }
        RequestClientQuitDialog();
        return true;
    }
}

bool ClientPresentAllowed()
{
    return G_bPresentAllowed;
}

bool ClientAppIsActive()
{
    return G_bAppActive;
}

// FUN_00424cb0 (.text:00424cb0-00424cd3): three byte tests, 1 or 0.
bool RenderMgrFrameReady()
{
    return G_bAppActive && G_bRenderPostCreate && G_bRenderFrameBegun;
}

void ClientNoteEngineReady()
{
    // WinMain 0x5b0fbe: DAT_0067ecc8 = 1; if (DAT_0067ecc9) FUN_00471f30(0);
    G_bQuitDialogReady = true;
    if (G_bQuitDialogPending)
    {
        G_bQuitDialogPending = false;
        RequestClientQuitDialog();
    }
}

void RequestClientQuitDialog()
{
    // 005b274b / 005b0fc8: FUN_00471f30(0) -- raise the NN_Exit confirmation. Its confirm paths
    // call DestroyWindow on the game window themselves.
    NnExitOpen(false);
}

// Replaces ClientWndProc (LAB_005b2570): the front end calls these from PumpEvents.
// PORT: the IME's first look at every message (IMEFocus::WndProc) is not routed here yet, and
// WM_NCHITTEST / WM_ERASEBKGND / WM_SYSKEYUP / SC_SCREENSAVE have no meaning outside Windows.
namespace
{
    class FClientWindowEvents : public fl::IWindowEvents
    {
    public:
        void OnFocus(bool bGained) override
        {
            if (bGained)
                ClientInputOnFocusGained(); // caseD_7 -- FUN_0041eac0
            else
                ClientInputOnFocusLost();   // caseD_8 -- FUN_0041ecf0
        }
        void OnActivateApp(bool bActive) override
        {
            OnActivateAppState(bActive);
            if (!G_bAppActive && g_display.fullscreen != 0)
            {
                // DALib::Pipeline slot 14 (+0x38) -- release the device while minimised.
                ReleaseRenderDeviceForDeactivate();
            }
        }
        void OnCloseRequested() override { OnSysCommandClose(); }
        void OnMouseMove() override { ClientInputOnMouseMove(); }
    };
}

fl::IWindowEvents* ClientWindowEvents()
{
    static FClientWindowEvents Events;
    return &Events;
}
