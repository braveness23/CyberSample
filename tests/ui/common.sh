# Shared helpers for the emulator GUI tests (source this file).  See docs/testing.md.
#
# Hatari runs headless on its own Xvfb display; xdotool injects keys/mouse, ImageMagick `import` takes screenshots.
# Everything started here is put in ONE process group so it can be killed together - never `pkill -f` by name
# (over ssh it matches its own shell, and by name it can hit unrelated emulators).
#
# Inputs (set before calling ui_start):  PRG (built CYBR160.PRG), TOS (EmuTOS/TOS image), WORK (scratch dir)
# Optional:  DISP (X display number, default 94), FAST (true|false: Hatari fast-forward, default true),
#            LOAD (a .SD file to open at start: launches through the RUNCMD helper, see below), EXTRA (more hatari options)
UI_HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
UI_ROOT=$(cd "$UI_HERE/../.." && pwd)
DISP=${DISP:-94}; FAST=${FAST:-true}

ui_need() { for c in hatari Xvfb xdotool import python3; do command -v $c >/dev/null || { echo "missing: $c" >&2; return 1; }; done; }

# ui_start: boot CYBR160.PRG in Hatari.
#  * Without LOAD the program is the Hatari --auto program, so the GEM menus work (hover to open them).
#  * With LOAD the program is started by RUNCMD.TOS (ci/runcmd) with the file as its argument, because --auto can't pass
#    arguments.  KNOWN LIMIT: under RUNCMD the pull-down menus do NOT open, so such tests use keyboard shortcuts only.
ui_start() {
  mkdir -p "$WORK/C" "$WORK/shots"; cp "$PRG" "$WORK/C/CYBR160.PRG"
  local auto='C:\CYBR160.PRG'
  if [ -n "${LOAD:-}" ]; then
    mkdir -p "$WORK/C/RUNCMD"; cp "$UI_ROOT/ci/runcmd/RUNCMD.TOS" "$WORK/C/RUNCMD/"; cp "$LOAD" "$WORK/C/T.SD"
    printf 'C:\\CYBR160.PRG C:\\T.SD\r\n' > "$WORK/C/BUILD.CMD"; auto='C:\RUNCMD\RUNCMD.TOS'
  fi
  export SDL_AUDIODRIVER=dummy
  setsid bash -c "Xvfb :$DISP -screen 0 1024x768x24 >/dev/null 2>&1 &
    sleep 1
    DISPLAY=:$DISP hatari --monitor mono --machine st --tos '$TOS' --harddrive '$WORK/C' --auto '$auto' \
      --window --confirm-quit off --fast-forward $FAST --fast-boot $FAST --statusbar off ${EXTRA:-} >'$WORK/hatari.log' 2>&1 &
    wait" >/dev/null 2>&1 &
  UI_PGID=$!
  export DISPLAY=:$DISP
  for i in $(seq 1 40); do [ -e /tmp/.X11-unix/X$DISP ] && break; sleep 0.5; done
  sleep 22                                    # EmuTOS boot + program start (fast-forward)
}
ui_stop()  { [ -n "${UI_PGID:-}" ] && kill -- -"$UI_PGID" 2>/dev/null; UI_PGID=; }
ui_clean() { ui_stop; }

# Atari screen = 640x400 at offset (192,184) in the 1024x768 Xvfb; coordinates below are Xvfb coordinates.
shot()  { import -window root "$WORK/shots/$1.png"; echo "$WORK/shots/$1.png"; }
key()   { xdotool key "$@"; sleep "${KEYWAIT:-2}"; }                       # e.g. key ctrl+a ; key Scroll_Lock (= ST Undo)
click() { xdotool mousemove "$1" "$2"; sleep 0.3; xdotool mousedown 1; sleep 0.15; xdotool mouseup 1; sleep "${3:-0.8}"; }
# GEM menus open on hover: move onto the title, then jiggle by 2px.  (Direct launch only, see ui_start.)
menu()  { xdotool mousemove "$1" "$2"; sleep 0.8; xdotool mousemove $(( $1 + 2 )) $(( $2 + 2 )); sleep "${3:-2}"; }

# dark_pixels IMG X0 Y0 X1 Y1 : count of black pixels in a rectangle - a robust "how much waveform is drawn" measure
dark_pixels() { python3 - "$@" <<'PYEOF'
import sys
from PIL import Image
f, x0, y0, x1, y1 = sys.argv[1], *map(int, sys.argv[2:6])
im = Image.open(f).convert("L"); px = im.load()
print(sum(1 for y in range(y0, y1) for x in range(x0, x1) if px[x, y] < 64))
PYEOF
}
# image_diff A B : number of differing pixels between two screenshots (0 = identical)
image_diff() { python3 - "$@" <<'PYEOF'
import sys
from PIL import Image, ImageChops
a, b = Image.open(sys.argv[1]).convert("L"), Image.open(sys.argv[2]).convert("L")
d = ImageChops.difference(a, b).point(lambda v: 255 if v > 32 else 0)
print(d.histogram()[255])
PYEOF
}

# ui_args "$@": common command line for the test scripts:  --prg FILE --tos FILE [--workdir DIR] [--keep] [--display N] [--fast true|false]
ui_args() {
  PRG=; TOS=; WORK=; KEEP=0
  while [ $# -gt 0 ]; do case $1 in
    --prg) PRG=$2; shift 2;; --tos) TOS=$2; shift 2;; --workdir) WORK=$2; shift 2;; --keep) KEEP=1; shift;;
    --display) DISP=$2; shift 2;; --fast) FAST=$2; shift 2;; *) echo "unknown option $1" >&2; return 2;; esac; done
  [ -f "$PRG" ] || { echo "--prg must be a built CYBR160.PRG (ci/build.sh)" >&2; return 2; }
  [ -n "$TOS" ] || TOS=$(ls "$UI_ROOT"/ci/.cache/emutos-256k-1.3/etos256us.img 2>/dev/null || true)
  [ -f "$TOS" ] || { echo "--tos: need a TOS/EmuTOS image (ci/build.sh downloads EmuTOS 1.3 into ci/.cache)" >&2; return 2; }
  [ -n "$WORK" ] || WORK=$(mktemp -d); mkdir -p "$WORK"
  ui_need
}
ui_finish() { local rc=$1; ui_stop; if [ "$rc" != 0 ]; then echo "screenshots kept in $WORK/shots"; elif [ "$KEEP" = 0 ]; then rm -rf "$WORK"; fi; exit "$rc"; }
EXPECT_FAILS=0
expect() {   # expect "description" condition-command...   (counts failures, keeps going)
  local msg=$1; shift
  if "$@"; then echo "ok   $msg"; else echo "FAIL $msg"; EXPECT_FAILS=$((EXPECT_FAILS + 1)); fi
}
lt() { [ "$1" -lt "$2" ]; }  ; gt() { [ "$1" -gt "$2" ]; }  ; eq() { [ "$1" = "$2" ]; }
