// Freelancer Decomp: a reimplementation of the Freelancer (2003) game client and engine,
// faithful to the behaviour of the original game.
// This file was reconstructed by reverse engineering the binaries of Freelancer 1.1,
// developed by Digital Anvil.
// Author: Giperion.

#include "globals.h"
#include "input_manager.h"


#include "fl/platform.h"

// FUN_005b0b60 (.text:005b0b60-005b0c20), cdecl(int* ExitCode): drain the OS event queue. WM_QUIT
// (here: the front end's quit request) stores its code through ExitCode and returns false.
// PORT: retail also let IMEFocus claim hot keys and routed keyboard messages through the menu
// accelerator table (KeyMapRouteInputMessage, FUN_00577850); keyboard events are not delivered
// by the SDL front end yet (TODO: IWindowEvents::OnKey -> KeyMapRouteInputMessage).
bool PumpMessages(int* exit_code)
{
    // 0x005b0b69: FUN_0041ef20 drains the mouse queue first.
    MouseDispatchEvents();
    return fl::GetWindowHost()->PumpEvents(ClientWindowEvents(), exit_code);
}
