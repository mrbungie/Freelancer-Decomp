#!/bin/bash
# Drive the macOS port without touching the real mouse.
#   flctl.sh run            launch the game in the background (control fifo enabled), wait for the menu
#   flctl.sh click X Y      click at window-client point X,Y (game surface 1024x768)
#   flctl.sh move X Y
#   flctl.sh shot [file]    window-only screenshot (default /tmp/fl_shot.png)
#   flctl.sh log [N]        last N lines of the game log (default 15)
#   flctl.sh try 'click X Y' 'wait 2' 'shot' 'log 8'   one-shot scenario, always quits
#   flctl.sh quit           stop the game
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"; FIFO=/tmp/fl.cmd; LOG=/tmp/fl_game.log
case "$1" in
  run) pkill -9 -x freelancer 2>/dev/null; rm -f $FIFO
       (cd "$ROOT/${FL_BIN:-bin}" && env FL_CONTROL=$FIFO $FL_EXTRA_ENV nohup ./freelancer >/dev/null 2>$LOG &); sleep 5; sleep 1; echo started;;
  click|move) timeout 3 bash -c "echo '$1 $2 $3' > $FIFO"; sleep 1;;
  shot) f="${2:-/tmp/fl_shot.png}"; rm -f "$f"; timeout 3 bash -c "echo 'shot $f' > $FIFO"
        for _ in $(seq 40); do [ -s "$f" ] && break; sleep 0.1; done; echo "$f";;   # the game writes its own frame
  log) tail -n "${2:-15}" $LOG | cut -c1-200;;
  quit) timeout 3 bash -c "echo quit > $FIFO" ; sleep 1; pkill -9 -x freelancer 2>/dev/null; echo stopped;;
  try) shift; "$0" run >/dev/null; for st in "$@"; do case "$st" in
         click*|move*) set -- $st; "$0" $1 $2 $3;; wait*) sleep ${st#wait };; shot*) "$0" shot ${st#shot };; log*) "$0" log ${st#log };; esac; done; "$0" quit >/dev/null;;
  *) sed -n 2,8p "$0";;
esac
