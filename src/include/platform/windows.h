// Forwarding windows.h to win_compat.h on non-Windows platforms
#pragma once

#ifdef _WIN32
#include_next <windows.h>
#else
#include "win_compat.h"
#endif
