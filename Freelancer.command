#!/bin/bash
# Double-click to launch Freelancer (macOS port). The game expects to run from bin/.
ulimit -n 65536 2>/dev/null
cd "$(dirname "$0")/bin" || exit 1
exec ./freelancer "$@"
