#!/usr/bin/env python3
"""Check what the virtual sampler received from File > Send against the sample it served on File > Get."""
import argparse, json, sys
sys.path.insert(0, __import__("os").path.dirname(__file__))
import sds_sampler as s

ap = argparse.ArgumentParser(); ap.add_argument("result")
ap.add_argument("--serve-bits", type=int, required=True); ap.add_argument("--send-format", type=int, required=True)
ap.add_argument("--trunc", type=int, default=1); ap.add_argument("--count", type=int, default=1000)
a = ap.parse_args()
r = json.load(open(a.result)); served = s.test_samples(a.count)
fails = []
def check(ok, msg):
    print(("ok   " if ok else "FAIL ") + msg)
    if not ok: fails.append(msg)

bits = r["format"]
check(bits == a.send_format, "header format is %d (Send dialog asked for %d)" % (bits, a.send_format))
# what the Atari held after Get is the served sample at the served precision; Send can't add precision
held = [(v >> (16 - a.serve_bits)) << (16 - a.serve_bits) for v in served]
want = [(v >> (16 - bits)) << (16 - bits) for v in held]
# "truncate at loop end" sends Loop1End words (the served loop end is count-1)
n = (a.count - 1) if a.trunc else a.count
check(r["length"] == n, "header length %d == words sent %d" % (r["length"], n))
check(r["bad_checksums"] == 0, "all packet checksums valid")
check(r["extra_packets"] == 0, "no extra packet beyond ceil(length/words-per-packet) (%d extra)" % r["extra_packets"])
check(r["packets"] == r["expected_packets"], "packet count %d == ceil(length/words-per-packet) %d" % (r["packets"], r["expected_packets"]))
got = r["samples"]
check(len(got) >= n, "received %d samples" % len(got))
bad = [i for i in range(min(len(got), n)) if got[i] != want[i]]
check(not bad, "samples identical to the served ones at %d-bit precision (%d mismatches%s)" % (bits, len(bad), (", first at %d" % bad[0]) if bad else ""))
print("PASS" if not fails else "FAILED (%d)" % len(fails)); sys.exit(1 if fails else 0)
