#!/usr/bin/env bash
# MIDI thru stress test (issue #24): a sequencer's clock (F8, 48 bytes/s) runs into the ST's MIDI in with
# Options > Midi Thru on (the default) while File > Get is opened and cancelled repeatedly.
# v1.60 crashed the machine here (EmuTOS "Bus Error ... System halted"): the thru handler called the BIOS
# from the MIDI interrupt. Checks: no crash, the clock is echoed while no dialog is open, and thru resumes
# after every Cancel (during Get the program owns the MIDI input, so no echo then is expected).
#
#   run_thru_stress.sh --prg out/CYBR160.PRG [--tos etos256us.img] [--cycles 6] [--workdir DIR] [--display N] [--keep]
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
PRG=; TOS=; CYCLES=6; WORK=; KEEP=0; DISP=96
while [ $# -gt 0 ]; do case $1 in
  --prg) PRG=$2; shift 2;; --tos) TOS=$2; shift 2;; --cycles) CYCLES=$2; shift 2;; --workdir) WORK=$2; shift 2;;
  --display) DISP=$2; shift 2;; --keep) KEEP=1; shift;; *) echo "unknown option $1" >&2; exit 2;; esac; done
[ -f "$PRG" ] || { echo "--prg must be a built CYBR160.PRG" >&2; exit 2; }
[ -n "$TOS" ] || TOS=$(ls "$HERE"/../../ci/.cache/emutos-256k-1.3/etos256us.img 2>/dev/null || true)
[ -f "$TOS" ] || { echo "--tos: need a TOS/EmuTOS image (ci/build.sh downloads one into ci/.cache)" >&2; exit 2; }
[ -n "$WORK" ] || WORK=$(mktemp -d); mkdir -p "$WORK/C" "$WORK/shots"
cleanup(){ [ -n "${PGID:-}" ] && kill -- -"$PGID" 2>/dev/null || true; [ -n "${FPID:-}" ] && kill "$FPID" 2>/dev/null || true
           [ "$KEEP" = 1 ] || rm -rf "$WORK"; }
trap cleanup EXIT
cp "$PRG" "$WORK/C/CYBR160.PRG"; rm -f "$WORK/from_atari" "$WORK/to_atari" "$WORK/wire.log" "$WORK/phases.log"
mkfifo "$WORK/from_atari" "$WORK/to_atari"
# Hatari opens both FIFOs at start-up and blocks until the other end exists; hold them open here (the
# feeder attaches only after the boot, so it doesn't stream clock into EmuTOS while it starts).
exec 3<>"$WORK/to_atari" 4<>"$WORK/from_atari"
export SDL_AUDIODRIVER=dummy
# Real speed (no fast-forward): the point is MIDI interrupts arriving while GEM is busy, in real time.
setsid bash -c "
  Xvfb :$DISP -screen 0 1024x768x24 >/dev/null 2>&1 &
  sleep 1
  DISPLAY=:$DISP hatari --monitor mono --machine st --tos '$TOS' --harddrive '$WORK/C' --auto 'C:\\CYBR160.PRG' \
      --midi-in '$WORK/to_atari' --midi-out '$WORK/from_atari' --window --confirm-quit off \
      --fast-forward false --fast-boot false --statusbar off >'$WORK/hatari.log' 2>&1 &
  wait" >/dev/null 2>&1 &
PGID=$!
export DISPLAY=:$DISP
phase(){ echo "$(date +%s.%N) $1" >> "$WORK/phases.log"; }
for i in $(seq 1 40); do [ -e /tmp/.X11-unix/X$DISP ] && break; sleep 0.5; done
sleep 40                                                        # EmuTOS + CyberSample boot at real speed
python3 "$HERE/clock_feeder.py" --from-atari "$WORK/from_atari" --to-atari "$WORK/to_atari" \
    --seconds $(( 8 + CYCLES * 4 )) --log "$WORK/wire.log" & FPID=$!
phase idle; sleep 3
for i in $(seq 1 "$CYCLES"); do
  phase "open$i";   xdotool key ctrl+g; sleep 1.5
  xdotool mousemove 579 489; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1   # Get dialog: Cancel
  phase "closed$i"; sleep 2.5
done
phase end; sleep 1
import -window root "$WORK/shots/end.png" 2>/dev/null || true
wait "$FPID" 2>/dev/null || true; FPID=
python3 - "$WORK" "$CYCLES" <<'EOF'
import sys
work, cycles = sys.argv[1], int(sys.argv[2])
crash = [l for l in open(f"{work}/hatari.log") if "Bus Error writing" in l]
ph = [(float(t), n) for t, n in (l.split() for l in open(f"{work}/phases.log"))]
ev = [l.split() for l in open(f"{work}/wire.log")]
def count(kind, a, b): return sum(1 for e in ev if e[1] == kind and a <= float(e[0]) < b)
rc = 0
print(("FAIL" if crash else "ok  ") + "  no system crash" + (f" ({crash[0].strip()})" if crash else "")); rc |= bool(crash)
for (t, name), (t2, _) in zip(ph, ph[1:]):
    if not (name == "idle" or name.startswith("closed")): continue
    sent, echoed = count("TX", t + 0.3, t2), count("RX", t + 0.3, t2 + 0.05)   # skip 0.3 s for the dialog to close
    ok = sent > 0 and echoed >= 0.8 * sent
    print(("ok  " if ok else "FAIL") + f"  {name:8s} thru echoed {echoed}/{sent} clocks"); rc |= not ok
print("PASS" if rc == 0 else "FAILED"); sys.exit(rc)
EOF
