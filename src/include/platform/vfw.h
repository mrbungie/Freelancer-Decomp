// Shim for <vfw.h>: MCIWnd (video playback child window) is unavailable; creation fails.
#pragma once
#include "win_compat.h"
#define MCIWNDF_NOMENU 0x0008
#define MCIWNDF_NOPLAYBAR 0x0002
#define MCIWNDF_NOERRORDLG 0x4000
inline HWND MCIWndCreateA(HWND, HINSTANCE, DWORD, const char*) { fl::ReportUnimplemented("MCIWndCreateA"); return nullptr; }
