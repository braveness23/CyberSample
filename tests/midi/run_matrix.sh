#!/usr/bin/env bash
# Run the MIDI loopback test over the interesting combinations of (format served on Get, format chosen on Send,
# truncate-at-loop-end).  Each run boots Hatari, so the whole matrix takes ~8 minutes.
#   tests/midi/run_matrix.sh --prg out/CYBR160.PRG [--tos img]      (extra options are passed to run_loopback.sh)
set -uo pipefail
HERE=$(cd "$(dirname "$0")" && pwd); fail=0
for cfg in "16 16 0" "16 16 1" "16 12 0" "16 12 1" "12 16 0" "12 12 1"; do
  read -r serve send trunc <<<"$cfg"
  echo "=== served $serve-bit, Send format $send-bit, truncate-at-loop-end=$trunc ==="
  out=$("$HERE/run_loopback.sh" "$@" --serve-bits "$serve" --send-format "$send" --trunc "$trunc" 2>&1) || fail=1
  echo "$out" | grep -E "^(ok|FAIL|PASS|FAILED)"
done
[ $fail = 0 ] && echo "MATRIX PASS" || echo "MATRIX FAILED"
exit $fail
