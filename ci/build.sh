#!/usr/bin/env bash
# Build CyberSample headlessly: Hatari (+EmuTOS) running Lattice C 5.60's command-line tools.
#
#   ci/build.sh --lattice /path/to/LatticeC [--tos tos.img] [--project src/CYBR160.PRJ] [--out out] [--slow]
#
# --lattice  directory containing the Lattice C 5.60 install (BIN, H, HEADERS, LIB). NOT included in this repo.
# --tos      TOS/EmuTOS image. Default: downloaded EmuTOS 1.3 (GPL) into ci/.cache
# --slow     disable Hatari's fast-forward (real ST speed; ~10x slower) - only for benchmarking
# Needs: hatari, xvfb-run, python3, curl, unzip.
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd); ROOT=$(dirname "$HERE")
LATTICE=; TOS=; PROJECT=$ROOT/src/CYBR160.PRJ; OUT=$ROOT/out; FAST=true; TIMEOUT=900
while [ $# -gt 0 ]; do case $1 in
  --lattice) LATTICE=$2; shift 2;; --tos) TOS=$2; shift 2;; --project) PROJECT=$2; shift 2;;
  --out) OUT=$2; shift 2;; --slow) FAST=false; shift;; --timeout) TIMEOUT=$2; shift 2;;
  *) echo "unknown option $1" >&2; exit 2;; esac; done
[ -d "$LATTICE/BIN" ] && [ -d "$LATTICE/LIB" ] || { echo "--lattice must point at a Lattice C install (BIN, LIB, H, HEADERS)" >&2; exit 2; }

if [ -z "$TOS" ]; then
  TOS=$HERE/.cache/emutos-256k-1.3/etos256us.img
  if [ ! -f "$TOS" ]; then
    mkdir -p "$HERE/.cache"; echo "fetching EmuTOS 1.3 ..."
    curl -fsSL --retry 4 --retry-delay 5 --connect-timeout 20 -o "$HERE/.cache/emutos.zip" \
      https://downloads.sourceforge.net/project/emutos/emutos/1.3/emutos-256k-1.3.zip \
      || { echo "EmuTOS download failed - pass an existing image with --tos" >&2; exit 2; }
    unzip -q -o "$HERE/.cache/emutos.zip" -d "$HERE/.cache"
  fi
fi

WORK=$(mktemp -d); trap 'rm -rf "$WORK"' EXIT
HD=$WORK/C; mkdir -p "$HD/RUNCMD" "$HD/SRC" "$OUT"
cp -r "$LATTICE" "$HD/LC"
cp "$ROOT"/src/* "$HD/SRC/"
cp "$HERE/runcmd/RUNCMD.TOS" "$HD/RUNCMD/"
find "$HD/SRC" -iname '*.O' -o -iname '*.PRG' | xargs -r rm -f
TARGET=$(python3 "$HERE/gen_buildcmd.py" "$PROJECT" "$WORK/gen"); cp "$WORK/gen/BUILD.CMD" "$HD/BUILD.CMD"; cp "$WORK/gen/LINK.LNK" "$HD/SRC/LINK.LNK"

export SDL_AUDIODRIVER=dummy
FF=(--fast-forward "$FAST" --timer-d "$FAST" --fast-boot "$FAST")
START=$(date +%s)
setsid xvfb-run -a -s "-screen 0 1024x768x24" hatari --monitor mono --machine st --tos "$TOS" --harddrive "$HD" \
  --auto 'C:\RUNCMD\RUNCMD.TOS' --conout 2 "${FF[@]}" --sound off --statusbar off --confirm-quit off --window \
  > "$WORK/hatari.log" 2>&1 &
HP=$!
while [ ! -f "$HD/DONE.TXT" ] && [ $(( $(date +%s) - START )) -lt "$TIMEOUT" ]; do sleep 0.5; done
ELAPSED=$(( $(date +%s) - START ))
# Hatari runs in its own session/process group (setsid): kill the whole group so nothing is left spinning
kill -- -"$HP" 2>/dev/null || true; sleep 1; kill -9 -- -"$HP" 2>/dev/null || true

# clean build log: keep tool output, drop emulator chatter
grep -a -v -E '^(INFO|WARN) ?:' "$WORK/hatari.log" | tr -d '\000' > "$OUT/build.log" || true
STATUS=$(cat "$HD/DONE.TXT" 2>/dev/null || echo TIMEOUT)
cat "$OUT/build.log"
echo "---- build finished in ${ELAPSED}s, runner status: ${STATUS}"
if [ "$STATUS" != "OK" ]; then echo "BUILD FAILED"; exit 1; fi
PRG=$(find "$HD/SRC" -iname "$TARGET" | head -1)
[ -n "$PRG" ] && [ -s "$PRG" ] || { echo "no output file $TARGET"; exit 1; }
cp "$PRG" "$OUT/$TARGET"
echo "OK: $OUT/$TARGET ($(stat -c %s "$OUT/$TARGET") bytes, sha256 $(sha256sum "$OUT/$TARGET" | cut -d' ' -f1))"
