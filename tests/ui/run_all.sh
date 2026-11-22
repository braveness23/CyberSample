#!/usr/bin/env bash
# All emulator tests that need a built .PRG:  smoke, per-slot undo, MIDI loopback matrix.
#   tests/ui/run_all.sh --prg out/CYBR160.PRG [--tos img]
HERE=$(cd "$(dirname "$0")" && pwd); rc=0
for t in "$HERE/smoke.sh" "$HERE/undo_test.sh"; do echo "##### $(basename "$t")"; "$t" "$@" || rc=1; done
echo "##### midi loopback matrix"; "$HERE/../midi/run_matrix.sh" "$@" || rc=1
[ $rc = 0 ] && echo "ALL EMULATOR TESTS PASS" || echo "SOME EMULATOR TESTS FAILED"; exit $rc
