#!/usr/bin/env bash
# Host-side tests for the pure DSP routines in src/EDIT.C (clip16, onepole_filter, gate_filter).
# The code under test is extracted verbatim from EDIT.C, so this checks the same source the Atari build compiles.
# Needs only gcc and libm - no Lattice C or emulator.
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
python3 - "$tmp/dsp_extract.h" <<'PY'
import sys
s = open("src/EDIT.C", encoding="latin-1").read().replace("\r\n", "\n")
a = s.index("static short\nclip16"); b = s.index("/* DSP-END */") + len("/* DSP-END */")
open(sys.argv[1], "w").write(s[a:b] + "\n")
PY
${CC:-gcc} -O1 -g -fsanitize=address,undefined -I"$tmp" -o "$tmp/dsp_test" tests/dsp_test.c -lm
ASAN_OPTIONS=detect_leaks=0 "$tmp/dsp_test"
