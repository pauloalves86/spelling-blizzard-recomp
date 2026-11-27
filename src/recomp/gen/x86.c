/* x86-16 decoder: see x86.h. */
#include "x86.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

const char *const x86_mnemonic_names[X86_COUNT] = {
#define X86_NAME(id, name) name,
    X86_MNEMONICS(X86_NAME)
#undef X86_NAME
};
const char *const x86_reg8[8] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};
const char *const x86_reg16[8] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
const char *const x86_reg32[8] = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"};
const char *const x86_sreg[6] = {"es", "cs", "ss", "ds", "fs", "gs"};
const char *const x86_cc[16] = {"o", "no", "b", "ae", "e", "ne", "be", "a",
                                "s", "ns", "p", "np", "l", "ge", "le", "g"};

enum { RG_BX = 3, RG_BP = 5, RG_SI = 6, RG_DI = 7 };
static const uint8_t modrm_bases[8][2] = {
    {RG_BX, RG_SI}, {RG_BX, RG_DI}, {RG_BP, RG_SI}, {RG_BP, RG_DI}, {RG_SI, 0}, {RG_DI, 0}, {RG_BP, 0}, {RG_BX, 0}};
static const uint8_t modrm_nbase[8] = {2, 2, 2, 2, 1, 1, 1, 1};

static const uint8_t grp1[8] = {X86_ADD, X86_OR, X86_ADC, X86_SBB, X86_AND, X86_SUB, X86_XOR, X86_CMP};
static const uint8_t grp2[8] = {X86_ROL, X86_ROR, X86_RCL, X86_RCR, X86_SHL, X86_SHR, X86_SAL, X86_SAR};

typedef struct {
    const uint8_t *c;
    size_t n;
    size_t p;
    uint32_t start;
    int segov;   /* -1 = none */
    int opsize;
    int imm_off, disp_off;
    X86Instr *ins;
    char *err;
    size_t errcap;
    jmp_buf jb;
} St;

static void fail(St *s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s->err, s->errcap, fmt, ap);
    va_end(ap);
    longjmp(s->jb, 1);
}

static unsigned u8(St *s) {
    if (s->p >= s->n) fail(s, "0x%x: past end", (unsigned)s->start);
    return s->c[s->p++];
}
static int s8(St *s) {
    unsigned v = u8(s);
    return v & 0x80 ? (int)v - 256 : (int)v;
}
static unsigned u16(St *s) {
    if (s->p + 2 > s->n) fail(s, "0x%x: past end", (unsigned)s->start);
    unsigned v = s->c[s->p] | (unsigned)s->c[s->p + 1] << 8;
    s->p += 2;
    return v;
}
static uint32_t u32(St *s) {
    if (s->p + 4 > s->n) fail(s, "0x%x: past end", (unsigned)s->start);
    uint32_t v = s->c[s->p] | (uint32_t)s->c[s->p + 1] << 8 | (uint32_t)s->c[s->p + 2] << 16 |
                 (uint32_t)s->c[s->p + 3] << 24;
    s->p += 4;
    return v;
}

static X86Op REG(int size, int idx) {
    X86Op o = {0};
    o.kind = OPK_REG; o.size = (uint8_t)size; o.idx = (uint8_t)idx;
    return o;
}
static X86Op SREG(int idx) {
    X86Op o = {0};
    o.kind = OPK_SREG; o.idx = (uint8_t)idx;
    return o;
}
static X86Op IMM(uint32_t v, int size) {
    X86Op o = {0};
    o.kind = OPK_IMM; o.size = (uint8_t)size; o.val = v;
    return o;
}
static X86Op MEM(int size, int seg, int nbase, const uint8_t *base, unsigned disp, int explicit_seg) {
    X86Op o = {0};
    o.kind = OPK_MEM; o.size = (uint8_t)size; o.seg = (uint8_t)seg; o.nbase = (uint8_t)nbase;
    for (int i = 0; i < nbase; i++) o.base[i] = base[i];
    o.disp = (uint16_t)disp; o.explicit_seg = (uint8_t)explicit_seg;
    return o;
}

static X86Op imm(St *s, int size) {
    if (size == 1) return IMM(u8(s), 1);
    if (size == 2) {
        s->imm_off = (int)(s->p - s->start);
        return IMM(u16(s), 2);
    }
    return IMM(u32(s), 4);
}

static uint32_t sext8(St *s, int osz) {
    return (uint32_t)s8(s) & (osz == 2 ? 0xFFFFu : 0xFFFFFFFFu);
}

/* Returns the reg field; *rm gets the r/m operand. */
static int modrm(St *s, int size, X86Op *rm) {
    unsigned m = u8(s);
    unsigned mod = m >> 6, reg = (m >> 3) & 7, r = m & 7;
    if (mod == 3) {
        *rm = REG(size ? size : 2, (int)r);
        return (int)reg;
    }
    unsigned disp;
    int nbase;
    const uint8_t *base;
    if (mod == 0 && r == 6) {
        s->disp_off = (int)(s->p - s->start);
        disp = u16(s);
        nbase = 0;
        base = modrm_bases[0];
    } else {
        base = modrm_bases[r];
        nbase = modrm_nbase[r];
        if (mod == 1) disp = (unsigned)s8(s) & 0xFFFF;
        else if (mod == 2) {
            s->disp_off = (int)(s->p - s->start);
            disp = u16(s);
        } else disp = 0;
    }
    int has_bp = 0;
    for (int i = 0; i < nbase; i++) has_bp |= base[i] == RG_BP;
    int seg = s->segov >= 0 ? s->segov : (has_bp ? SR_SS : SR_DS);
    *rm = MEM(size, seg, nbase, base, disp, s->segov >= 0);
    return (int)reg;
}

/* Fills the instruction like the Python decoder's I(op, *ops, **kw). */
static void I(St *s, int mnem, int nops, X86Op a, X86Op b, X86Op c) {
    X86Instr *i = s->ins;
    i->mnem = (uint8_t)mnem;
    i->nops = (uint8_t)nops;
    i->ops[0] = a; i->ops[1] = b; i->ops[2] = c;
    i->opsize = (uint8_t)s->opsize;
    i->imm_off = (int8_t)s->imm_off;
    i->disp_off = (int8_t)s->disp_off;
}
#define I0(m) I(s, m, 0, none, none, none)
#define I1(m, a) I(s, m, 1, a, none, none)
#define I2(m, a, b) I(s, m, 2, a, b, none)
#define I3(m, a, b, c) I(s, m, 3, a, b, c)

/* A string op or xlat (built without operands, offsets or the prefix's operand size). */
static void STR(St *s, int mnem, int opsize, int uses_seg) {
    X86Instr *i = s->ins;
    i->mnem = (uint8_t)mnem;
    i->nops = 0;
    i->opsize = (uint8_t)opsize;
    i->imm_off = -1;
    i->disp_off = -1;
    i->src_seg = (uint8_t)(uses_seg && s->segov >= 0 ? s->segov : SR_DS);
}

static void branch(St *s, int mnem, int cc, uint32_t target) {
    X86Op none = {0};
    I0(mnem);
    s->ins->cc = (int8_t)cc;
    s->ins->target = (int32_t)(target & 0xFFFF);
}

static void run0f(St *s) {
    X86Op none = {0}, m, r2;
    unsigned b = u8(s);
    int osz = s->opsize, r;
    if (b >= 0x80 && b <= 0x8F) {
        unsigned d = u16(s);
        branch(s, X86_JCC, (int)(b - 0x80), (uint32_t)(s->p + d));
        return;
    }
    if (b >= 0x90 && b <= 0x9F) {
        modrm(s, 1, &m);
        I1(X86_SETCC, m);
        s->ins->cc = (int8_t)(b - 0x90);
        return;
    }
    switch (b) {
    case 0xA0: I1(X86_PUSH, SREG(SR_FS)); return;
    case 0xA1: I1(X86_POP, SREG(SR_FS)); return;
    case 0xA8: I1(X86_PUSH, SREG(SR_GS)); return;
    case 0xA9: I1(X86_POP, SREG(SR_GS)); return;
    case 0xB6: case 0xB7: case 0xBE: case 0xBF:
        r = modrm(s, (b == 0xB6 || b == 0xBE) ? 1 : 2, &m);
        I2(b < 0xB8 ? X86_MOVZX : X86_MOVSX, REG(osz, r), m);
        return;
    case 0xAF:
        r = modrm(s, osz, &m);
        I2(X86_IMUL2, REG(osz, r), m);
        return;
    case 0xA4: case 0xA5: case 0xAC: case 0xAD:
        r = modrm(s, osz, &m);
        r2 = (b == 0xA4 || b == 0xAC) ? IMM(u8(s), 1) : REG(1, 1);
        I3(b < 0xA8 ? X86_SHLD : X86_SHRD, m, REG(osz, r), r2);
        return;
    case 0xA3: case 0xAB: case 0xB3: case 0xBB:
        r = modrm(s, osz, &m);
        I2(b == 0xA3 ? X86_BT : b == 0xAB ? X86_BTS : b == 0xB3 ? X86_BTR : X86_BTC, m, REG(osz, r));
        return;
    case 0xBA: {
        r = modrm(s, osz, &m);
        if (r < 4) fail(s, "0x%x: 0f ba /%d", (unsigned)s->start, r);
        static const uint8_t bt[4] = {X86_BT, X86_BTS, X86_BTR, X86_BTC};
        I2(bt[r - 4], m, IMM(u8(s), 1));
        return;
    }
    case 0xB2: case 0xB4: case 0xB5:
        r = modrm(s, osz, &m);
        I2(b == 0xB2 ? X86_LSS : b == 0xB4 ? X86_LFS : X86_LGS, REG(osz, r), m);
        return;
    }
    fail(s, "0x%x: unknown opcode 0f 0x%02x", (unsigned)s->start, b);
}

static void run(St *s) {
    X86Op none = {0}, m, im;
    unsigned b = u8(s);
    int osz = s->opsize, r, size;

    if (b < 0x40 && (b & 7) < 6) {
        int op = grp1[b >> 3];
        switch (b & 7) {
        case 0: r = modrm(s, 1, &m); I2(op, m, REG(1, r)); return;
        case 1: r = modrm(s, osz, &m); I2(op, m, REG(osz, r)); return;
        case 2: r = modrm(s, 1, &m); I2(op, REG(1, r), m); return;
        case 3: r = modrm(s, osz, &m); I2(op, REG(osz, r), m); return;
        case 4: im = imm(s, 1); I2(op, REG(1, 0), im); return;
        case 5: im = imm(s, osz); I2(op, REG(osz, 0), im); return;
        }
    }
    if (b == 0x06 || b == 0x0E || b == 0x16 || b == 0x1E) { I1(X86_PUSH, SREG((int)(b >> 3))); return; }
    if (b == 0x07 || b == 0x17 || b == 0x1F) { I1(X86_POP, SREG((int)(b >> 3))); return; }
    if (b >= 0x40 && b <= 0x47) { I1(X86_INC, REG(osz, (int)(b - 0x40))); return; }
    if (b >= 0x48 && b <= 0x4F) { I1(X86_DEC, REG(osz, (int)(b - 0x48))); return; }
    if (b >= 0x50 && b <= 0x57) { I1(X86_PUSH, REG(osz, (int)(b - 0x50))); return; }
    if (b >= 0x58 && b <= 0x5F) { I1(X86_POP, REG(osz, (int)(b - 0x58))); return; }
    if (b >= 0x70 && b <= 0x7F) {
        int d = s8(s);
        branch(s, X86_JCC, (int)(b - 0x70), (uint32_t)((int)s->p + d));
        return;
    }
    if (b >= 0x91 && b <= 0x97) { I2(X86_XCHG, REG(osz, 0), REG(osz, (int)(b - 0x90))); return; }
    if (b >= 0xB0 && b <= 0xB7) { im = imm(s, 1); I2(X86_MOV, REG(1, (int)(b - 0xB0)), im); return; }
    if (b >= 0xB8 && b <= 0xBF) { im = imm(s, osz); I2(X86_MOV, REG(osz, (int)(b - 0xB8)), im); return; }
    if (b >= 0xD8 && b <= 0xDF) {
        r = modrm(s, 0, &m);
        I3(X86_FPU, m, IMM(b, 1), IMM((uint32_t)r, 1));
        return;
    }

    switch (b) {
    case 0x27: I0(X86_DAA); return;
    case 0x2F: I0(X86_DAS); return;
    case 0x37: I0(X86_AAA); return;
    case 0x3F: I0(X86_AAS); return;
    case 0x60: I0(X86_PUSHA); return;
    case 0x61: I0(X86_POPA); return;
    case 0x62: r = modrm(s, osz, &m); I2(X86_BOUND, REG(osz, r), m); return;
    case 0x68: im = imm(s, osz); I1(X86_PUSH, im); return;
    case 0x6A: im = IMM(sext8(s, osz), osz); I1(X86_PUSH, im); return;
    case 0x69: case 0x6B:
        r = modrm(s, osz, &m);
        im = b == 0x69 ? imm(s, osz) : IMM(sext8(s, osz), osz);
        I3(X86_IMUL3, REG(osz, r), m, im);
        return;
    case 0x6C: I0(X86_INS); s->ins->opsize = 1; return;
    case 0x6D: I0(X86_INS); return;
    case 0x6E: I0(X86_OUTS); s->ins->opsize = 1; return;
    case 0x6F: I0(X86_OUTS); return;
    case 0x80: case 0x81: case 0x82: case 0x83:
        size = (b == 0x80 || b == 0x82) ? 1 : osz;
        r = modrm(s, size, &m);
        im = b == 0x83 ? IMM(sext8(s, osz), osz) : imm(s, size);
        I2(grp1[r], m, im);
        return;
    case 0x84: case 0x85:
        size = b == 0x84 ? 1 : osz;
        r = modrm(s, size, &m); I2(X86_TEST, m, REG(size, r)); return;
    case 0x86: case 0x87:
        size = b == 0x86 ? 1 : osz;
        r = modrm(s, size, &m); I2(X86_XCHG, m, REG(size, r)); return;
    case 0x88: case 0x89: case 0x8A: case 0x8B:
        size = (b == 0x88 || b == 0x8A) ? 1 : osz;
        r = modrm(s, size, &m);
        if (b <= 0x89) I2(X86_MOV, m, REG(size, r));
        else I2(X86_MOV, REG(size, r), m);
        return;
    case 0x8C: r = modrm(s, 2, &m); I2(X86_MOV, m, SREG(r)); return;
    case 0x8D: r = modrm(s, 0, &m); I2(X86_LEA, REG(osz, r), m); return;
    case 0x8E: r = modrm(s, 2, &m); I2(X86_MOV, SREG(r), m); return;
    case 0x8F: modrm(s, osz, &m); I1(X86_POP, m); return;
    case 0x90: I0(X86_NOP); return;
    case 0x98: I0(osz == 2 ? X86_CBW : X86_CWDE); return;
    case 0x99: I0(osz == 2 ? X86_CWD : X86_CDQ); return;
    case 0x9A: case 0xEA: {
        s->imm_off = (int)(s->p - s->start);
        unsigned off = u16(s), seg = u16(s);
        I0(b == 0x9A ? X86_CALLF : X86_JMPF);
        s->ins->has_far = 1;
        s->ins->far_seg = (uint16_t)seg;
        s->ins->far_off = (uint16_t)off;
        return;
    }
    case 0x9B: I0(X86_WAIT); return;
    case 0x9C: I0(X86_PUSHF); return;
    case 0x9D: I0(X86_POPF); return;
    case 0x9E: I0(X86_SAHF); return;
    case 0x9F: I0(X86_LAHF); return;
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: {
        size = (b == 0xA0 || b == 0xA2) ? 1 : osz;
        s->disp_off = (int)(s->p - s->start);
        unsigned disp = u16(s);
        m = MEM(size, s->segov >= 0 ? s->segov : SR_DS, 0, modrm_bases[0], disp, s->segov >= 0);
        if (b <= 0xA1) I2(X86_MOV, REG(size, 0), m);
        else I2(X86_MOV, m, REG(size, 0));
        return;
    }
    case 0xA4: case 0xA5: STR(s, X86_MOVS, b == 0xA4 ? 1 : osz, 1); return;
    case 0xA6: case 0xA7: STR(s, X86_CMPS, b == 0xA6 ? 1 : osz, 1); return;
    case 0xA8: case 0xA9:
        size = b == 0xA8 ? 1 : osz;
        im = imm(s, size);
        I2(X86_TEST, REG(size, 0), im);
        return;
    case 0xAA: case 0xAB: STR(s, X86_STOS, b == 0xAA ? 1 : osz, 0); return;
    case 0xAC: case 0xAD: STR(s, X86_LODS, b == 0xAC ? 1 : osz, 1); return;
    case 0xAE: case 0xAF: STR(s, X86_SCAS, b == 0xAE ? 1 : osz, 0); return;
    case 0xC0: case 0xC1: case 0xD0: case 0xD1: case 0xD2: case 0xD3: {
        size = (b == 0xC0 || b == 0xD0 || b == 0xD2) ? 1 : osz;
        r = modrm(s, size, &m);
        X86Op cnt;
        if (b == 0xC0 || b == 0xC1) cnt = IMM(u8(s), 1);
        else if (b == 0xD0 || b == 0xD1) cnt = IMM(1, 1);
        else cnt = REG(1, 1);
        I2(grp2[r], m, cnt);
        return;
    }
    case 0xC2: im = IMM(u16(s), 2); I1(X86_RET, im); return;
    case 0xC3: I0(X86_RET); return;
    case 0xC4: case 0xC5:
        r = modrm(s, osz, &m);
        I2(b == 0xC4 ? X86_LES : X86_LDS, REG(osz, r), m);
        return;
    case 0xC6: case 0xC7:
        size = b == 0xC6 ? 1 : osz;
        modrm(s, size, &m);
        im = imm(s, size);
        I2(X86_MOV, m, im);
        return;
    case 0xC8: {
        unsigned sz = u16(s), lvl = u8(s);
        I2(X86_ENTER, IMM(sz, 2), IMM(lvl, 1));
        return;
    }
    case 0xC9: I0(X86_LEAVE); return;
    case 0xCA: im = IMM(u16(s), 2); I1(X86_RETF, im); return;
    case 0xCB: I0(X86_RETF); return;
    case 0xCC: I1(X86_INT, IMM(3, 1)); return;
    case 0xCD: im = IMM(u8(s), 1); I1(X86_INT, im); return;
    case 0xCE: I0(X86_INTO); return;
    case 0xCF: I0(X86_IRET); return;
    case 0xD4: im = IMM(u8(s), 1); I1(X86_AAM, im); return;
    case 0xD5: im = IMM(u8(s), 1); I1(X86_AAD, im); return;
    case 0xD7: STR(s, X86_XLAT, 2, 1); return;
    case 0xE0: case 0xE1: case 0xE2: case 0xE3: {
        int d = s8(s);
        static const uint8_t lp[4] = {X86_LOOPNE, X86_LOOPE, X86_LOOP, X86_JCXZ};
        branch(s, lp[b - 0xE0], -1, (uint32_t)((int)s->p + d));
        return;
    }
    case 0xE4: im = imm(s, 1); I2(X86_IN, REG(1, 0), im); return;
    case 0xE5: im = imm(s, 1); I2(X86_IN, REG(osz, 0), im); return;
    case 0xE6: im = imm(s, 1); I2(X86_OUT, im, REG(1, 0)); return;
    case 0xE7: im = imm(s, 1); I2(X86_OUT, im, REG(osz, 0)); return;
    case 0xE8: case 0xE9: {
        unsigned d = u16(s);
        branch(s, b == 0xE8 ? X86_CALL : X86_JMP, -1, (uint32_t)(s->p + d));
        return;
    }
    case 0xEB: {
        int d = s8(s);
        branch(s, X86_JMP, -1, (uint32_t)((int)s->p + d));
        return;
    }
    case 0xEC: I2(X86_IN, REG(1, 0), REG(2, 2)); return;
    case 0xED: I2(X86_IN, REG(osz, 0), REG(2, 2)); return;
    case 0xEE: I2(X86_OUT, REG(2, 2), REG(1, 0)); return;
    case 0xEF: I2(X86_OUT, REG(2, 2), REG(osz, 0)); return;
    case 0xF4: I0(X86_HLT); return;
    case 0xF5: I0(X86_CMC); return;
    case 0xF6: case 0xF7:
        size = b == 0xF6 ? 1 : osz;
        r = modrm(s, size, &m);
        if (r == 0 || r == 1) {
            im = imm(s, size);
            I2(X86_TEST, m, im);
        } else {
            static const uint8_t g3[8] = {0, 0, X86_NOT, X86_NEG, X86_MUL, X86_IMUL, X86_DIV, X86_IDIV};
            I1(g3[r], m);
        }
        return;
    case 0xF8: I0(X86_CLC); return;
    case 0xF9: I0(X86_STC); return;
    case 0xFA: I0(X86_CLI); return;
    case 0xFB: I0(X86_STI); return;
    case 0xFC: I0(X86_CLD); return;
    case 0xFD: I0(X86_STD); return;
    case 0xFE:
        r = modrm(s, 1, &m);
        if (r > 1) fail(s, "0x%x: FE /%d", (unsigned)s->start, r);
        I1(r ? X86_DEC : X86_INC, m);
        return;
    case 0xFF: {
        if (s->p >= s->n) fail(s, "0x%x: past end", (unsigned)s->start);
        int r0 = (s->c[s->p] >> 3) & 7;
        static const int8_t sizes[8] = {0, 0, 2, 4, 2, 4, 0, -1};
        static const uint8_t ops[7] = {X86_INC, X86_DEC, X86_CALL_IND, X86_CALLF_IND, X86_JMP_IND,
                                       X86_JMPF_IND, X86_PUSH};
        if (sizes[r0] < 0) fail(s, "0x%x: FF /7", (unsigned)s->start);
        r = modrm(s, sizes[r0] ? sizes[r0] : osz, &m);
        I1(ops[r], m);
        return;
    }
    case 0x0F: run0f(s); return;
    }
    fail(s, "0x%x: unknown opcode 0x%02x", (unsigned)s->start, b);
}

int x86_decode(const uint8_t *code, size_t len, uint32_t addr, X86Instr *out, char *err, size_t errcap) {
    St s;
    memset(out, 0, sizeof(*out));
    out->addr = addr;
    out->cc = -1;
    out->target = -1;
    out->imm_off = out->disp_off = -1;
    out->src_seg = SR_DS;
    s.c = code; s.n = len; s.p = addr; s.start = addr;
    s.segov = -1; s.opsize = 2; s.imm_off = s.disp_off = -1;
    s.ins = out; s.err = err; s.errcap = errcap;
    if (setjmp(s.jb)) return -1;

    int rep = REP_NONE, adsize = 2;
    for (;;) {
        if (s.p >= s.n) fail(&s, "0x%x: past end", (unsigned)addr);
        unsigned b = code[s.p];
        if (b == 0x26) s.segov = SR_ES;
        else if (b == 0x2E) s.segov = SR_CS;
        else if (b == 0x36) s.segov = SR_SS;
        else if (b == 0x3E) s.segov = SR_DS;
        else if (b == 0x64) s.segov = SR_FS;
        else if (b == 0x65) s.segov = SR_GS;
        else if (b == 0x66) s.opsize = 4;
        else if (b == 0x67) adsize = 4;
        else if (b == 0xF0) { /* lock: ignored */ }
        else if (b == 0xF2) rep = REP_REPNE;
        else if (b == 0xF3) rep = REP_REP;
        else break;
        s.p++;
    }
    if (adsize == 4) fail(&s, "0x%x: 32-bit addressing not supported", (unsigned)addr);
    run(&s);
    int m = out->mnem;
    int is_str = m == X86_MOVS || m == X86_STOS || m == X86_LODS || m == X86_CMPS || m == X86_SCAS ||
                 m == X86_INS || m == X86_OUTS;
    out->rep = (uint8_t)(is_str ? rep : REP_NONE);
    if (rep && (m == X86_CMPS || m == X86_SCAS)) out->rep = rep == REP_REP ? REP_REPE : REP_REPNE;
    if (s.p - addr > 255) fail(&s, "0x%x: too long", (unsigned)addr);
    out->length = (uint8_t)(s.p - addr);
    return 0;
}

int x86_op_equal(const X86Op *a, const X86Op *b) {
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
    case OPK_REG: return a->size == b->size && a->idx == b->idx;
    case OPK_SREG: return a->idx == b->idx;
    case OPK_IMM: return a->size == b->size && a->val == b->val;
    case OPK_MEM:
        if (a->size != b->size || a->seg != b->seg || a->nbase != b->nbase || a->disp != b->disp ||
            a->explicit_seg != b->explicit_seg)
            return 0;
        for (int i = 0; i < a->nbase; i++)
            if (a->base[i] != b->base[i]) return 0;
        return 1;
    }
    return 1;
}

static size_t fmt_op(const X86Op *o, char *buf, size_t cap) {
    switch (o->kind) {
    case OPK_REG: {
        const char *const *t = o->size == 1 ? x86_reg8 : o->size == 2 ? x86_reg16 : x86_reg32;
        return (size_t)snprintf(buf, cap, "%s", t[o->idx]);
    }
    case OPK_SREG: return (size_t)snprintf(buf, cap, "%s", o->idx < 6 ? x86_sreg[o->idx] : "?");
    case OPK_IMM: return (size_t)snprintf(buf, cap, "0x%x", (unsigned)o->val);
    case OPK_MEM: {
        size_t n = (size_t)snprintf(buf, cap, "%s:[", x86_sreg[o->seg]);
        for (int i = 0; i < o->nbase && n < cap; i++)
            n += (size_t)snprintf(buf + n, cap - n, "%s%s", i ? "+" : "", x86_reg16[o->base[i]]);
        if ((o->disp || !o->nbase) && n < cap)
            n += (size_t)snprintf(buf + n, cap - n, "%s0x%x", o->nbase ? "+" : "", (unsigned)o->disp);
        if (n < cap) n += (size_t)snprintf(buf + n, cap - n, "]");
        return n;
    }
    }
    return 0;
}

void x86_format(const X86Instr *ins, char *buf, size_t cap) {
    static const char *const reps[4] = {"", "rep ", "repe ", "repne "};
    const char *name = x86_mnemonic_names[ins->mnem];
    size_t n;
    if (ins->cc >= 0 && (ins->mnem == X86_JCC || ins->mnem == X86_SETCC))
        n = (size_t)snprintf(buf, cap, "%s%s%s", reps[ins->rep], ins->mnem == X86_JCC ? "j" : "set",
                             x86_cc[ins->cc]);
    else
        n = (size_t)snprintf(buf, cap, "%s%s", reps[ins->rep], name);
    for (int i = 0; i < ins->nops && n < cap; i++) {
        n += (size_t)snprintf(buf + n, cap - n, i ? ", " : " ");
        if (n < cap) n += fmt_op(&ins->ops[i], buf + n, cap - n);
    }
    if (ins->target >= 0 && n < cap) n += (size_t)snprintf(buf + n, cap - n, " 0x%04x", (unsigned)ins->target);
    if (ins->has_far && n < cap)
        snprintf(buf + n, cap - n, " 0x%04x:0x%04x", (unsigned)ins->far_seg, (unsigned)ins->far_off);
}
