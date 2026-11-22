# Porting notes: 1993 Lattice C → Lattice C 5.60

The original `CYBR160.PRJ` and sources were written against an older Lattice C (5.0x era). HiSoft's **Lattice C 5.60**
(8 Nov 1993) needed the following. Every change is in [`../patches/`](../patches/).

## Compatibility shims
1. **`ELTX(r)` / `PTRX(r)`** — old AES-binding macros expanding to pointers to a GRECT's four fields, used as
   `wind_get(h, WF_x, ELTX(&r))`. 5.60's `AES.H` only has `PTRS`/`ELTS` (by value). Added at the end of `DEFS.H`
   (`PTRX` goes through one extra `*` because callers pass `&ptr`).
2. **`TED_PTEXT(tree,obj)`** — old accessor macro for a TEDINFO object's text pointer; missing in 5.60. It compiled as an
   implicit function and only surfaced as an *undefined symbol at link time*. Defined in `DEFS.H`.
3. **`MENU` name clash** — 5.60's `AES.H` has `typedef ... MENU;` (for `menu_popup`); the WERCS resource tree was also named
   `MENU`. Renamed `CYBMENU` in `CYBR.C`, `CYBRWERK.C`, `CYBRWERK.H`. A project-wide `-dMENU=CYBMENU` does **not** work: it also
   renames the typedef inside `AES.H` and just moves the clash.

## A real bug that existed in 1993
`CYBR.C` called `Dgetpath(start_path, Dgetdrv())`. `Dgetdrv()` is 0-based (A=0, C=2) but `Dgetpath`'s drive argument is
**1-based, 0 = default drive**. On a floppy the numbers happen to line up; on a hard drive it asks for drive B and returns an
empty string. The file selector then started at `\*.*` with no drive letter, `hndl_fsel` called `Dsetdrv(path[0]-'A')` on a
backslash, and every open outside the program's own folder failed. Fixed by building a drive-qualified path.

## Build-system gotchas
- The IDE's **Make does not track header dependencies**: after editing `DEFS.H` it reports "up to date". Use **Make all**.
- The generated prototype files (`*.I`) are required (`DEFS.H` includes them). Don't delete them to force a rebuild.
- `v_opnvwk(work_in, &handle, work_out)` takes the handle **by pointer** in the VDI binding.
- The project expects Lattice installed as a sibling folder named `LC` (`-i..\LC\HEADERS\`).
- Lattice reads `PATH`, `INCLUDE` and `LIB` from the environment (the IDE keeps them under *Options > Environment*).
