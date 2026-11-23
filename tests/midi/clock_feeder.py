#!/usr/bin/env python3
"""A sequencer's MIDI clock on the ST's MIDI in, for the MIDI-thru stress test (issue #24).

Writes F8 at --bpm (24 per beat) into --to-atari for --seconds and logs, with time.time() stamps, every
clock sent ("TX") and every byte the ST sends back on --from-atari ("RX <hex>": the Midi Thru echo)."""
import argparse, os, select, time

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--from-atari", required=True); ap.add_argument("--to-atari", required=True)
    ap.add_argument("--bpm", type=float, default=120); ap.add_argument("--seconds", type=float, default=60)
    ap.add_argument("--log", required=True)
    a = ap.parse_args()
    rx = os.open(a.from_atari, os.O_RDWR | os.O_NONBLOCK); tx = os.open(a.to_atari, os.O_RDWR)
    log = open(a.log, "a", buffering=1)
    period = 60.0 / (a.bpm * 24); t0 = time.time(); k = 0
    while time.time() - t0 < a.seconds:
        due = t0 + k * period
        while True:
            left = due - time.time()
            r, _, _ = select.select([rx], [], [], max(0.0, left))
            if r:
                try:
                    for b in os.read(rx, 4096): log.write("%.4f RX %02x\n" % (time.time(), b))
                except BlockingIOError: pass
            if time.time() >= due: break
        os.write(tx, b"\xf8"); log.write("%.4f TX f8\n" % time.time()); k += 1

if __name__ == "__main__":
    main()
