"""Minimal, complete NE (New Executable) parser for the static recompiler.

Parses the header, segment table, per-segment relocations (all source and
target types), entry table, module reference / imported-name tables and
resident/non-resident names. Files are opened read-only.
"""
import struct
from dataclasses import dataclass, field


@dataclass
class Reloc:
    src_type: int      # 0 lobyte, 2 selector, 3 far ptr (sel:off), 5 offset, 11 ptr48, 13 off32
    flags: int         # bits 0-1 target type, bit 2 additive
    offset: int        # first location in segment (chain head unless additive)
    target: tuple      # ('internal', seg, off) | ('movable', seg, entry_ord) | ('ord', mod, ord) | ('name', mod, name) | ('osfix', type)
    locations: list = field(default_factory=list)  # all patched locations (chain walked)


@dataclass
class Segment:
    num: int           # 1-based
    file_off: int
    length: int        # bytes present in file
    flags: int
    minalloc: int
    data: bytes
    relocs: list

    @property
    def is_data(self):
        return bool(self.flags & 1)


class NEFile:
    def __init__(self, path):
        self.path = path
        self.raw = open(path, "rb").read()
        d = self.raw
        ne = struct.unpack_from("<H", d, 0x3C)[0]
        assert d[ne:ne + 2] == b"NE", "not an NE file"
        self.ne = ne
        h = lambda o, f="<H": struct.unpack_from(f, d, ne + o)[0]
        self.flags = h(0x0C)
        self.autodata = h(0x0E)
        self.heap_size = h(0x10)
        self.stack_size = h(0x12)
        self.ip, self.cs = h(0x14), h(0x16)
        self.sp, self.ss = h(0x18), h(0x1A)
        nseg = h(0x1C)
        nmod = h(0x1E)
        self.nonres_size = h(0x20)
        segtab = ne + h(0x22)
        self.restab = ne + h(0x24)
        restab_res = ne + h(0x26)
        modref = ne + h(0x28)
        self.impnames = ne + h(0x2A)
        enttab = ne + h(0x04)
        entlen = h(0x06)
        self.nonres_off = h(0x2C, "<I")
        align = h(0x32) or 9
        self.align = align
        self.expver = h(0x3E) if ne + 0x3E < len(d) else 0

        self.modules = []
        for i in range(nmod):
            o = struct.unpack_from("<H", d, modref + 2 * i)[0]
            self.modules.append(self.impname(o))

        self.segments = {}
        for i in range(nseg):
            off, ln, fl, mn = struct.unpack_from("<HHHH", d, segtab + i * 8)
            fo = off << align
            length = (ln or 0x10000) if off else 0
            data = d[fo:fo + length] if off else b""
            mina = mn or 0x10000
            seg = Segment(i + 1, fo, length, fl, mina, data, [])
            if off and fl & 0x100:
                seg.relocs = self._relocs(seg)
            self.segments[i + 1] = seg

        self.entries = self._entries(enttab, entlen)
        self.resident_names = self._names(self.restab)
        self.nonresident_names = self._names(self.nonres_off, absolute=True) if self.nonres_off else {}
        self.restable_offset = restab_res

    def impname(self, off):
        p = self.impnames + off
        n = self.raw[p]
        return self.raw[p + 1:p + 1 + n].decode("latin-1")

    def _names(self, p, absolute=False):
        d = self.raw
        out = {}
        while True:
            n = d[p]
            if n == 0:
                break
            name = d[p + 1:p + 1 + n].decode("latin-1")
            ordv = struct.unpack_from("<H", d, p + 1 + n)[0]
            out[ordv] = name
            p += 3 + n
        return out

    def _entries(self, p, length):
        d = self.raw
        end = p + length
        ordv = 1
        out = {}
        while p < end:
            cnt = d[p]
            if cnt == 0:
                break
            ind = d[p + 1]
            p += 2
            for _ in range(cnt):
                if ind == 0:
                    pass
                elif ind == 0xFF:
                    fl = d[p]
                    seg = d[p + 3]
                    off = struct.unpack_from("<H", d, p + 4)[0]
                    out[ordv] = (seg, off, fl)
                    p += 6
                    ordv += 1
                    continue
                else:
                    fl = d[p]
                    off = struct.unpack_from("<H", d, p + 1)[0]
                    out[ordv] = (ind, off, fl)
                    p += 3
                    ordv += 1
                    continue
                ordv += 1
        return out

    def _relocs(self, seg):
        d = self.raw
        rp = seg.file_off + seg.length
        count = struct.unpack_from("<H", d, rp)[0]
        out = []
        for k in range(count):
            src, flags, loc, a, b = struct.unpack_from("<BBHHH", d, rp + 2 + k * 8)
            tt = flags & 3
            if tt == 0:
                if (a & 0xFF) == 0xFF:
                    target = ("movable", None, b)  # entry ordinal b
                    ent = self.entries_raw_lookup(b)
                    if ent:
                        target = ("internal", ent[0], ent[1])
                else:
                    target = ("internal", a & 0xFF, b)
            elif tt == 1:
                target = ("ord", self.modules[a - 1], b)
            elif tt == 2:
                target = ("name", self.modules[a - 1], self.impname(b))
            else:
                target = ("osfix", a, b)
            r = Reloc(src, flags, loc, target)
            x = loc
            seen = set()
            while True:
                r.locations.append(x)
                if flags & 4:
                    break
                if x + 2 > len(seg.data):
                    break
                nxt = struct.unpack_from("<H", seg.data, x)[0]
                if nxt == 0xFFFF or nxt in seen:
                    break
                seen.add(x)
                x = nxt
            out.append(r)
        return out

    def entries_raw_lookup(self, ordv):
        # entries are parsed after segments; resolve lazily
        if not hasattr(self, "entries"):
            self._pending = True
            return None
        return self.entries.get(ordv)


def sel_of(segnum):
    """Runtime selector == Ghidra segment value for BLIZZARD.EXE segment N."""
    return 0x1000 + (segnum - 1) * 8
