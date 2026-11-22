#!/usr/bin/env bash
# Per-slot Undo (issue #14): load a quiet sine, Select All + Normalize, then Undo/Redo and switch slots.
# The "how much waveform is drawn" measure is the count of black pixels in the zoom view, so Undo is checked pixel-for-pixel.
# Uses keyboard shortcuts only (the file is loaded through RUNCMD, where the pull-down menus don't open):
#   Ctrl-A select all, N normalize, Scroll Lock = the ST's UNDO key, 1/2 = switch sample slot.
#   tests/ui/undo_test.sh --prg out/CYBR160.PRG [--tos img] [--workdir DIR] [--keep]
set -uo pipefail
source "$(dirname "$0")/common.sh"
ui_args "$@" || exit 2
trap ui_stop EXIT
python3 "$UI_HERE/make_sd.py" "$WORK/T.SD" 8000 440 1
LOAD="$WORK/T.SD"; ui_start
zoom() { dark_pixels "$(shot "$1")" 232 372 790 482; }          # inside the lower (zoomed) waveform box
a=$(zoom a_loaded)
key ctrl+a; KEYWAIT=3 key n;          b=$(zoom b_normalized)
KEYWAIT=3 key Scroll_Lock;            c=$(zoom c_undone)
KEYWAIT=3 key Scroll_Lock;            d=$(zoom d_redone)
key 2; KEYWAIT=2.5 key Scroll_Lock;   e=$(zoom e_slot2_empty_undo_noop)
key 1;                                f=$(zoom f_slot1_again)
KEYWAIT=3 key Scroll_Lock;            g=$(zoom g_slot1_undone)
echo "dark pixels: loaded=$a normalized=$b undone=$c redone=$d slot2=$e slot1=$f slot1-undone=$g"
expect "normalize makes the waveform much larger" gt "$b" $((a * 2))
expect "Undo restores the original view pixel-for-pixel" eq "$c" "$a"
expect "second Undo (= Redo) restores the normalized view" eq "$d" "$b"
expect "empty slot 2 shows no waveform and Undo there does nothing" lt "$e" 100
expect "back in slot 1 the normalized sample is still there" eq "$f" "$b"
expect "slot 1 still has its own Undo" eq "$g" "$a"
ui_finish $((EXPECT_FAILS > 0))
