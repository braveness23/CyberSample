# Headless / CI build

`ci/build.sh` builds the whole program with no GUI: **Hatari** (an Atari ST emulator) boots **EmuTOS** and runs Lattice C 5.60's own
command-line tools. About **22 seconds** for a full build on a modern machine.

## How it works
1. Hatari's `--auto` can start one program but cannot pass it arguments. So `ci/runcmd/RUNCMD.TOS` (source: `RUNCMD.C`, a ~40-line TOS
   program) is the one program: it reads `C:\BUILD.CMD` and `Pexec()`s each line. `set NAME=value` lines build the environment
   (Lattice's driver finds its compiler passes through `PATH`, and uses `INCLUDE`/`LIB`).
2. `ci/gen_buildcmd.py` turns the project's `CYBR160.PRJ` into that command file: one `LCC.TTP -c <options> file.C` per C file,
   `ASM.TTP` for the assembler file, and a `CLINK.TTP WITH LINK.LNK` link step.
3. Hatari runs with `--conout 2`, which prints the Atari console on the host. Compiler messages and every tool's exit code
   (`RUNCMD< exit N`) land in `out/build.log`. RUNCMD finally writes `C:\DONE.TXT` (`OK` / `FAIL`); the host waits for it, then
   kills the emulator and checks that the `.PRG` exists.

`RUNCMD.TOS` is committed as a prebuilt binary (it is this repo's own code); rebuild it from `RUNCMD.C` / `RUNCMD.PRJ` in the Lattice IDE
if you change it.

## Speed
Measured on an otherwise idle 4-core x86 box, same inputs, byte-identical output (sha256 `93a75206…eb2f3`):

| Mode | Time |
|---|---|
| Real ST speed (`--slow`) | 274 s |
| Fast-forward (default: `--fast-forward true --timer-d true --fast-boot true --sound off`) | 22 s (~12×) |
| For reference: driving the Lattice IDE by hand, at real speed | ~8–10 min |

Fast-forward is fine for compile/link because nothing there depends on timing. Don't use it for audio, MIDI-dump or playback tests.
Falcon/TT mode (`--cpuclock 32`) does **not** work with the EmuTOS 256k image used here.

## What's not in this repo
**Lattice C is commercial software and is not included.** Supply your own install with `--lattice DIR` (the folder containing `BIN`, `H`,
`HEADERS`, `LIB`). The Atari ROM is not needed: EmuTOS (GPL) is downloaded, or pass your own with `--tos`.

## GitHub Actions
`.github/workflows/build.yml` runs a privacy guardrail (`ci/scrub-check.sh`) always, and the real build only when the repository secrets
`LATTICE_URL` (+ `LATTICE_SHA256`) point at a tarball of your Lattice install. Forks and fork PRs have no secrets, so the build is
skipped there. **This workflow has not been run on GitHub's runners yet** (it was developed against a local runner); the Hatari in
Ubuntu's package may be an older version than the one used here (2.6.1).

## Pitfalls found
- Run Hatari in its own process group (`setsid`) and kill the *group*; killing only `xvfb-run` leaves Hatari spinning at 100% CPU.
- `CLINK.TTP` with no arguments (or a bad one) **prompts for input** and hangs the batch; always pass `WITH file`.
- Lattice's driver prints "Can't execute lc1" if `PATH` isn't in the environment.
