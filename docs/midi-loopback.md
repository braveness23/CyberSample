# MIDI loopback test

`tests/midi/` tests File > Get and File > Send **in the real program, in an emulator, with no MIDI hardware**.

```
CYBR160.PRG  --MIDI out-->  FIFO  -->  sds_sampler.py   (a virtual SDS sampler)
(in Hatari)  <--MIDI in---  FIFO  <--
```

Hatari's `--midi-out` / `--midi-in` options are pointed at two named pipes. `sds_sampler.py` sits on the other end and speaks the
MIDI Sample Dump Standard:

* On a **dump request** (File > Get) it serves a deterministic 1000-word test sample (extremes, a sweep and a pseudo-random tail, so
  every bit of the 12- and 16-bit encodings is exercised) as a header plus 120-byte packets, waiting for the program's ACK after each.
* On a **dump header** (File > Send) it ACKs, collects the packets (checking each XOR checksum and packet number), and writes
  what it received - format, length, loop points, packet counts, decoded samples - to a JSON file.

`run_loopback.sh` drives the GUI with `xdotool` (Ctrl-G, then File > Send... and the dialog's checkbox/radio/OK), and
`check_result.py` compares the result with what was served: header format, announced length vs words sent, checksums, no extra packet,
and every sample identical at the chosen precision. So one run is a full round trip: sampler -> Get (decode) -> memory -> Send (encode) -> sampler.

```
tests/midi/run_loopback.sh --prg out/CYBR160.PRG --serve-bits 16 --send-format 16 --trunc 0
tests/midi/run_matrix.sh   --prg out/CYBR160.PRG          # 6 combinations of served format / send format / truncate, ~8 min
```

Needs `hatari`, `Xvfb`, `xdotool`, ImageMagick's `import`, `python3` and a TOS/EmuTOS image (`ci/build.sh` downloads EmuTOS 1.3
into `ci/.cache`). It is not part of the GitHub workflow because it needs a built `.PRG`; the screenshots of each step are kept in the
work directory if a run fails (`--keep`, `--workdir`). The GUI click coordinates are fixed positions on the 640x400 mono screen.

It found these bugs: #27 (Send ignored the format choice), #30 (one packet too many when the length is a multiple of the packet size)
and #31 (header announced more words than were sent with "truncate at loop end").

## Real-time bytes inside the dump (`--rt-splice`)

On a real MIDI network the line often carries real-time bytes while a dump is running: a sequencer's clock (`F8`, every
20.8 ms at 120 BPM) through a merger or thru box, or active sensing (`FE`). The MIDI spec allows them **anywhere, including
inside a sysex message**, and receivers must ignore them there. A 127-byte SDS packet takes ~40 ms at 31250 baud, so with a
clock running one or two `F8` land inside every packet.

`--rt-splice f8` (or `fe`) makes the virtual sampler insert that byte every `--rt-every` bytes (default 40) inside each
message it serves, so this is tested without any other equipment:

```
tests/midi/run_loopback.sh --prg out/CYBR160.PRG --serve-bits 16 --send-format 16 --trunc 0 --rt-splice f8
```

Result on v1.60 (2026-11-22): **FAIL** - the header (21 bytes, nothing spliced at the default spacing) is ACKed, then every
transmission of packet 0 is NAKed (`F0 7E 00 7E 00 F7`) until the sampler gives up; the same with `fe`. Without
`--rt-splice` the same run passes. `GET.C` reads packets at fixed offsets in the raw MIDI input buffer and counts bytes
with no real-time filtering, so any spliced byte shifts the packet and breaks the checksum. The same failure was seen with a
live sequencer's clock merged in (not just this synthetic splice).
