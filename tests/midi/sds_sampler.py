#!/usr/bin/env python3
"""A virtual MIDI sampler speaking the MIDI Sample Dump Standard, for loopback-testing CyberSample's Get/Send.

It sits on the two FIFOs Hatari's --midi-in/--midi-out are pointed at:
  * when CyberSample sends a DUMP REQUEST (File > Get) it answers with the test sample, in the format given by --bits
    (header, then 120-byte packets, waiting for an ACK after each);
  * when CyberSample sends a DUMP HEADER (File > Send) it ACKs it, takes the packets (verifying each checksum,
    ACKing or NAKing) and writes what it received to --result as JSON.
Everything seen on the wire is appended to --log.  The sample served is deterministic (see test_samples()).
"""
import argparse, json, os, select, sys, time

def test_samples(n=1000):
    """16-bit signed test data: extremes, a sweep, and a pseudo-random tail - exercises every bit of the 3-byte format."""
    out = [-32768, 32767, 0, -1, 1, 255, -256, 4095, -4096]
    x = 12345
    while len(out) < n:
        x = (x * 1103515245 + 12345) & 0x7fffffff
        out.append(((x >> 8) & 0xffff) - 32768)
    return out[:n]

def bytes_per_word(bits): return (bits + 6) // 7

def encode_word(v16, bits):
    """16-bit signed -> SDS bytes: unsigned offset-binary, `bits` significant, left-justified in 7-bit groups."""
    nb = bytes_per_word(bits)
    u = (v16 + 32768) >> (16 - bits)                 # keep the top `bits` bits
    u <<= 7 * nb - bits                              # left-justify in the 7*nb-bit field
    return [(u >> (7 * (nb - 1 - i))) & 0x7f for i in range(nb)]

def decode_word(bs, bits):
    nb = bytes_per_word(bits)
    u = 0
    for b in bs: u = (u << 7) | b
    u >>= 7 * nb - bits
    return (u << (16 - bits)) - 32768                # back to 16-bit signed (low bits zero)

def checksum(msg): 
    c = 0
    for b in msg[1:-2]: c ^= b                       # bytes 1 .. (before checksum)
    return c

def le7(v): return [v & 0x7f, (v >> 7) & 0x7f, (v >> 14) & 0x7f]
def from7(l, m, h): return l | (m << 7) | (h << 14)

class Link:
    def __init__(self, rx, tx, log):
        self.rx = os.open(rx, os.O_RDWR | os.O_NONBLOCK); self.tx = os.open(tx, os.O_RDWR)
        self.buf = bytearray(); self.log = open(log, "a", buffering=1)
    def say(self, *a): self.log.write("%.3f %s\n" % (time.time(), " ".join(str(x) for x in a)))
    def send(self, msg, what):
        os.write(self.tx, bytes(msg)); self.say("TX", what, len(msg), "bytes")
    def next_msg(self, timeout):
        """Return one F0..F7 message from the Atari, or None on timeout."""
        end = time.time() + timeout
        while True:
            i = self.buf.find(b"\xf0")
            if i >= 0:
                j = self.buf.find(b"\xf7", i)
                if j >= 0:
                    m = list(self.buf[i:j + 1]); del self.buf[:j + 1]; return m
            left = end - time.time()
            if left <= 0: return None
            r, _, _ = select.select([self.rx], [], [], left)
            if r:
                try: self.buf += os.read(self.rx, 4096)
                except BlockingIOError: pass

def handshake(link, timeout=5):
    """Next ACK/NAK/CANCEL from the Atari, skipping WAIT (0x7c) messages, which it sends right after a request."""
    end = time.time() + timeout
    while time.time() < end:
        m = link.next_msg(end - time.time())
        if m is None: return None
        if len(m) == 6 and m[3] == 0x7c: link.say("RX wait"); continue
        return m
    return None

def ack(dev, pp): return [0xf0, 0x7e, dev, 0x7f, pp & 0x7f, 0xf7]
def nak(dev, pp): return [0xf0, 0x7e, dev, 0x7e, pp & 0x7f, 0xf7]

RT_SPLICE = None   # (byte, every): set by --rt-splice / --rt-every

def splice(msg):
    """Insert a real-time byte (F8 clock, FE active sensing...) every N bytes inside a sysex message, as a MIDI merger
    or thru box with a running clock does. The MIDI spec allows real-time bytes anywhere, including inside sysex,
    and a receiver must ignore them there."""
    if not RT_SPLICE: return msg
    b, n = RT_SPLICE; out = []
    for i, x in enumerate(msg):
        if 0 < i < len(msg) - 1 and i % n == 0: out.append(b)
        out.append(x)
    return out

def serve(link, dev, samples, bits, rate):
    """Atari asked for a dump: send header then packets, waiting for ACK after each."""
    nb = bytes_per_word(bits); spp = 120 // nb
    period = 1000000000 // rate
    hdr = [0xf0, 0x7e, dev, 0x01, 0, 0, bits] + le7(period) + le7(len(samples)) + le7(0) + le7(len(samples) - 1) + [0x00, 0xf7]
    link.send(splice(hdr), "header bits=%d len=%d" % (bits, len(samples)))
    m = handshake(link)
    link.say("RX", "after header:", m)
    if not m or m[3] != 0x7f: link.say("FAIL no ACK for header"); return False
    for pp in range((len(samples) + spp - 1) // spp):
        words = samples[pp * spp:(pp + 1) * spp]
        words += [0] * (spp - len(words))
        data = []
        for w in words: data += encode_word(w, bits)
        data += [0] * (120 - len(data))
        pkt = [0xf0, 0x7e, dev, 0x02, pp & 0x7f] + data + [0, 0xf7]
        pkt[-2] = checksum(pkt)
        for attempt in range(5):
            link.send(splice(pkt), "packet %d" % pp)
            m = handshake(link)
            if m and m[3] == 0x7f: break
            link.say("RX", "packet", pp, "got", m)
        else:
            link.say("FAIL packet", pp, "never ACKed"); return False
    link.say("serve complete"); return True

def receive(link, hdr, result_path):
    """Atari is sending a dump: ACK the header, collect packets, write result JSON."""
    dev = hdr[2]; bits = hdr[6]
    period = from7(*hdr[7:10]); length = from7(*hdr[10:13])
    lstart = from7(*hdr[13:16]); lend = from7(*hdr[16:19]); lmode = hdr[19]
    nb = bytes_per_word(bits); spp = 120 // nb
    link.say("RX header: format=%d period=%d length=%d loop=%d..%d mode=0x%02x" % (bits, period, length, lstart, lend, lmode))
    link.send(ack(dev, 0), "ack header")
    got, bad, expect = [], 0, (length + spp - 1) // spp
    pp = 0
    while pp < expect:
        m = link.next_msg(5)
        if m is None: link.say("FAIL timeout waiting for packet", pp); break
        if len(m) != 127 or m[3] != 0x02: link.say("RX unexpected", len(m), m[:6]); continue
        if checksum(m) != m[125]: bad += 1; link.say("RX bad checksum packet", m[4]); link.send(nak(dev, m[4]), "nak"); continue
        if m[4] != (pp & 0x7f): link.say("RX packet number", m[4], "expected", pp & 0x7f)
        data = m[5:125]
        for i in range(spp): got.append(decode_word(data[i * nb:(i + 1) * nb], bits))
        link.send(ack(dev, m[4]), "ack %d" % pp); pp += 1
    extra = 0                                        # a well-behaved sender stops after ceil(length/spp) packets
    while True:
        m = link.next_msg(2.0)
        if m is None: break
        if len(m) == 127 and m[3] == 0x02:
            extra += 1; link.say("RX EXTRA packet", m[4]); link.send(ack(dev, m[4]), "ack extra")
    got = got[:length]
    json.dump(dict(format=bits, period=period, length=length, loop_start=lstart, loop_end=lend, loop_mode=lmode,
                   packets=pp, expected_packets=expect, extra_packets=extra, bad_checksums=bad, samples=got), open(result_path, "w"))
    link.say("receive complete", pp, "packets")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--from-atari", required=True); ap.add_argument("--to-atari", required=True)
    ap.add_argument("--bits", type=int, default=16, help="SDS format to serve on Get")
    ap.add_argument("--rate", type=int, default=44100); ap.add_argument("--count", type=int, default=1000)
    ap.add_argument("--log", required=True); ap.add_argument("--result", required=True)
    ap.add_argument("--run-for", type=float, default=120)
    ap.add_argument("--rt-splice", help="hex real-time byte (e.g. f8) to insert inside every served sysex message")
    ap.add_argument("--rt-every", type=int, default=40, help="insert it every N bytes (40 ~ a 120 BPM clock at 31250 baud)")
    a = ap.parse_args()
    global RT_SPLICE
    if a.rt_splice: RT_SPLICE = (int(a.rt_splice, 16), max(1, a.rt_every))
    link = Link(a.from_atari, a.to_atari, a.log); samples = test_samples(a.count); link.say("sampler up")
    end = time.time() + a.run_for
    while time.time() < end:
        m = link.next_msg(1)
        if not m: continue
        link.say("RX", len(m), "bytes", m[:8])
        if len(m) >= 5 and m[1] == 0x7e and m[3] == 0x03: serve(link, m[2], samples, a.bits, a.rate)
        elif len(m) == 21 and m[3] == 0x01: receive(link, m, a.result)
    link.say("sampler done")

if __name__ == "__main__": main()
