#!/usr/bin/env bash
# Host-side tests for code in src/ that doesn't need GEM or an emulator:
#   dsp_test  - clip16, the one-pole filter and the gate from src/EDIT.C
#   ccp_test  - Cut/Copy from src/CCP.C with the sample-slot helpers from src/MISC.C
# The code under test is extracted verbatim from the sources, so this checks the same text the Atari build compiles.
# Needs only gcc, libm and python3 - no Lattice C or emulator.
#   CCP_SRC=path/to/CCP.C tests/run.sh   # run the Cut/Copy test against another revision of CCP.C
set -euo pipefail
cd "$(dirname "$0")/.."
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT

python3 - "$tmp/dsp_extract.h" "$tmp/ccp_extract.h" "${CCP_SRC:-src/CCP.C}" <<'PYEOF'
import sys

def read(path):
    return open(path, encoding="latin-1").read().replace("\r\n", "\n")

# DSP: from clip16 through the DSP-END marker in EDIT.C
s = read("src/EDIT.C")
a = s.index("static short\nclip16"); b = s.index("/* DSP-END */") + len("/* DSP-END */")
open(sys.argv[1], "w").write(s[a:b] + "\n")

# Cut/Copy: named functions, each from its "type\nname(" line to the closing "}" at column 0
def grab(path, names):
    s = read(path); out = []
    for n in names:
        i = s.index("\n" + n + "(")
        a = s.rfind("\n", 0, i) + 1
        b = s.index("\n}\n", i) + 3
        out.append(s[a:b])
    return "\n".join(out)

body = grab("src/MISC.C", ["sample_length", "edit_length", "off_addr", "new_sample", "init_sample_parameters",
                           "reset_filename", "reset_view_params", "reset_edit_params",
                           "reset_loop1_params", "reset_loop2_params"])
body += "\n" + grab(sys.argv[3], ["hndl_cut", "hndl_copy"])
open(sys.argv[2], "w").write(body)
PYEOF

${CC:-gcc} -O1 -g -fsanitize=address,undefined -I"$tmp" -o "$tmp/dsp_test" tests/dsp_test.c -lm
ASAN_OPTIONS=detect_leaks=0 "$tmp/dsp_test"

# leak detection stays ON for this one: the allocation-failure paths must not leak
${CC:-gcc} -O1 -g -fsanitize=address,undefined -I"$tmp" -o "$tmp/ccp_test" tests/ccp_test.c
"$tmp/ccp_test"
