#!/usr/bin/env python3
"""Generate the BUILD.CMD (for RUNCMD.TOS) and the CLink WITH file from a Lattice C .PRJ project file.

usage: gen_buildcmd.py <project.PRJ> <out-dir>
Writes <out-dir>/BUILD.CMD and <out-dir>/LINK.LNK (both CRLF).  Paths assume the layout
    C:\\LC\\   Lattice C install     C:\\SRC\\   the project sources
"""
import os, re, sys

prj, out = sys.argv[1], sys.argv[2]
lines = [l.strip() for l in open(prj, "rb").read().decode("latin1").splitlines() if l.strip()]
target = os.path.basename(lines[0].replace("\\", "/"))          # e.g. CYBR160.PRG
opts = {}
files = []
seen_eq = False
for l in lines[1:]:
    m = re.match(r"^\.(\w+)\s*\[(.*)\]\s*$", l)
    if m and not seen_eq:
        opts[m.group(1).upper()] = m.group(2).strip()
    elif l == "=":
        seen_eq = True
    elif seen_eq:
        files.append(l)

fix = lambda o: re.sub(r"-i\.\.\\LC\\HEADERS\\", r"-iC:\\LC\\HEADERS\\", o)
startup = [f for f in files if f.upper() == "C.O"]
libs = [f for f in files if f.upper().endswith(".LIB")]
srcs = [f for f in files if f not in startup and f not in libs]

cmd = [r"set PATH=C:\LC\BIN", r"set INCLUDE=C:\LC\H\ ".strip(), r"set LIB=C:\LC\LIB\ ".strip()]
objs = []
for s in srcs:
    stem, ext = os.path.splitext(s)
    full = "C:\\SRC\\" + s
    if ext.upper() == ".C":
        cmd.append(r"C:\LC\BIN\LCC.TTP -c %s %s" % (fix(opts.get("C", "")), full))
    elif ext.upper() == ".S":
        cmd.append(r"C:\LC\BIN\ASM.TTP %s %s" % (opts.get("S", ""), full))
    else:
        raise SystemExit("unsupported source type: " + s)
    objs.append("C:\\SRC\\" + stem + ".O")
cmd.append(r"C:\LC\BIN\CLINK.TTP WITH C:\SRC\LINK.LNK")

link = "FROM " + " ".join(startup[:1] + objs) + "\nLIB " + " ".join(libs) + "\nTO C:\\SRC\\" + target + "\n"
os.makedirs(out, exist_ok=True)
open(os.path.join(out, "BUILD.CMD"), "wb").write(("\r\n".join(cmd) + "\r\n").encode("latin1"))
open(os.path.join(out, "LINK.LNK"), "wb").write(link.replace("\n", "\r\n").encode("latin1"))
print(target)
