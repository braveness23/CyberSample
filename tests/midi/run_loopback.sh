#!/usr/bin/env bash
# MIDI loopback test: drives the real CYBR160.PRG in Hatari through File > Get and File > Send against a virtual
# SDS sampler (sds_sampler.py) connected to Hatari's --midi-in/--midi-out FIFOs, then checks what came back.
#
#   run_loopback.sh --prg out/CYBR160.PRG [--tos etos256us.img] [--serve-bits 16] [--send-format 16] [--trunc 0]
#                   [--workdir DIR] [--fast false] [--keep] [--rt-splice f8 [--rt-every 40]]
#
#   --serve-bits   SDS format the virtual sampler offers on Get (12 or 16)
#   --send-format  Format radio button chosen in the Send dialog (12 or 16)
#   --rt-splice    real-time byte the sampler inserts inside every sysex it serves (MIDI clock/active sensing on the line)
#   --trunc        1 = leave "Sample Length Truncated at Loop End" ticked (default in the program), 0 = untick it
# Needs: hatari, Xvfb, xdotool, ImageMagick `import`, python3.  Not run in CI (needs a built .PRG and an emulator).
# GUI coordinates are for the 640x400 mono screen at its fixed position in a 1024x768 Xvfb.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
PRG=; TOS=; SERVE=16; FMT=16; TRUNC=1; WORK=; FAST=true; KEEP=0; DISP=95; RT=; RTN=40
while [ $# -gt 0 ]; do case $1 in
  --prg) PRG=$2; shift 2;; --tos) TOS=$2; shift 2;; --serve-bits) SERVE=$2; shift 2;; --send-format) FMT=$2; shift 2;;
  --trunc) TRUNC=$2; shift 2;; --workdir) WORK=$2; shift 2;; --fast) FAST=$2; shift 2;; --keep) KEEP=1; shift;;
  --display) DISP=$2; shift 2;; --rt-splice) RT=$2; shift 2;; --rt-every) RTN=$2; shift 2;; *) echo "unknown option $1" >&2; exit 2;; esac; done
[ -f "$PRG" ] || { echo "--prg must be a built CYBR160.PRG" >&2; exit 2; }
[ -n "$TOS" ] || TOS=$(ls "$HERE"/../../ci/.cache/emutos-256k-1.3/etos256us.img 2>/dev/null || true)
[ -f "$TOS" ] || { echo "--tos: need a TOS/EmuTOS image (ci/build.sh downloads one into ci/.cache)" >&2; exit 2; }
[ -n "$WORK" ] || WORK=$(mktemp -d); mkdir -p "$WORK/C" "$WORK/shots"
cleanup(){ [ -n "${PGID:-}" ] && kill -- -"$PGID" 2>/dev/null || true; [ "$KEEP" = 1 ] || rm -rf "$WORK"; }
trap cleanup EXIT
cp "$PRG" "$WORK/C/CYBR160.PRG"; rm -f "$WORK/from_atari" "$WORK/to_atari" "$WORK/result.json" "$WORK/wire.log"
mkfifo "$WORK/from_atari" "$WORK/to_atari"
export SDL_AUDIODRIVER=dummy
# one process group for sampler + Xvfb + Hatari so everything can be killed together (never pkill by name)
setsid bash -c "
  python3 '$HERE/sds_sampler.py' --from-atari '$WORK/from_atari' --to-atari '$WORK/to_atari' --log '$WORK/wire.log' \
      --result '$WORK/result.json' --bits $SERVE --run-for 150 ${RT:+--rt-splice $RT --rt-every $RTN} &
  Xvfb :$DISP -screen 0 1024x768x24 >/dev/null 2>&1 &
  sleep 1
  DISPLAY=:$DISP hatari --monitor mono --machine st --tos '$TOS' --harddrive '$WORK/C' --auto 'C:\\CYBR160.PRG' \
      --midi-in '$WORK/to_atari' --midi-out '$WORK/from_atari' --window --confirm-quit off \
      --fast-forward $FAST --fast-boot $FAST --statusbar off >'$WORK/hatari.log' 2>&1 &
  wait" >/dev/null 2>&1 &
PGID=$!
export DISPLAY=:$DISP
shot(){ import -window root "$WORK/shots/$1.png" 2>/dev/null || true; }
click(){ xdotool mousemove "$1" "$2"; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1; sleep 0.8; }
for i in $(seq 1 40); do [ -e /tmp/.X11-unix/X$DISP ] && break; sleep 0.5; done
sleep 22                                                        # EmuTOS + CyberSample boot
shot 0_boot
xdotool key ctrl+g; sleep 2; shot 1_get_dialog                   # File > Get (shortcut)
xdotool key Return                                               # OK: sample 0, device 0
for i in $(seq 1 60); do grep -q "serve complete\|FAIL" "$WORK/wire.log" 2>/dev/null && break; sleep 1; done
sleep 4; shot 2_after_get
xdotool mousemove 335 193; sleep 0.8; xdotool mousemove 337 195; sleep 2   # open the File menu
click 340 403; sleep 2; shot 3_send_dialog                       # File > Send...
[ "$TRUNC" = 0 ] && click 344 402                                # untick "Truncated at Loop End"
if [ "$FMT" = 16 ]; then click 519 433; else click 430 433; fi   # Format radio
shot 4_send_dialog_set
click 440 497                                                    # OK
for i in $(seq 1 90); do [ -s "$WORK/result.json" ] && break; sleep 1; done
sleep 1; shot 5_after_send
RC=0
if [ -s "$WORK/result.json" ]; then
  python3 "$HERE/check_result.py" "$WORK/result.json" --serve-bits "$SERVE" --send-format "$FMT" --trunc "$TRUNC" || RC=$?
else
  echo "FAIL: the virtual sampler never received a complete dump"; RC=1
fi
if [ $RC -ne 0 ]; then echo "--- wire log (tail) ---"; tail -15 "$WORK/wire.log" 2>/dev/null || true; KEEP=1; echo "kept $WORK"; fi
exit $RC
