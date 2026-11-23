# Testing CyberSample

How this project is tested, **why each technique exists, and the gotchas learned the hard way**. If you add a test technique,
write it down here.

## Ground rules

1. **Every fix gets a test that fails on the old code and passes on the new.** Prove the first half: run the test against the
   previous revision (`CCP_SRC=old/CCP.C tests/run.sh`, or build the old commit) and watch it fail.
2. **Measure, don't eyeball.** Screenshots are compared by pixel counts or against a reference image, never "looks right".
3. **Try to break your own checks.** Example: the `sizeof(SD_HEADER) == 1336` compile-time assertion was verified by changing it to
   1337 and confirming the build fails with "invalid array limit expression".
4. **State what was and wasn't exercised** when reporting a fix (e.g. "built and started, dialog path not run").

## The layers

| Layer | Command | Needs | Time | Covers |
|---|---|---|---|---|
| Host unit tests | `tests/run.sh` | gcc, python3 | seconds | DSP (`clip16`, one-pole filter, gate) and Cut/Copy with the slot helpers, under ASan/UBSan/LSan. **Runs in CI on every push.** |
| Build | `ci/build.sh --lattice DIR` | hatari, Xvfb, Lattice C | ~25 s | Compiles and links with the real Lattice C 5.60 (inside Hatari). See `docs/ci.md`. |
| Emulator smoke | `tests/ui/smoke.sh` | + xdotool, ImageMagick, python3-pil | ~40 s | Boots, window drawn, About dialog pixel-identical to `docs/screenshots/about-dialog.png`. |
| Emulator undo | `tests/ui/undo_test.sh` | same | ~60 s | Per-slot Undo/Redo, checked pixel-for-pixel. |
| MIDI loopback | `tests/midi/run_matrix.sh` | same | ~8 min | File > Get / Send against a virtual SDS sampler: header, lengths, packets, checksums, bit-exact samples. See `docs/midi-loopback.md`. |
| MIDI real-time in a dump | `tests/midi/run_loopback.sh … --rt-splice f8` (and `fe`) | same | ~1 min each | Get/Send with clock/active-sensing bytes inside every served packet (as a merger with a running clock delivers them). |
| MIDI thru stress | `tests/midi/run_thru_stress.sh` | same | ~75 s | Clock (48 B/s) into MIDI in with Midi Thru on while File > Get is opened and cancelled 6×: no crash, thru echoes every clock outside the dialog and resumes after each Cancel. |

`tests/ui/run_all.sh --prg out/CYBR160.PRG` runs all the emulator tests.

Which test covers which issue:

| Issue | Test |
|---|---|
| #11 filters, #12 gate, #13 clipping | `tests/run.sh` (`dsp_test`) |
| #29 Cut/Copy on the paste buffer | `tests/run.sh` (`ccp_test`) |
| #14 per-slot Undo | `tests/ui/undo_test.sh` |
| #7 SDS decode, #27 Send format, #30 packet count, #31 announced length | `tests/midi/run_matrix.sh` |
| #33 real-time bytes inside an SDS dump | `tests/midi/run_loopback.sh --rt-splice f8` / `fe` |
| #24 MIDI thru from interrupt context | `tests/midi/run_thru_stress.sh` |
| #25 `sizeof(SD_HEADER)` check | negative build test (ground rule 3) |
| About dialog / start-up | `tests/ui/smoke.sh` |

## Technique: host-side tests on code extracted from the real sources

`tests/run.sh` does **not** copy the code under test. A small python step cuts the functions out of `src/EDIT.C`, `src/CCP.C`
and `src/MISC.C` (between `/* DSP-BEGIN */` ... `/* DSP-END */`, or from a function's `type\nname(` line to its closing `}`) into
a generated header, and the test `#include`s it, so what's tested is the text the Atari build compiles. Everything GEM-specific is
replaced by a stand-in at the top of the test file (`tests/ccp_test.c` has a reduced `SD_HEADER`, `form_alert` that counts calls, ...).
That works because the code is C89 and the DSP code is deliberately kept free of GEM calls (so *keep pure code out of GEM handlers*).

Gotchas:
* Run under `-fsanitize=address,undefined`. Leak detection is **on** for `ccp_test` because the allocation-failure paths must not leak.
* LeakSanitizer exits via `_exit` **without flushing stdio**, so with output piped the test's `FAIL` lines vanish. `ccp_test.c` calls
  `setvbuf(stdout, NULL, _IONBF, 0)`.
* Inject allocation failures by `#define malloc test_malloc` before including the extracted code (fail the Nth call).
* `CCP_SRC=<file> tests/run.sh` runs the Cut/Copy test against another revision of `CCP.C`, e.g. the one before a fix:
  `git show HEAD~1:src/CCP.C > /tmp/old.C`.
* Files are CRLF; the extractor normalises line endings.

## Technique: driving the real program in headless Hatari

`tests/ui/common.sh` is the toolbox (source it; read its comments). The ingredients:

* **Hatari on Xvfb**: `Xvfb :N -screen 0 1024x768x24`, then `hatari --monitor mono --machine st --tos EMUTOS --harddrive DIR --auto 'C:\CYBR160.PRG' --window --statusbar off`.
  The Atari mono screen is 640x400 at offset **(192,184)** inside the Xvfb, so Atari (x,y) = Xvfb (x+192, y+184).
  `--harddrive DIR` maps a host directory as drive C:, so putting the `.PRG` (and test files) in it is all the "install" needed.
  `--fast-forward true --fast-boot true` is fine for functional tests (boot to program in ~22 s). Do **not** use fast-forward for timing
  tests (see #23).
* **EmuTOS** (`etos256us.img`, downloaded by `ci/build.sh`) is used instead of a real TOS ROM; builds are byte-identical.
* **Input**: `xdotool key ...` / `mousemove` / `mousedown` / `mouseup`. Hatari receives them as ST key/mouse events.
  * GEM **menus open on hover**: move onto the title, wait, move 2 px, wait (`menu()` in `common.sh`). Items are then ordinary clicks.
  * The ST **Undo key = Scroll Lock** (`xdotool key Scroll_Lock`) - from Hatari's manual. (PageUp/PageDown do nothing.)
  * Shortcuts the program defines: Ctrl-A select all, Ctrl-G Get, N normalize, R reverse, 1-8 sample slot (list in `src/DEFS.H`).
  * In a dialog **Return presses the default button**.
* **Getting a known sample in without the file selector**: `tests/ui/make_sd.py` writes a `.SD` file (1336-byte header + big-endian
  16-bit samples). Hatari's `--auto` can't pass arguments, so it is launched through `ci/runcmd/RUNCMD.TOS`, which `Pexec`s the line
  in `C:\BUILD.CMD` (`C:\CYBR160.PRG C:\T.SD`) - the program opens `argv[1]` at start. **Limit: under RUNCMD the pull-down menus do
  not open.** So tests that need menus (About, File > Send, ...) start the program directly and get their data another way (MIDI Get);
  tests that start from a file use keyboard shortcuts only.
* **Screenshots**: `import -window root x.png`. Two measures, both in `common.sh`:
  `dark_pixels IMG x0 y0 x1 y1` (how much waveform is drawn - the zoom-view box is x 232-790, y 372-482) and `image_diff A B`
  (0 = identical). Hatari's mono output is fully deterministic: the undo test's reference numbers are 6127 dark pixels before and
  22122 after Normalize, every run.
* **Coordinates** (Xvfb coordinates, direct launch): menu titles at y=193: CyberSample x=260, File 335, Edit 383, Sample 551, Loop 607;
  About item (290,214); File > Get (340,387), File > Send (340,403). Get dialog OK (443,488). Send dialog: "truncate at loop end"
  checkbox (344,402), Format 12-bit (430,433), 16-bit (519,433), OK (440,497).
  To find a new coordinate: take a screenshot of the dialog and read it off - never guess.

### Process hygiene (this has bitten us)
* Start Xvfb + Hatari + helpers in **one `setsid` process group** and stop them with `kill -- -PGID` (`ui_start`/`ui_stop`).
* **Never `pkill -f`/`pgrep -f` by name** from a command that itself contains that name - over ssh it matches its own shell and kills your
  session. If you must look, use the bracket trick: `ps -eo pid,args | grep "[h]atari"`.
* Stopping a backgrounded test runner does **not** stop the emulator it launched. Check afterwards: no `hatari`/`Xvfb` left, no temp dirs.
* Always run emulator work on an otherwise idle machine; orphaned emulators skew timings (an earlier "build takes 100 s" was really
  an overloaded box; the idle figure is ~22 s).
* `ci/build.sh` downloads EmuTOS from SourceForge; a transient DNS failure shows as `EmuTOS download failed`. Keep a copy and pass `--tos`.

## Technique: MIDI loopback
See `docs/midi-loopback.md`. In short: Hatari's `--midi-out`/`--midi-in` go to two FIFOs; `tests/midi/sds_sampler.py` is a virtual
MIDI Sample Dump Standard sampler on the other side. Details worth remembering: open the FIFOs `O_RDWR` so neither side blocks; the
program sends a **WAIT (0x7C)** message right after its dump request - a sampler must skip it when waiting for an ACK; the test
sample contains the extreme values and an exact multiple of the packet size on purpose; compare at the *chosen* precision (a 12-bit
send cannot return 16-bit data).

## Adding a test - recipe
1. Pure logic (no GEM)? Put it in a function with no GEM calls, extract it as above, add cases to a host test. Run it against the old
   code first.
2. Needs the real program? Add `tests/ui/<name>.sh` using `ui_args`/`ui_start`/`key`/`shot`/`dark_pixels`/`expect`/`ui_finish`; add it to
   `run_all.sh`. Take screenshots at every step (kept on failure) so a failing run can be diagnosed without re-running.
3. Needs MIDI? Extend `sds_sampler.py` / `check_result.py` and add a combination to `run_matrix.sh`.
4. Update this file and the table above.

## Not covered yet
Real MIDI hardware; real TOS (the emulator tests use EmuTOS); timing behaviour (#23 loop-count timeouts depend on CPU speed);
the emulator tests in GitHub Actions (they need the commercial Lattice C for the build - the host tests already run there);
dialog-driven edits (filters, gate, compressor) in the emulator; Cut/Copy on the paste-buffer slot in the emulator (menus needed).
