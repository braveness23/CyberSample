#!/usr/bin/env bash
# Smoke test: the built program boots in Hatari, draws its window, and the About dialog matches the reference screenshot.
#   tests/ui/smoke.sh --prg out/CYBR160.PRG [--tos img] [--workdir DIR] [--keep]
set -uo pipefail
source "$(dirname "$0")/common.sh"
ui_args "$@" || exit 2
trap ui_stop EXIT
ui_start
img=$(shot 1_boot)
# the title bar of the main window is drawn: the region holding "CyberSample v.160" has plenty of dark pixels
expect "main window drawn (title bar has text)" gt "$(dark_pixels "$img" 440 208 590 224)" 300
menu 260 193; click 290 214 2                      # "CyberSample" menu > About CyberSample...
about=$(shot 2_about)
d=$(image_diff "$about" "$UI_ROOT/docs/screenshots/about-dialog.png")
expect "About dialog matches docs/screenshots/about-dialog.png ($d differing pixels)" lt "$d" 50
ui_finish $((EXPECT_FAILS > 0))
