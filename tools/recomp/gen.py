#!/usr/bin/env python3
"""Static recompiler: original/BLIZZARD.EXE (Win16 NE, x86-16) -> C.

Every function becomes `void f_SSSS_OOOO(void)` operating on the emulated CPU
(src/recomp/cpu.h). Branches become gotos, near/far calls push the original
return address and call the target C function, `ret`/`retf` pop it and
return. Relocations are resolved at generation time (selector immediates,
far calls to other segments, far calls to imported APIs). Indirect far calls
(C++ virtual calls) go through `call_far`, backed by a generated lookup table.

Usage: gen.py <out_dir>
"""
import os
import struct
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
from ne import NEFile, sel_of  # noqa: E402
from x86 import Decoder, Imm, Mem, Reg, SReg, DecodeError  # noqa: E402
import xml.etree.ElementTree as ET  # noqa: E402

EXE = os.path.join(ROOT, "original", "BLIZZARD.EXE")


def load_exports():
    out = {}
    for mod in ("kernel", "gdi", "user", "mmsystem"):
        t = ET.parse(os.path.join(HERE, "exports", f"{mod}.exports"))
        for e in t.getroot():
            out[(mod.upper(), int(e.get("ORDINAL")))] = (e.get("NAME"), int(e.get("PURGE")))
    out[("KEYBOARD", 129)] = ("VKKEYSCAN", 2)
    out[("SHELL", 5)] = ("SHELL_5", -1)
    return out


EXPORTS = load_exports()


class Program:
    def __init__(self):
        self.ne = NEFile(EXE)
        self.code_segs = {n: s for n, s in self.ne.segments.items() if not s.is_data and s.data}
        self.dec = {n: Decoder(s.data) for n, s in self.code_segs.items()}
        # (segnum, location) -> ('sel', segnum) | ('import', mod, ord) | ('import_off', mod, ord)
        self.reloc = {}
        self.imports = {}  # (mod, ord) -> index
        for n, s in self.ne.segments.items():
            for r in s.relocs:
                for loc in r.locations:
                    t = r.target
                    if t[0] == "internal":
                        if r.src_type == 2:
                            self.reloc[(n, loc)] = ("sel", t[1])
                        elif r.src_type == 3:
                            self.reloc[(n, loc)] = ("farptr", t[1], t[2])
                        elif r.src_type == 5:
                            self.reloc[(n, loc)] = ("off", t[1], t[2])
                        else:
                            raise SystemExit(f"unhandled internal reloc {r}")
                    elif t[0] == "ord":
                        key = (t[1], t[2])
                        self.imports.setdefault(key, len(self.imports))
                        self.reloc[(n, loc)] = ({3: "import", 5: "import_off", 2: "import_sel"}[r.src_type], key)
                    else:
                        raise SystemExit(f"unhandled reloc target {r}")
        self.entries = defaultdict(set)
        # functions.txt: entry points from the Ghidra analysis; extra_entries.txt: manual additions.
        for name in ("functions.txt", "extra_entries.txt"):
            path = os.path.join(HERE, name)
            if not os.path.exists(path):
                continue
            for line in open(path):
                line = line.split("#")[0].strip()
                if line:
                    s, o = line.split(":")
                    segn = (int(s, 16) - 0x1000) // 8 + 1
                    if segn in self.code_segs:
                        self.entries[segn].add(int(o, 16))
        self.entries[self.ne.cs].add(self.ne.ip)
        for segn, (es, eo, _fl) in [(e[0], e) for e in self.ne.entries.values()]:
            self.entries[es].add(eo)
        self.funcs = {}  # (seg, entry) -> Func

    def imm_value(self, seg, ins):
        """Resolve a relocation on the instruction's 16-bit immediate."""
        if ins.imm_off is None:
            return None
        return self.reloc.get((seg, ins.addr + ins.imm_off))

    def disp_value(self, seg, ins):
        """Resolve a relocation on the instruction's 16-bit displacement."""
        if ins.disp_off is None:
            return None
        return self.reloc.get((seg, ins.addr + ins.disp_off))


class Func:
    def __init__(self, prog, seg, entry):
        self.prog = prog
        self.seg = seg
        self.entry = entry
        self.ins = {}
        self.labels = set()
        self.tables = {}  # addr of jmp_ind -> list of targets
        self.calls = set()  # (seg, off) direct call targets
        self.tail = set()

    @property
    def name(self):
        return f"f_{sel_of(self.seg):04x}_{self.entry:04x}"

    def decode(self):
        dec = self.prog.dec[self.seg]
        work = [self.entry]
        while work:
            a = work.pop()
            while a not in self.ins:
                try:
                    i = dec.decode(a)
                except (DecodeError, IndexError, struct.error) as e:
                    self.ins[a] = ("bad", str(e))
                    break
                self.ins[a] = i
                if i.op == "jcc" or i.op in ("loop", "loope", "loopne", "jcxz"):
                    self.labels.add(i.target)
                    work.append(i.target)
                elif i.op == "jmp":
                    if i.target in self.prog.entries[self.seg] and i.target != self.entry:
                        self.tail.add(i.target)
                    else:
                        self.labels.add(i.target)
                        work.append(i.target)
                    break
                elif i.op == "call":
                    self.calls.add((self.seg, i.target))
                elif i.op == "jmp_ind":
                    tgts = self.jump_table(i)
                    if tgts:
                        self.tables[a] = tgts
                        for t in tgts:
                            self.labels.add(t)
                            work.append(t)
                    break
                if i.op in ("ret", "retf", "iret", "jmpf", "jmpf_ind", "hlt"):
                    break
                if i.op == "callf":
                    rs = self.prog.reloc.get((self.seg, i.addr + i.imm_off + 2))
                    if rs and rs[0] == "sel":
                        self.calls.add((rs[1], i.far.off))
                a += i.length

    def prog_import_name(self, key):
        return EXPORTS.get(key, (f"{key[0]}_{key[1]}", -1))[0]

    def jump_table(self, i):
        m = i.ops[0]
        if not (isinstance(m, Mem) and m.seg == "cs" and m.base == ("bx",)):
            return None
        # find preceding `cmp ax, N` within the previous few instructions
        n = None
        for back in range(1, 20):
            p = i.addr - back
            ins = self.ins.get(p)
            if ins and not isinstance(ins, tuple) and ins.op == "cmp" and isinstance(ins.ops[0], Reg) \
                    and isinstance(ins.ops[1], Imm) and ins.ops[0].size == 2:
                n = ins.ops[1].val
                break
        if n is None:
            return None
        data = self.prog.code_segs[self.seg].data
        return [struct.unpack_from("<H", data, m.disp + 2 * k)[0] for k in range(n + 1)]


# ---------------------------------------------------------------- emission

R8 = ["AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"]
R16 = ["AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"]
R32 = ["EAX", "ECX", "EDX", "EBX", "cpu.r[R_ESP].e", "cpu.r[R_EBP].e", "cpu.r[R_ESI].e", "cpu.r[R_EDI].e"]
SEGX = {"es": "ES", "ss": "SS", "ds": "DS", "fs": "cpu.s[S_FS]", "gs": "cpu.s[S_GS]"}
T = {1: "uint8_t", 2: "uint16_t", 4: "uint32_t"}
RDF = {1: "RB", 2: "RW", 4: "RD"}
WRF = {1: "WB", 2: "WW", 4: "WD"}
CCX = {
    "o": "cpu.of", "no": "!cpu.of", "b": "cpu.cf", "ae": "!cpu.cf", "e": "cpu.zf", "ne": "!cpu.zf",
    "be": "(cpu.cf || cpu.zf)", "a": "(!cpu.cf && !cpu.zf)", "s": "cpu.sf", "ns": "!cpu.sf",
    "p": "PF()", "np": "!PF()", "l": "(cpu.sf != cpu.of)", "ge": "(cpu.sf == cpu.of)",
    "le": "(cpu.zf || cpu.sf != cpu.of)", "g": "(!cpu.zf && cpu.sf == cpu.of)",
}


class Emitter:
    def __init__(self, prog, func):
        self.prog = prog
        self.f = func
        self.cs = sel_of(func.seg)
        self.out = []

    def w(self, s):
        self.out.append("    " + s)

    def segval(self, seg):
        if seg == "cs":
            return f"0x{self.cs:04x}"
        return SEGX[seg]

    def ea(self, m, ins):
        parts = [R16[["ax", "cx", "dx", "bx", "sp", "bp", "si", "di"].index(b)] for b in m.base]
        disp = m.disp
        rel = self.prog.disp_value(self.f.seg, ins)
        if rel:
            disp = self.reloc_word(rel, disp)
        if disp:
            parts.append(f"0x{disp:04x}")
        if not parts:
            return "0"
        return f"(uint16_t)({' + '.join(parts)})"

    def reloc_word(self, rel, raw):
        kind = rel[0]
        if kind == "sel":
            return sel_of(rel[1])
        if kind == "import_off":
            name = EXPORTS.get(rel[1], ("?", 0))[0]
            if name == "__AHINCR":
                return 8
            if name == "__AHSHIFT":
                return 3
            if name == "__WINFLAGS":
                return 0x0413  # WF_PMODE|WF_CPU386|WF_ENHANCED|WF_80x87? (no 87)
            raise SystemExit(f"unhandled import offset {name}")
        if kind == "off":
            return rel[2]
        raise SystemExit(f"unhandled reloc in operand {rel}")

    def rd(self, o, ins, size=None):
        if isinstance(o, Reg):
            return {1: R8, 2: R16, 4: R32}[o.size][o.idx]
        if isinstance(o, SReg):
            return f"0x{self.cs:04x}" if o.idx == 1 else ["ES", "", "SS", "DS", "cpu.s[S_FS]", "cpu.s[S_GS]"][o.idx]
        if isinstance(o, Imm):
            rel = self.prog.imm_value(self.f.seg, ins)
            v = o.val
            if rel and o.size == 2:
                v = self.reloc_word(rel, v)
            return f"0x{v:x}u" if o.size != 1 else f"0x{v:x}"
        if isinstance(o, Mem):
            sz = size or o.size
            return f"{RDF[sz]}({self.segval(o.seg)}, {self.ea(o, ins)})"
        raise SystemExit(f"bad operand {o}")

    def wr(self, o, val, ins, size=None):
        if isinstance(o, Reg):
            return f"{self.rd(o, ins)} = ({T[o.size]})({val});"
        if isinstance(o, SReg):
            if o.idx == 1:
                raise SystemExit("write to cs")
            return f"{self.rd(o, ins)} = (uint16_t)({val});"
        if isinstance(o, Mem):
            sz = size or o.size
            return f"{WRF[sz]}({self.segval(o.seg)}, {self.ea(o, ins)}, ({T[sz]})({val}));"
        raise SystemExit(f"bad dest {o}")

    def rmw(self, o, ins, fn):
        """Read-modify-write with a single address computation."""
        if isinstance(o, Mem):
            sz = o.size
            self.w(f"{{ uint16_t s_ = {self.segval(o.seg)}, a_ = {self.ea(o, ins)}; "
                   f"{WRF[sz]}(s_, a_, {fn(f'{RDF[sz]}(s_, a_)')}); }}")
        else:
            self.w(f"{self.rd(o, ins)} = {fn(self.rd(o, ins))};")

    def size_of(self, ins):
        for o in ins.ops:
            if isinstance(o, (Reg, Mem)) and getattr(o, "size", 0):
                return o.size
        return ins.opsize

    def call_internal(self, seg, off, far, ret_ip):
        if far:
            self.w(f"PUSH16(0x{self.cs:04x}); PUSH16(0x{ret_ip:04x});")
        else:
            self.w(f"PUSH16(0x{ret_ip:04x});")
        key = (seg, off)
        if off in self.prog.entries[seg]:
            self.w(f"f_{sel_of(seg):04x}_{off:04x}();")
            self.prog.called.add(key)
        else:
            self.prog.new_entries.add(key)
            self.w(f"call_far(0x{sel_of(seg):04x}, 0x{off:04x});")

    def emit(self):
        f = self.f
        p = self.prog
        out_name = f.name
        self.out.append(f"void {out_name}(void) {{")
        addrs = sorted(f.ins)
        # entry may not be the lowest address: jump to it first
        if addrs and addrs[0] != f.entry:
            f.labels.add(f.entry)
            self.w(f"goto L_{f.entry:04x};")
        prev_falls = False
        for idx, a in enumerate(addrs):
            ins = f.ins[a]
            nxt = addrs[idx + 1] if idx + 1 < len(addrs) else None
            if a in f.labels:
                self.out.append(f"L_{a:04x}:;")
            elif idx > 0 and not prev_falls:
                pass
            if isinstance(ins, tuple):
                self.w(f'recomp_fatal("bad instruction at {self.cs:04x}:{a:04x}: {ins[1]}");')
                prev_falls = False
                continue
            self.w(f"/* {a:04x}: {ins} */")
            falls = self.emit_ins(ins)
            end = a + ins.length
            if falls and end != nxt:
                # fall-through to an address that is not the next emitted instruction
                if end in f.ins:
                    f.labels.add(end)
                    self.w(f"goto L_{end:04x};")
                else:
                    self.w(f'recomp_fatal("fell off {self.cs:04x}:{end:04x}");')
            prev_falls = falls
        self.out.append("}")
        return self.out

    def emit_ins(self, i):  # noqa: C901
        op = i.op
        o = i.ops
        sz = self.size_of(i)
        next_ip = i.addr + i.length
        rd = lambda x, s=None: self.rd(x, i, s)
        wr = lambda x, v, s=None: self.w(self.wr(x, v, i, s))
        bits = {1: "8", 2: "16", 4: "32"}

        if op == "mov":
            if isinstance(o[0], SReg) and isinstance(o[1], Mem):
                wr(o[0], rd(o[1], 2))
            elif isinstance(o[1], SReg) and isinstance(o[0], Mem):
                wr(o[0], rd(o[1]), 2)
            else:
                wr(o[0], rd(o[1]))
        elif op in ("add", "or", "adc", "sbb", "and", "sub", "xor"):
            b = bits[o[0].size]
            src = rd(o[1])
            if op == "xor" and o[0] == o[1]:
                self.w(f"{rd(o[0])} = 0; cpu.cf = cpu.of = 0; cpu.zf = 1; cpu.sf = 0; cpu.pres = 0;")
            else:
                self.w(f"{{ {T[o[0].size]} v_ = ({T[o[0].size]}){src};")
                self.rmw(o[0], i, lambda x: f"{op}{b}({x}, v_)")
                self.w("}")
        elif op == "cmp":
            self.w(f"sub{bits[o[0].size]}({rd(o[0])}, ({T[o[0].size]}){rd(o[1])});")
        elif op == "test":
            self.w(f"and{bits[o[0].size]}({rd(o[0])}, ({T[o[0].size]}){rd(o[1])});")
        elif op in ("inc", "dec", "neg"):
            self.rmw(o[0], i, lambda x: f"{op}{bits[o[0].size]}({x})")
        elif op == "not":
            self.rmw(o[0], i, lambda x: f"({T[o[0].size]})~{x}")
        elif op in ("rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar"):
            fn = "shl" if op == "sal" else op
            cnt = rd(o[1])
            self.w(f"{{ unsigned c_ = {cnt};")
            self.rmw(o[0], i, lambda x: f"{fn}{bits[o[0].size]}({x}, c_)")
            self.w("}")
        elif op in ("shld", "shrd"):
            cnt = rd(o[2])
            self.w(f"{{ unsigned c_ = {cnt}; {T[o[0].size]} s2_ = {rd(o[1])};")
            self.rmw(o[0], i, lambda x: f"{op}{bits[o[0].size]}({x}, s2_, c_)")
            self.w("}")
        elif op == "xchg":
            if o[0] == o[1]:
                pass
            elif isinstance(o[0], Mem):
                self.w(f"{{ uint16_t s_ = {self.segval(o[0].seg)}, a_ = {self.ea(o[0], i)}; "
                       f"{T[sz]} t_ = {RDF[sz]}(s_, a_); {WRF[sz]}(s_, a_, {rd(o[1])}); {rd(o[1])} = t_; }}")
            else:
                self.w(f"{{ {T[sz]} t_ = {rd(o[0])}; {rd(o[0])} = {rd(o[1])}; {rd(o[1])} = t_; }}")
        elif op == "lea":
            self.w(f"{rd(o[0])} = {self.ea(o[1], i)};")
        elif op in ("les", "lds", "lss", "lfs", "lgs"):
            sreg = {"les": "ES", "lds": "DS", "lss": "SS", "lfs": "cpu.s[S_FS]", "lgs": "cpu.s[S_GS]"}[op]
            m = o[1]
            self.w(f"{{ uint16_t s_ = {self.segval(m.seg)}, a_ = {self.ea(m, i)}; "
                   f"{rd(o[0])} = {RDF[o[0].size]}(s_, a_); {sreg} = RW(s_, (uint16_t)(a_ + {o[0].size})); }}")
        elif op == "movzx":
            self.w(f"{rd(o[0])} = ({T[o[0].size]}){rd(o[1])};")
        elif op == "movsx":
            st = {1: "int8_t", 2: "int16_t"}[o[1].size]
            self.w(f"{rd(o[0])} = ({T[o[0].size]})({st}){rd(o[1])};")
        elif op == "push":
            x = o[0]
            if isinstance(x, SReg):
                self.w(f"PUSH16({rd(x)});")
            elif isinstance(x, Imm):
                self.w(f"PUSH{'32' if i.opsize == 4 else '16'}({rd(x)});")
            elif isinstance(x, Reg) and x.size == 2 and x.idx == 4:
                self.w("{ uint16_t t_ = SP; PUSH16(t_); }")
            else:
                self.w(f"PUSH{bits[x.size]}({rd(x)});")
        elif op == "pop":
            x = o[0]
            if isinstance(x, SReg):
                self.w(f"{rd(x)} = POP16();")
            elif isinstance(x, Mem):
                self.w(f"{{ {T[x.size]} t_ = POP{bits[x.size]}(); {self.wr(x, 't_', i)} }}")
            else:
                self.w(f"{rd(x)} = POP{bits[x.size]}();")
        elif op == "pusha":
            self.w("{ uint16_t t_ = SP; PUSH16(AX); PUSH16(CX); PUSH16(DX); PUSH16(BX); PUSH16(t_); PUSH16(BP); PUSH16(SI); PUSH16(DI); }")
        elif op == "popa":
            self.w("DI = POP16(); SI = POP16(); BP = POP16(); SP += 2; BX = POP16(); DX = POP16(); CX = POP16(); AX = POP16();")
        elif op == "pushf":
            self.w("PUSH16(get_flags());")
        elif op == "popf":
            self.w("set_flags(POP16());")
        elif op == "lahf":
            self.w("AH = (uint8_t)get_flags();")
        elif op == "sahf":
            self.w("set_flags((uint16_t)((get_flags() & 0xff00) | AH));")
        elif op == "cbw":
            self.w("AX = (uint16_t)(int16_t)(int8_t)AL;")
        elif op == "cwde":
            self.w("EAX = (uint32_t)(int32_t)(int16_t)AX;")
        elif op == "cwd":
            self.w("DX = (AX & 0x8000) ? 0xffff : 0;")
        elif op == "cdq":
            self.w("EDX = (EAX & 0x80000000u) ? 0xffffffffu : 0;")
        elif op in ("mul", "imul", "div", "idiv"):
            self.w(f"cpu_{op}{bits[o[0].size]}({rd(o[0])});")
        elif op == "imul3":
            self.w(f"{rd(o[0])} = cpu_imul3_{bits[o[0].size]}({rd(o[1])}, {rd(o[2])});")
        elif op == "imul2":
            self.w(f"{rd(o[0])} = cpu_imul3_{bits[o[0].size]}({rd(o[0])}, {rd(o[1])});")
        elif op in ("clc", "stc", "cmc", "cld", "std", "cli", "sti"):
            self.w({"clc": "cpu.cf = 0;", "stc": "cpu.cf = 1;", "cmc": "cpu.cf ^= 1;", "cld": "cpu.df = 0;",
                    "std": "cpu.df = 1;", "cli": "cpu.ifl = 0;", "sti": "cpu.ifl = 1;"}[op])
        elif op in ("nop", "wait"):
            pass
        elif op == "setcc":
            wr(o[0], f"{CCX[i.cc]} ? 1 : 0")
        elif op in ("bt", "bts", "btr", "btc"):
            b = bits[o[0].size]
            self.w(f"{{ unsigned n_ = ({rd(o[1])}) & {int(b) - 1}; ")
            if op == "bt":
                self.w(f"cpu.cf = ({rd(o[0])} >> n_) & 1; }}")
            else:
                expr = {"bts": "x | (1u << n_)", "btr": "x & ~(1u << n_)", "btc": "x ^ (1u << n_)"}[op]
                self.w(f"cpu.cf = ({rd(o[0])} >> n_) & 1;")
                self.rmw(o[0], i, lambda x: f"({T[o[0].size]})({expr.replace('x', x)})")
                self.w("}")
        elif op == "enter":
            n, lvl = o[0].val, o[1].val
            if lvl != 0:
                raise SystemExit(f"enter level {lvl}")
            self.w(f"PUSH16(BP); BP = SP; SP -= 0x{n:x};")
        elif op == "leave":
            self.w("SP = BP; BP = POP16();")
        elif op == "xlat":
            self.w(f"AL = RB({self.segval(i.extra['seg'])}, (uint16_t)(BX + AL));")
        elif op in ("movs", "stos", "lods", "scas", "cmps"):
            self.emit_string(i)
        elif op == "jcc":
            self.w(f"if ({CCX[i.cc]}) goto L_{i.target:04x};")
        elif op == "jcxz":
            self.w(f"if (CX == 0) goto L_{i.target:04x};")
        elif op == "loop":
            self.w(f"if (--CX != 0) goto L_{i.target:04x};")
        elif op == "loope":
            self.w(f"if (--CX != 0 && cpu.zf) goto L_{i.target:04x};")
        elif op == "loopne":
            self.w(f"if (--CX != 0 && !cpu.zf) goto L_{i.target:04x};")
        elif op == "jmp":
            if i.target in self.f.tail:
                self.w(f"f_{self.cs:04x}_{i.target:04x}(); return;")
                self.prog.called.add((self.f.seg, i.target))
            else:
                self.w(f"goto L_{i.target:04x};")
            return False
        elif op == "jmp_ind":
            tgts = self.f.tables.get(i.addr)
            if tgts:
                self.w(f"switch ({rd(o[0])}) {{")
                for t in sorted(set(tgts)):
                    self.w(f"case 0x{t:04x}: goto L_{t:04x};")
                self.w(f"default: recomp_bad_jump(0x{self.cs:04x}, 0x{i.addr:04x}, {rd(o[0])});")
                self.w("}")
            else:
                self.w(f"recomp_bad_jump(0x{self.cs:04x}, 0x{i.addr:04x}, {rd(o[0])});")
            return False
        elif op == "jmpf_ind":
            m = o[0]
            self.w(f"{{ uint16_t s_ = {self.segval(m.seg)}, a_ = {self.ea(m, i)}; "
                   f"if (jmp_far(RW(s_, (uint16_t)(a_ + 2)), RW(s_, a_))) return; return; }}")
            return False
        elif op == "jmpf":
            rel = self.prog.reloc.get((self.f.seg, i.addr + i.imm_off + 2))
            if rel and rel[0] == "sel":
                self.w(f"f_{sel_of(rel[1]):04x}_{i.far.off:04x}(); return;")
                self.prog.called.add((rel[1], i.far.off))
            else:
                raise SystemExit(f"unresolved jmpf at {self.cs:04x}:{i.addr:04x}")
            return False
        elif op == "call":
            self.call_internal(self.f.seg, i.target, False, next_ip)
        elif op == "call_ind":
            self.w(f"{{ uint16_t t_ = {rd(o[0])}; PUSH16(0x{next_ip:04x}); call_far(0x{self.cs:04x}, t_); }}")
        elif op == "callf":
            rel_off = self.prog.reloc.get((self.f.seg, i.addr + i.imm_off))
            rel_seg = self.prog.reloc.get((self.f.seg, i.addr + i.imm_off + 2))
            if rel_off and rel_off[0] == "import":
                key = rel_off[1]
                name, purge = EXPORTS.get(key, (f"{key[0]}_{key[1]}", -1))
                self.prog.used_imports[key] = (name, purge)
                self.w(f"PUSH16(0x{self.cs:04x}); PUSH16(0x{next_ip:04x}); api_{key[0]}_{name}();")
            elif rel_seg and rel_seg[0] == "sel":
                self.call_internal(rel_seg[1], i.far.off, True, next_ip)
            else:
                raise SystemExit(f"unresolved callf at {self.cs:04x}:{i.addr:04x} {rel_off} {rel_seg}")
        elif op == "callf_ind":
            m = o[0]
            self.w(f"{{ uint16_t s_ = {self.segval(m.seg)}, a_ = {self.ea(m, i)}; "
                   f"uint16_t to_ = RW(s_, a_), ts_ = RW(s_, (uint16_t)(a_ + 2)); "
                   f"PUSH16(0x{self.cs:04x}); PUSH16(0x{next_ip:04x}); call_far(ts_, to_); }}")
        elif op == "ret":
            n = o[0].val if o else 0
            self.w(f"SP += {2 + n}; return;")
            return False
        elif op == "retf":
            n = o[0].val if o else 0
            self.w(f"SP += {4 + n}; return;")
            return False
        elif op == "iret":
            self.w("SP += 4; set_flags(POP16()); return;")
            return False
        elif op == "int":
            self.w(f"cpu_int(0x{o[0].val:02x});")
        elif op in ("in", "out", "ins", "outs"):
            self.w("cpu_in_out();")
        elif op == "hlt":
            self.w('recomp_fatal("hlt");')
            return False
        else:
            raise SystemExit(f"unhandled op {op} at {self.cs:04x}:{i.addr:04x}: {i}")
        return True

    def emit_string(self, i):
        sz = i.opsize
        t = T[sz]
        step = f"(cpu.df ? -{sz} : {sz})"
        src_seg = self.segval(i.extra.get("seg", "ds"))
        op = i.op
        if op == "movs":
            body = f"{WRF[sz]}(ES, DI, {RDF[sz]}({src_seg}, SI)); SI += {step}; DI += {step};"
        elif op == "stos":
            reg = {1: "AL", 2: "AX", 4: "EAX"}[sz]
            body = f"{WRF[sz]}(ES, DI, {reg}); DI += {step};"
        elif op == "lods":
            reg = {1: "AL", 2: "AX", 4: "EAX"}[sz]
            body = f"{reg} = {RDF[sz]}({src_seg}, SI); SI += {step};"
        elif op == "scas":
            reg = {1: "AL", 2: "AX", 4: "EAX"}[sz]
            body = f"sub{sz * 8}({reg}, {RDF[sz]}(ES, DI)); DI += {step};"
        elif op == "cmps":
            body = f"sub{sz * 8}({RDF[sz]}({src_seg}, SI), {RDF[sz]}(ES, DI)); SI += {step}; DI += {step};"
        if not i.rep:
            self.w(body)
        elif i.rep == "rep" or op in ("movs", "stos", "lods"):
            self.w(f"while (CX) {{ {body} CX--; }}")
        elif i.rep == "repe":
            self.w(f"while (CX) {{ {body} CX--; if (!cpu.zf) break; }}")
        else:
            self.w(f"while (CX) {{ {body} CX--; if (cpu.zf) break; }}")


PROLOGUES = (b"\x55\x8b\xec", b"\x8c\xd8\x90", b"\x1e\x58\x90", b"\x45\x55", b"\x8c\xd8\x45")


def looks_like_entry(prog, seg, off):
    s = prog.code_segs.get(seg)
    if not s or off >= len(s.data) - 4:
        return False
    if s.data[off] == 0xC8 and s.data[off + 3] == 0:  # enter N, 0
        return True
    return any(s.data[off:off + len(p)] == p for p in PROLOGUES)


def discover_pointer_entries(prog, done):
    found = set()
    # bytes covered by already-decoded instructions (a real entry is never mid-instruction)
    inside = set()
    for (seg, e), f in done.items():
        for a, i in f.ins.items():
            if not isinstance(i, tuple):
                for b in range(a + 1, a + i.length):
                    inside.add((seg, b))
    # 1. far pointer cells (offset word followed by a relocated selector word)
    for n, sg in prog.ne.segments.items():
        for (rs, loc), rel in prog.reloc.items():
            if rs != n or rel[0] != "sel" or rel[1] not in prog.code_segs or loc < 2:
                continue
            off = struct.unpack_from("<H", sg.data, loc - 2)[0]
            if off not in prog.entries[rel[1]] and looks_like_entry(prog, rel[1], off):
                found.add((rel[1], off))
    # 2. instruction pairs: `mov [x], OFF` / `push OFF` near a relocated selector immediate
    for (seg, e), f in done.items():
        addrs = sorted(a for a in f.ins if not isinstance(f.ins[a], tuple))
        for k, a in enumerate(addrs):
            i = f.ins[a]
            r = prog.imm_value(seg, i)
            if not r or r[0] != "sel" or r[1] not in prog.code_segs:
                continue
            for j in range(max(0, k - 4), min(len(addrs), k + 5)):
                o = f.ins[addrs[j]]
                if j == k or isinstance(o, tuple):
                    continue
                for op in o.ops:
                    if isinstance(op, Imm) and op.size == 2 and not prog.imm_value(seg, o):
                        if op.val not in prog.entries[r[1]] and looks_like_entry(prog, r[1], op.val):
                            found.add((r[1], op.val))
    good = set()
    for (seg, off) in found:
        if (seg, off) in inside:
            continue
        f = Func(prog, seg, off)
        f.decode()
        if any(isinstance(i, tuple) for i in f.ins.values()):
            continue
        good.add((seg, off))
    return good


def main():
    out_dir = sys.argv[1]
    os.makedirs(out_dir, exist_ok=True)
    prog = Program()
    prog.called = set()
    prog.used_imports = {}
    # discover functions, adding direct call targets as new entries until fixed point
    done = {}
    while True:
        pending = [(s, e) for s in prog.entries for e in prog.entries[s] if (s, e) not in done]
        if not pending:
            break
        for s, e in pending:
            f = Func(prog, s, e)
            f.decode()
            done[(s, e)] = f
            for cs, ce in f.calls:
                prog.entries[cs].add(ce)
    # function pointers built from a relocated selector + nearby offset
    # (window procedures, callbacks, vtables) that no direct call reaches
    found = discover_pointer_entries(prog, done)
    if found:
        for (s, e) in found:
            prog.entries[s].add(e)
        while True:
            pending = [(s, e) for s in prog.entries for e in prog.entries[s] if (s, e) not in done]
            if not pending:
                break
            for s, e in pending:
                f = Func(prog, s, e)
                f.decode()
                done[(s, e)] = f
                for cs, ce in f.calls:
                    prog.entries[cs].add(ce)
    # second pass: tail-jump detection depends on the final entry set
    funcs = {}
    for (s, e) in sorted(done):
        f = Func(prog, s, e)
        f.decode()
        funcs[(s, e)] = f
    prog.new_entries = set()

    chunks = defaultdict(list)
    for (s, e), f in sorted(funcs.items()):
        em = Emitter(prog, f)
        code = em.emit()
        chunks[(s, e // 0x2000)].append("\n".join(code))

    header = '#include "recomp_gen.h"\n\n'
    names = sorted(f.name for f in funcs.values())
    with open(os.path.join(out_dir, "recomp_gen.h"), "w") as fh:
        fh.write("/* Generated by tools/recomp/gen.py - do not edit. */\n#pragma once\n#include \"cpu.h\"\n#include \"api.h\"\n")
        for n in names:
            fh.write(f"void {n}(void);\n")
    for (s, k), bodies in chunks.items():
        with open(os.path.join(out_dir, f"gen_{sel_of(s):04x}_{k}.c"), "w") as fh:
            fh.write("/* Generated by tools/recomp/gen.py from original/BLIZZARD.EXE - do not edit. */\n")
            fh.write("#ifdef __GNUC__\n#pragma GCC diagnostic ignored \"-Wunused-label\"\n#endif\n")
            fh.write(header)
            fh.write("\n\n".join(bodies))
            fh.write("\n")
    with open(os.path.join(out_dir, "gen_table.c"), "w") as fh:
        fh.write('/* Generated function table. */\n#include "recomp_gen.h"\n\n')
        fh.write("const RecompEntry recomp_table[] = {\n")
        for (s, e), f in sorted(funcs.items(), key=lambda kv: (sel_of(kv[0][0]), kv[0][1])):
            fh.write(f"    {{0x{sel_of(s):04x}, 0x{e:04x}, {f.name}}},\n")
        fh.write("};\n")
        fh.write(f"const unsigned recomp_table_count = {len(funcs)};\n")
    with open(os.path.join(out_dir, "gen_imports.h"), "w") as fh:
        fh.write("/* Imported APIs referenced by generated code: X(module, name, ordinal, purge) */\n")
        for key, (name, purge) in sorted(prog.used_imports.items()):
            fh.write(f"X({key[0]}, {name}, {key[1]}, {purge})\n")
    # all imports (incl. ones only referenced from data) with thunk indices
    with open(os.path.join(out_dir, "gen_import_list.h"), "w") as fh:
        fh.write("/* All imports in relocation order: X(index, module, name, ordinal, purge) */\n")
        for key, idx in sorted(prog.imports.items(), key=lambda kv: kv[1]):
            name, purge = EXPORTS.get(key, (f"{key[0]}_{key[1]}", -1))
            fh.write(f"X({idx}, {key[0]}, {name}, {key[1]}, {purge})\n")
    with open(os.path.join(out_dir, "gen_api_decls.h"), "w") as fh:
        fh.write("/* Prototypes for every imported API (generated). */\n")
        for key, idx in sorted(prog.imports.items(), key=lambda kv: kv[1]):
            name, purge = EXPORTS.get(key, (f"{key[0]}_{key[1]}", -1))
            fh.write(f"void api_{key[0]}_{name}(void);\n")
    with open(os.path.join(out_dir, "gen_api_stubs.c"), "w") as fh:
        fh.write("/* Weak fallbacks for APIs not implemented yet (generated). */\n#include \"api.h\"\n\n")
        for key, idx in sorted(prog.imports.items(), key=lambda kv: kv[1]):
            name, purge = EXPORTS.get(key, (f"{key[0]}_{key[1]}", -1))
            fh.write(f"__attribute__((weak)) void api_{key[0]}_{name}(void) "
                     f"{{ api_unimplemented(\"{key[0]}\", \"{name}\", {key[1]}); }}\n")
    print(f"{len(funcs)} functions, {len(prog.used_imports)} imports called directly, "
          f"{len(prog.new_entries)} unresolved call targets")


if __name__ == "__main__":
    main()
