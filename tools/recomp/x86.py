"""x86 real/16-bit protected-mode instruction decoder for the static recompiler.

Covers the 8086/186/286 integer set plus the 386 extensions a 1994 Win16
compiler emits (0x66 operand-size prefix, 0F xx: movzx/movsx/jcc near/setcc/
shld/shrd/bt*/imul r,rm/push fs,gs). x87 escape opcodes are decoded for length
only. Anything unknown raises DecodeError so gaps are never silently skipped.
"""
import struct
from dataclasses import dataclass, field


class DecodeError(Exception):
    pass


REG8 = ["al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"]
REG16 = ["ax", "cx", "dx", "bx", "sp", "bp", "si", "di"]
REG32 = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]
SREG = ["es", "cs", "ss", "ds", "fs", "gs"]
CC = ["o", "no", "b", "ae", "e", "ne", "be", "a", "s", "ns", "p", "np", "l", "ge", "le", "g"]


@dataclass(frozen=True)
class Reg:
    size: int  # 1, 2, 4
    idx: int

    def __str__(self):
        return {1: REG8, 2: REG16, 4: REG32}[self.size][self.idx]


@dataclass(frozen=True)
class SReg:
    idx: int

    def __str__(self):
        return SREG[self.idx]


@dataclass(frozen=True)
class Imm:
    val: int
    size: int

    def __str__(self):
        return hex(self.val)


@dataclass(frozen=True)
class Mem:
    size: int            # access size in bytes (0 = n/a, e.g. lea)
    seg: str             # effective segment register name
    base: tuple          # register names summed, e.g. ("bx", "si")
    disp: int            # 16-bit displacement (unsigned)
    explicit_seg: bool = False

    def __str__(self):
        parts = list(self.base)
        if self.disp or not parts:
            parts.append(hex(self.disp))
        return f"{self.seg}:[{'+'.join(parts)}]"


@dataclass(frozen=True)
class FarImm:
    seg: int
    off: int


@dataclass
class Instr:
    addr: int
    length: int
    op: str
    ops: tuple = ()
    rep: str = ""          # "rep", "repe", "repne"
    opsize: int = 2        # operand size for string ops etc.
    raw: bytes = b""
    target: int = None     # near branch target (offset)
    far: FarImm = None     # direct far call/jmp
    cc: str = None         # condition code for jcc/setcc/loop variants
    imm_off: int = None    # offset within instruction of the 16-bit immediate (for relocations)
    disp_off: int = None   # offset within instruction of the 16-bit memory displacement
    extra: dict = field(default_factory=dict)

    def __str__(self):
        s = f"{self.rep + ' ' if self.rep else ''}{self.op}"
        if self.cc:
            s = s.replace("cc", self.cc)
        if self.ops:
            s += " " + ", ".join(str(o) for o in self.ops)
        if self.target is not None:
            s += f" {self.target:#06x}"
        if self.far is not None:
            s += f" {self.far.seg:#06x}:{self.far.off:#06x}"
        return s


MODRM_BASES = [("bx", "si"), ("bx", "di"), ("bp", "si"), ("bp", "di"), ("si",), ("di",), ("bp",), ("bx",)]

GRP1 = ["add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"]
GRP2 = ["rol", "ror", "rcl", "rcr", "shl", "shr", "sal", "sar"]


class Decoder:
    def __init__(self, code: bytes):
        self.code = code

    def decode(self, addr: int) -> Instr:
        c = self.code
        p = addr
        segov = None
        rep = ""
        opsize = 2
        adsize = 2
        lock = False
        while True:
            if p >= len(c):
                raise DecodeError(f"{addr:#x}: past end")
            b = c[p]
            if b in (0x26, 0x2E, 0x36, 0x3E):
                segov = {0x26: "es", 0x2E: "cs", 0x36: "ss", 0x3E: "ds"}[b]
            elif b == 0x64:
                segov = "fs"
            elif b == 0x65:
                segov = "gs"
            elif b == 0x66:
                opsize = 4
            elif b == 0x67:
                adsize = 4
            elif b == 0xF0:
                lock = True
            elif b == 0xF2:
                rep = "repne"
            elif b == 0xF3:
                rep = "rep"
            else:
                break
            p += 1
        if adsize == 4:
            raise DecodeError(f"{addr:#x}: 32-bit addressing not supported")
        st = _State(c, p, segov, opsize)
        ins = st.run(addr)
        ins.rep = rep if ins.op in ("movs", "stos", "lods", "cmps", "scas", "ins", "outs") else ""
        if rep and ins.op in ("cmps", "scas"):
            ins.rep = "repe" if rep == "rep" else "repne"
        ins.length = st.p - addr
        ins.raw = bytes(c[addr:st.p])
        ins.extra["lock"] = lock
        return ins


class _State:
    def __init__(self, c, p, segov, opsize):
        self.c = c
        self.p = p
        self.segov = segov
        self.opsize = opsize
        self.imm_off = None
        self.disp_off = None
        self.start = None

    def u8(self):
        v = self.c[self.p]
        self.p += 1
        return v

    def s8(self):
        v = self.u8()
        return v - 256 if v & 0x80 else v

    def u16(self):
        v = struct.unpack_from("<H", self.c, self.p)[0]
        self.p += 2
        return v

    def u32(self):
        v = struct.unpack_from("<I", self.c, self.p)[0]
        self.p += 4
        return v

    def imm(self, size):
        if size == 1:
            return Imm(self.u8(), 1)
        if size == 2:
            self.imm_off = self.p - self.start
            return Imm(self.u16(), 2)
        return Imm(self.u32(), 4)

    def modrm(self, size):
        m = self.u8()
        mod, reg, rm = m >> 6, (m >> 3) & 7, m & 7
        if mod == 3:
            return reg, (Reg(size, rm) if size else Reg(2, rm)), True
        if mod == 0 and rm == 6:
            self.disp_off = self.p - self.start
            disp = self.u16()
            base = ()
        else:
            base = MODRM_BASES[rm]
            if mod == 1:
                disp = self.s8() & 0xFFFF
            elif mod == 2:
                self.disp_off = self.p - self.start
                disp = self.u16()
            else:
                disp = 0
        seg = self.segov or ("ss" if "bp" in base else "ds")
        return reg, Mem(size, seg, base, disp, self.segov is not None), False

    def run(self, addr):
        self.start = addr
        b = self.u8()
        osz = self.opsize
        I = lambda op, *ops, **kw: Instr(addr, 0, op, tuple(ops), opsize=osz, imm_off=self.imm_off,
                                         disp_off=self.disp_off, **kw)

        # ALU ops 00-3F pattern
        if b < 0x40 and (b & 7) < 6:
            op = GRP1[b >> 3]
            k = b & 7
            if k == 0:
                r, m, _ = self.modrm(1); return I(op, m, Reg(1, r))
            if k == 1:
                r, m, _ = self.modrm(osz); return I(op, m, Reg(osz, r))
            if k == 2:
                r, m, _ = self.modrm(1); return I(op, Reg(1, r), m)
            if k == 3:
                r, m, _ = self.modrm(osz); return I(op, Reg(osz, r), m)
            if k == 4:
                return I(op, Reg(1, 0), self.imm(1))
            if k == 5:
                return I(op, Reg(osz, 0), self.imm(osz))
        if b in (0x06, 0x0E, 0x16, 0x1E):
            return I("push", SReg(b >> 3))
        if b in (0x07, 0x17, 0x1F):
            return I("pop", SReg(b >> 3))
        if b == 0x27: return I("daa")
        if b == 0x2F: return I("das")
        if b == 0x37: return I("aaa")
        if b == 0x3F: return I("aas")
        if 0x40 <= b <= 0x47: return I("inc", Reg(osz, b - 0x40))
        if 0x48 <= b <= 0x4F: return I("dec", Reg(osz, b - 0x48))
        if 0x50 <= b <= 0x57: return I("push", Reg(osz, b - 0x50))
        if 0x58 <= b <= 0x5F: return I("pop", Reg(osz, b - 0x58))
        if b == 0x60: return I("pusha")
        if b == 0x61: return I("popa")
        if b == 0x62:
            r, m, _ = self.modrm(osz); return I("bound", Reg(osz, r), m)
        if b == 0x68: return I("push", self.imm(osz))
        if b == 0x6A: return I("push", Imm(self.s8() & (0xFFFF if osz == 2 else 0xFFFFFFFF), osz))
        if b in (0x69, 0x6B):
            r, m, _ = self.modrm(osz)
            if b == 0x69:
                im = self.imm(osz)
            else:
                im = Imm(self.s8() & (0xFFFF if osz == 2 else 0xFFFFFFFF), osz)
            return I("imul3", Reg(osz, r), m, im)
        if b == 0x6C: return I("ins", opsize=1)
        if b == 0x6D: return I("ins")
        if b == 0x6E: return I("outs", opsize=1)
        if b == 0x6F: return I("outs")
        if 0x70 <= b <= 0x7F:
            d = self.s8()
            return I("jcc", cc=CC[b - 0x70], target=(self.p + d) & 0xFFFF)
        if b in (0x80, 0x81, 0x82, 0x83):
            size = 1 if b in (0x80, 0x82) else osz
            r, m, _ = self.modrm(size)
            if b == 0x83:
                im = Imm(self.s8() & (0xFFFF if osz == 2 else 0xFFFFFFFF), osz)
            else:
                im = self.imm(size)
            return I(GRP1[r], m, im)
        if b in (0x84, 0x85):
            size = 1 if b == 0x84 else osz
            r, m, _ = self.modrm(size); return I("test", m, Reg(size, r))
        if b in (0x86, 0x87):
            size = 1 if b == 0x86 else osz
            r, m, _ = self.modrm(size); return I("xchg", m, Reg(size, r))
        if 0x88 <= b <= 0x8B:
            size = 1 if b in (0x88, 0x8A) else osz
            r, m, _ = self.modrm(size)
            if b <= 0x89:
                return I("mov", m, Reg(size, r))
            return I("mov", Reg(size, r), m)
        if b == 0x8C:
            r, m, isreg = self.modrm(2)
            return I("mov", m, SReg(r))
        if b == 0x8D:
            r, m, _ = self.modrm(0); return I("lea", Reg(osz, r), m)
        if b == 0x8E:
            r, m, _ = self.modrm(2); return I("mov", SReg(r), m)
        if b == 0x8F:
            r, m, _ = self.modrm(osz); return I("pop", m)
        if b == 0x90: return I("nop")
        if 0x91 <= b <= 0x97: return I("xchg", Reg(osz, 0), Reg(osz, b - 0x90))
        if b == 0x98: return I("cbw" if osz == 2 else "cwde")
        if b == 0x99: return I("cwd" if osz == 2 else "cdq")
        if b == 0x9A:
            self.imm_off = self.p - self.start
            off = self.u16(); seg = self.u16()
            return I("callf", far=FarImm(seg, off))
        if b == 0x9B: return I("wait")
        if b == 0x9C: return I("pushf")
        if b == 0x9D: return I("popf")
        if b == 0x9E: return I("sahf")
        if b == 0x9F: return I("lahf")
        if 0xA0 <= b <= 0xA3:
            size = 1 if b in (0xA0, 0xA2) else osz
            self.disp_off = self.p - self.start
            disp = self.u16()
            m = Mem(size, self.segov or "ds", (), disp, self.segov is not None)
            if b <= 0xA1:
                return I("mov", Reg(size, 0), m)
            return I("mov", m, Reg(size, 0))
        if b in (0xA4, 0xA5): return Instr(addr, 0, "movs", opsize=1 if b == 0xA4 else osz, extra={"seg": self.segov or "ds"})
        if b in (0xA6, 0xA7): return Instr(addr, 0, "cmps", opsize=1 if b == 0xA6 else osz, extra={"seg": self.segov or "ds"})
        if b in (0xA8, 0xA9):
            size = 1 if b == 0xA8 else osz
            return I("test", Reg(size, 0), self.imm(size))
        if b in (0xAA, 0xAB): return Instr(addr, 0, "stos", opsize=1 if b == 0xAA else osz)
        if b in (0xAC, 0xAD): return Instr(addr, 0, "lods", opsize=1 if b == 0xAC else osz, extra={"seg": self.segov or "ds"})
        if b in (0xAE, 0xAF): return Instr(addr, 0, "scas", opsize=1 if b == 0xAE else osz)
        if 0xB0 <= b <= 0xB7: return I("mov", Reg(1, b - 0xB0), self.imm(1))
        if 0xB8 <= b <= 0xBF: return I("mov", Reg(osz, b - 0xB8), self.imm(osz))
        if b in (0xC0, 0xC1, 0xD0, 0xD1, 0xD2, 0xD3):
            size = 1 if b in (0xC0, 0xD0, 0xD2) else osz
            r, m, _ = self.modrm(size)
            if b in (0xC0, 0xC1):
                cnt = Imm(self.u8(), 1)
            elif b in (0xD0, 0xD1):
                cnt = Imm(1, 1)
            else:
                cnt = Reg(1, 1)  # cl
            return I(GRP2[r], m, cnt)
        if b == 0xC2: return I("ret", Imm(self.u16(), 2))
        if b == 0xC3: return I("ret")
        if b in (0xC4, 0xC5):
            r, m, _ = self.modrm(osz)
            return I("les" if b == 0xC4 else "lds", Reg(osz, r), m)
        if b in (0xC6, 0xC7):
            size = 1 if b == 0xC6 else osz
            r, m, _ = self.modrm(size)
            return I("mov", m, self.imm(size))
        if b == 0xC8:
            sz = self.u16(); lvl = self.u8()
            return I("enter", Imm(sz, 2), Imm(lvl, 1))
        if b == 0xC9: return I("leave")
        if b == 0xCA: return I("retf", Imm(self.u16(), 2))
        if b == 0xCB: return I("retf")
        if b == 0xCC: return I("int", Imm(3, 1))
        if b == 0xCD: return I("int", Imm(self.u8(), 1))
        if b == 0xCE: return I("into")
        if b == 0xCF: return I("iret")
        if b == 0xD4: return I("aam", Imm(self.u8(), 1))
        if b == 0xD5: return I("aad", Imm(self.u8(), 1))
        if b == 0xD7: return Instr(addr, 0, "xlat", extra={"seg": self.segov or "ds"})
        if 0xD8 <= b <= 0xDF:
            r, m, _ = self.modrm(0)
            return I("fpu", m, Imm(b, 1), Imm(r, 1))
        if b == 0xE0: d = self.s8(); return I("loopne", target=(self.p + d) & 0xFFFF)
        if b == 0xE1: d = self.s8(); return I("loope", target=(self.p + d) & 0xFFFF)
        if b == 0xE2: d = self.s8(); return I("loop", target=(self.p + d) & 0xFFFF)
        if b == 0xE3: d = self.s8(); return I("jcxz", target=(self.p + d) & 0xFFFF)
        if b == 0xE4: return I("in", Reg(1, 0), self.imm(1))
        if b == 0xE5: return I("in", Reg(osz, 0), self.imm(1))
        if b == 0xE6: return I("out", self.imm(1), Reg(1, 0))
        if b == 0xE7: return I("out", self.imm(1), Reg(osz, 0))
        if b == 0xE8:
            d = self.u16()
            return I("call", target=(self.p + d) & 0xFFFF)
        if b == 0xE9:
            d = self.u16()
            return I("jmp", target=(self.p + d) & 0xFFFF)
        if b == 0xEA:
            self.imm_off = self.p - self.start
            off = self.u16(); seg = self.u16()
            return I("jmpf", far=FarImm(seg, off))
        if b == 0xEB:
            d = self.s8()
            return I("jmp", target=(self.p + d) & 0xFFFF)
        if b == 0xEC: return I("in", Reg(1, 0), Reg(2, 2))
        if b == 0xED: return I("in", Reg(osz, 0), Reg(2, 2))
        if b == 0xEE: return I("out", Reg(2, 2), Reg(1, 0))
        if b == 0xEF: return I("out", Reg(2, 2), Reg(osz, 0))
        if b == 0xF4: return I("hlt")
        if b == 0xF5: return I("cmc")
        if b in (0xF6, 0xF7):
            size = 1 if b == 0xF6 else osz
            r, m, _ = self.modrm(size)
            if r in (0, 1):
                return I("test", m, self.imm(size))
            return I(["", "", "not", "neg", "mul", "imul", "div", "idiv"][r], m)
        if b == 0xF8: return I("clc")
        if b == 0xF9: return I("stc")
        if b == 0xFA: return I("cli")
        if b == 0xFB: return I("sti")
        if b == 0xFC: return I("cld")
        if b == 0xFD: return I("std")
        if b == 0xFE:
            r, m, _ = self.modrm(1)
            if r > 1:
                raise DecodeError(f"{addr:#x}: FE /{r}")
            return I(["inc", "dec"][r], m)
        if b == 0xFF:
            m0 = self.c[self.p]
            r = (m0 >> 3) & 7
            size = {0: osz, 1: osz, 2: 2, 3: 4, 4: 2, 5: 4, 6: osz}.get(r)
            if size is None:
                raise DecodeError(f"{addr:#x}: FF /7")
            r, m, _ = self.modrm(size)
            return I(["inc", "dec", "call_ind", "callf_ind", "jmp_ind", "jmpf_ind", "push"][r], m)
        if b == 0x0F:
            return self.run0f(addr, I)
        raise DecodeError(f"{addr:#x}: unknown opcode {b:#04x}")

    def run0f(self, addr, I):
        b = self.u8()
        osz = self.opsize
        if 0x80 <= b <= 0x8F:
            d = self.u16()
            return I("jcc", cc=CC[b - 0x80], target=(self.p + d) & 0xFFFF)
        if 0x90 <= b <= 0x9F:
            r, m, _ = self.modrm(1)
            return I("setcc", m, cc=CC[b - 0x90])
        if b == 0xA0: return I("push", SReg(4))
        if b == 0xA1: return I("pop", SReg(4))
        if b == 0xA8: return I("push", SReg(5))
        if b == 0xA9: return I("pop", SReg(5))
        if b in (0xB6, 0xB7, 0xBE, 0xBF):
            src = 1 if b in (0xB6, 0xBE) else 2
            r, m, _ = self.modrm(src)
            return I("movzx" if b < 0xB8 else "movsx", Reg(osz, r), m)
        if b == 0xAF:
            r, m, _ = self.modrm(osz); return I("imul2", Reg(osz, r), m)
        if b in (0xA4, 0xA5, 0xAC, 0xAD):
            r, m, _ = self.modrm(osz)
            cnt = Imm(self.u8(), 1) if b in (0xA4, 0xAC) else Reg(1, 1)
            return I("shld" if b < 0xA8 else "shrd", m, Reg(osz, r), cnt)
        if b in (0xA3, 0xAB, 0xB3, 0xBB):
            r, m, _ = self.modrm(osz)
            return I({0xA3: "bt", 0xAB: "bts", 0xB3: "btr", 0xBB: "btc"}[b], m, Reg(osz, r))
        if b == 0xBA:
            r, m, _ = self.modrm(osz)
            return I({4: "bt", 5: "bts", 6: "btr", 7: "btc"}[r], m, Imm(self.u8(), 1))
        if b in (0xB2, 0xB4, 0xB5):
            r, m, _ = self.modrm(osz)
            return I({0xB2: "lss", 0xB4: "lfs", 0xB5: "lgs"}[b], Reg(osz, r), m)
        raise DecodeError(f"{addr:#x}: unknown opcode 0f {b:#04x}")
