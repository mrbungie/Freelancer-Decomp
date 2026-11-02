#!/bin/bash
# Build the ASan binary, run the game with FL_MENU_STATE=${1:-16}, print the first AddressSanitizer report.
cd "$(dirname "$0")/../.." && cmake --build build-asan 2>&1 | grep -E " error " | head -3
FL_BIN=bin-asan FL_EXTRA_ENV="ASAN_OPTIONS=detect_leaks=0 FL_NO_CRASH_HANDLER=1 FL_MENU_STATE=${1:-16}" tools/port/flctl.sh try "wait ${2:-35}" >/dev/null 2>&1
grep -n "ERROR: AddressSanitizer" -A${3:-9} /tmp/fl_game.log | cut -c1-190
