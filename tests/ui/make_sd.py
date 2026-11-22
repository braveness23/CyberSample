#!/usr/bin/env python3
"""Write a CyberSample .SD file containing a sine wave:  make_sd.py OUT.SD [amplitude=8000] [hz=440] [seconds=1]

The .SD format is a fixed 1336-byte header followed by big-endian signed 16-bit samples; offsets are those documented
in src/DEFS.H.  Used to give GUI tests a known sample without driving the file selector."""
import math, struct, sys
out = sys.argv[1]; amp = int(sys.argv[2]) if len(sys.argv) > 2 else 8000
hz = float(sys.argv[3]) if len(sys.argv) > 3 else 440.0; secs = float(sys.argv[4]) if len(sys.argv) > 4 else 1.0
rate = 44100; n = int(rate * secs); h = bytearray(1336)
def w16(o, v): h[o:o + 2] = struct.pack(">h", v)
def w32(o, v): h[o:o + 4] = struct.pack(">l", v)
w16(0, 1336); w16(2, 11); w16(56, -256); w32(58, 1); w32(62, 327); h[66] = 4; h[68:74] = b"%Scale"; w32(102, 1)
w16(172, 15); w16(174, 15); w32(184, n * 2)                       # SampLength is in BYTES in the file
w32(752, -1); w32(756, -1)                                         # no loop
w32(1020, rate); w32(1024, 1000000000 // rate); w16(1028, 16); h[1030:1036] = b"Linear"
w32(1324, -1); w32(1328, -1); h[1332] = 1; h[1333] = 1
data = b"".join(struct.pack(">h", int(amp * math.sin(2 * math.pi * hz * i / rate))) for i in range(n))
open(out, "wb").write(bytes(h) + data)
