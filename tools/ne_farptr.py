#!/usr/bin/env python3
"""Read-only NE far-pointer resolver for original/BLIZZARD.EXE (Step 14).

BLIZZARD.EXE's segments are moveable and every inter-/intra-segment far
pointer in code segments is relocated by a *non-additive selector fixup
chain* (NE relocation source type 2, internal-reference target).  In the raw
file the selector word of each such pointer holds the offset of the next
chain link, not a selector.  Byte scans for "offset + 0x10xx" therefore miss
every table cell and `push seg`/`mov word [x+2], seg` immediate.  This tool
walks the chains so each selector word can be attributed to its target
segment.

Commands:
  cells SEL:OFF [N]      decode N (default 32) 4-byte far-pointer cells
  refs SEL:OFF           far-pointer cells / immediates whose target is SEL:OFF
  install SEL:OFF        `mov word [..], OFF` immediates paired with a selector
                         fixup for SEL within 12 bytes (table installation)

The executable is opened read-only and never modified.
"""
import hashlib
import struct
import sys
from pathlib import Path

EXE = Path(__file__).resolve().parent.parent / "original" / "BLIZZARD.EXE"
SHA256 = "ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2"

data = EXE.read_bytes()
if hashlib.sha256(data).hexdigest() != SHA256:
    sys.exit(f"unexpected input hash for {EXE}")

ne = struct.unpack_from("<H", data, 0x3C)[0]
nseg = struct.unpack_from("<H", data, ne + 0x1C)[0]
segtab = ne + struct.unpack_from("<H", data, ne + 0x22)[0]
align = struct.unpack_from("<H", data, ne + 0x32)[0]

# segment number (1-based) -> (file offset, length, flags); Ghidra selector = 0x1000 + 8 * (n - 1)
segs = {}
for i in range(nseg):
    off, ln, fl, _ = struct.unpack_from("<HHHH", data, segtab + i * 8)
    if off:
        segs[i + 1] = (off << align, ln or 0x10000, fl)


def sel(n):
    return 0x1000 + (n - 1) * 8


def segnum(selector):
    return (selector - 0x1000) // 8 + 1


# (segment, offset of selector word) -> target segment number
selector_target = {}
for s, (fo, ln, fl) in segs.items():
    if not fl & 0x100:  # no relocation data
        continue
    rp = fo + ln
    count = struct.unpack_from("<H", data, rp)[0]
    for k in range(count):
        src, flags, loc, a, _b = struct.unpack_from("<BBHHH", data, rp + 2 + k * 8)
        if src != 2 or flags & 3 != 0:  # selector fixup, internal reference
            continue
        x = loc
        while x != 0xFFFF:
            selector_target[(s, x)] = a & 0xFF
            if flags & 4:  # additive: no chain
                break
            x = struct.unpack_from("<H", data, fo + x)[0]


def word(s, off):
    return struct.unpack_from("<H", data, segs[s][0] + off)[0]


def parse(addr):
    a, b = addr.split(":")
    return segnum(int(a, 16)), int(b, 16)


def cell(s, off):
    t = selector_target.get((s, off + 2))
    return f"{sel(t):04x}:{word(s, off):04x}" if t else f"----:{word(s, off):04x}"


def cmd_cells(addr, n="32"):
    s, base = parse(addr)
    for k in range(int(n, 0)):
        off = base + 4 * k
        print(f"{sel(s):04x}:{off:04x} +{4 * k:#05x} -> {cell(s, off)}")


def cmd_refs(addr):
    ts, toff = parse(addr)
    for (s, loc), t in sorted(selector_target.items()):
        if t == ts and word(s, loc - 2) == toff:
            print(f"far-pointer cell {sel(s):04x}:{loc - 2:04x} -> {sel(ts):04x}:{toff:04x}")


def cmd_install(addr):
    ts, toff = parse(addr)
    pat = struct.pack("<H", toff)
    for s, (fo, ln, _fl) in segs.items():
        blob = data[fo:fo + ln]
        i = blob.find(pat)
        while i >= 0:
            if any(selector_target.get((s, l)) == ts for l in range(i - 12, i + 14)):
                print(f"immediate at {sel(s):04x}:{i:04x} bytes={blob[max(0, i - 4):i + 8].hex(' ')}")
            i = blob.find(pat, i + 1)


if __name__ == "__main__":
    print(f"# input {EXE.name} sha256={SHA256}")
    {"cells": cmd_cells, "refs": cmd_refs, "install": cmd_install}[sys.argv[1]](*sys.argv[2:])
