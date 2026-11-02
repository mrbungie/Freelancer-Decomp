#!/bin/bash
# Run the normal (non-ASan) build under lldb until it crashes; print a symbolized backtrace with source lines.
# usage: crash.sh [menu_state=16] [timeout_s=60]
cd "$(dirname "$0")/../../bin" || exit 1
pkill -9 -x freelancer 2>/dev/null
FL_NO_CRASH_HANDLER=1 FL_MENU_STATE=${1:-16} timeout ${2:-60} lldb -b -o 'process handle SIGBUS SIGSEGV -s true' -o run -o 'bt 14' -o 'frame variable' -o 'process kill' -o quit ./freelancer 2>&1 | grep -E "stop reason|frame #|error:|= " | cut -c1-200 | head -${3:-40}
pkill -9 -x freelancer 2>/dev/null
