// Shim for <process.h>
#pragma once
#include "win_compat.h"
inline int _getpid() { return (int)GetCurrentProcessId(); }
