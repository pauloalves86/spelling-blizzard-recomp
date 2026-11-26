/* Static-recompilation runtime: emulated x86-16 CPU state and selector memory.
 *
 * Generated code (build/recomp/gen_*.c) is a mechanical translation of every
 * function in original/BLIZZARD.EXE. It keeps the original register and stack
 * discipline: arguments live on the emulated stack, far pointers are
 * selector:offset pairs, and selectors are chosen so that segment N of the
 * EXE has selector 0x1000 + 8*(N-1) -- identical to Ghidra's addresses, so
 * `1008:72b6` in Ghidra is f_1008_72b6 here and the same address at runtime.
 */
#ifndef SB_RECOMP_CPU_H
#define SB_RECOMP_CPU_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef union Reg32 {
    uint32_t e;
    uint16_t w;
    struct {
        uint8_t l, h;
    } b;
} Reg32;

enum { R_EAX, R_ECX, R_EDX, R_EBX, R_ESP, R_EBP, R_ESI, R_EDI };
enum { S_ES, S_CS, S_SS, S_DS, S_FS, S_GS };

typedef struct CPU {
    Reg32 r[8];
    uint16_t s[6];
    uint8_t cf, zf, sf, of, df, af, ifl;
    uint8_t pres; /* low byte of last result, for PF */
} CPU;

extern CPU cpu;

#define MEM_SIZE (64u * 1024u * 1024u)
#define MAX_SELECTORS 8192u
extern uint8_t *MEM;
extern uint32_t SEGBASE[MAX_SELECTORS];
extern uint32_t SEGLIMIT[MAX_SELECTORS];

#define AX cpu.r[R_EAX].w
#define CX cpu.r[R_ECX].w
#define DX cpu.r[R_EDX].w
#define BX cpu.r[R_EBX].w
#define SP cpu.r[R_ESP].w
#define BP cpu.r[R_EBP].w
#define SI cpu.r[R_ESI].w
#define DI cpu.r[R_EDI].w
#define AL cpu.r[R_EAX].b.l
#define AH cpu.r[R_EAX].b.h
#define CL cpu.r[R_ECX].b.l
#define CH cpu.r[R_ECX].b.h
#define DL cpu.r[R_EDX].b.l
#define DH cpu.r[R_EDX].b.h
#define BL cpu.r[R_EBX].b.l
#define BH cpu.r[R_EBX].b.h
#define EAX cpu.r[R_EAX].e
#define ECX cpu.r[R_ECX].e
#define EDX cpu.r[R_EDX].e
#define EBX cpu.r[R_EBX].e
#define ES cpu.s[S_ES]
#define SS cpu.s[S_SS]
#define DS cpu.s[S_DS]

static inline uint32_t LIN(uint16_t sel, uint16_t off) { return SEGBASE[sel >> 3] + off; }
static inline uint8_t RB(uint16_t s, uint16_t o) { return MEM[LIN(s, o)]; }
static inline uint16_t RW(uint16_t s, uint16_t o) { uint16_t v; memcpy(&v, MEM + LIN(s, o), 2); return v; }
static inline uint32_t RD(uint16_t s, uint16_t o) { uint32_t v; memcpy(&v, MEM + LIN(s, o), 4); return v; }
static inline void WB(uint16_t s, uint16_t o, uint8_t v) { MEM[LIN(s, o)] = v; }
static inline void WW(uint16_t s, uint16_t o, uint16_t v) { memcpy(MEM + LIN(s, o), &v, 2); }
static inline void WD(uint16_t s, uint16_t o, uint32_t v) { memcpy(MEM + LIN(s, o), &v, 4); }
/* Host pointer for a far pointer (valid until the block moves). */
static inline void *FARPTR(uint16_t s, uint16_t o) { return MEM + LIN(s, o); }

static inline void PUSH16(uint16_t v) { SP -= 2; WW(SS, SP, v); }
static inline uint16_t POP16(void) { uint16_t v = RW(SS, SP); SP += 2; return v; }
static inline void PUSH32(uint32_t v) { SP -= 4; WD(SS, SP, v); }
static inline uint32_t POP32(void) { uint32_t v = RD(SS, SP); SP += 4; return v; }

/* ---- flags ---- */
static inline int PF(void) {
    uint8_t v = cpu.pres;
    v ^= v >> 4; v ^= v >> 2; v ^= v >> 1;
    return !(v & 1);
}
#define SZP8(r)  (cpu.zf = (uint8_t)(r) == 0, cpu.sf = ((r) >> 7) & 1, cpu.pres = (uint8_t)(r))
#define SZP16(r) (cpu.zf = (uint16_t)(r) == 0, cpu.sf = ((r) >> 15) & 1, cpu.pres = (uint8_t)(r))
#define SZP32(r) (cpu.zf = (uint32_t)(r) == 0, cpu.sf = ((r) >> 31) & 1, cpu.pres = (uint8_t)(r))

#define DEF_ARITH(N, T, BITS, SZP)                                                        \
static inline T add##N(T a, T b) { T r = (T)(a + b); cpu.cf = r < a; cpu.of = ((a ^ r) & (b ^ r)) >> (BITS - 1) & 1; cpu.af = ((a ^ b ^ r) >> 4) & 1; SZP(r); return r; } \
static inline T adc##N(T a, T b) { T c = cpu.cf; T r = (T)(a + b + c); cpu.cf = c ? r <= a : r < a; cpu.of = ((a ^ r) & (b ^ r)) >> (BITS - 1) & 1; cpu.af = ((a ^ b ^ r) >> 4) & 1; SZP(r); return r; } \
static inline T sub##N(T a, T b) { T r = (T)(a - b); cpu.cf = a < b; cpu.of = ((a ^ b) & (a ^ r)) >> (BITS - 1) & 1; cpu.af = ((a ^ b ^ r) >> 4) & 1; SZP(r); return r; } \
static inline T sbb##N(T a, T b) { T c = cpu.cf; T r = (T)(a - b - c); cpu.cf = c ? a <= b : a < b; cpu.of = ((a ^ b) & (a ^ r)) >> (BITS - 1) & 1; cpu.af = ((a ^ b ^ r) >> 4) & 1; SZP(r); return r; } \
static inline T and##N(T a, T b) { T r = a & b; cpu.cf = cpu.of = 0; SZP(r); return r; } \
static inline T or##N(T a, T b)  { T r = a | b; cpu.cf = cpu.of = 0; SZP(r); return r; } \
static inline T xor##N(T a, T b) { T r = a ^ b; cpu.cf = cpu.of = 0; SZP(r); return r; } \
static inline T inc##N(T a) { T r = (T)(a + 1); cpu.of = r == ((T)1 << (BITS - 1)); cpu.af = (r & 0xF) == 0; SZP(r); return r; } \
static inline T dec##N(T a) { T r = (T)(a - 1); cpu.of = a == ((T)1 << (BITS - 1)); cpu.af = (a & 0xF) == 0; SZP(r); return r; } \
static inline T neg##N(T a) { T r = (T)(0 - a); cpu.cf = a != 0; cpu.of = a == ((T)1 << (BITS - 1)); SZP(r); return r; } \
static inline T shl##N(T a, unsigned c) { c &= 31; if (!c) return a; { T r = (T)(c > BITS ? 0 : (T)(a << (c - 1))); cpu.cf = r >> (BITS - 1) & 1; r = (T)(r << 1); cpu.of = (r >> (BITS - 1) & 1) ^ cpu.cf; SZP(r); return r; } } \
static inline T shr##N(T a, unsigned c) { c &= 31; if (!c) return a; { T r = (T)(c > BITS ? 0 : (T)(a >> (c - 1))); cpu.cf = r & 1; cpu.of = (a >> (BITS - 1)) & 1; r = (T)(r >> 1); SZP(r); return r; } } \
static inline T rol##N(T a, unsigned c) { c &= 31; if (!c) return a; c %= BITS; { T r = c ? (T)((a << c) | (a >> (BITS - c))) : a; cpu.cf = r & 1; cpu.of = (r >> (BITS - 1) & 1) ^ cpu.cf; return r; } } \
static inline T ror##N(T a, unsigned c) { c &= 31; if (!c) return a; c %= BITS; { T r = c ? (T)((a >> c) | (a << (BITS - c))) : a; cpu.cf = r >> (BITS - 1) & 1; cpu.of = ((r >> (BITS - 1)) ^ (r >> (BITS - 2))) & 1; return r; } } \
static inline T rcl##N(T a, unsigned c) { c &= 31; while (c--) { T nc = a >> (BITS - 1) & 1; a = (T)((a << 1) | cpu.cf); cpu.cf = (uint8_t)nc; } cpu.of = (a >> (BITS - 1) & 1) ^ cpu.cf; return a; } \
static inline T rcr##N(T a, unsigned c) { c &= 31; if (c) cpu.of = (a >> (BITS - 1) & 1) ^ cpu.cf; while (c--) { T nc = a & 1; a = (T)((a >> 1) | ((T)cpu.cf << (BITS - 1))); cpu.cf = (uint8_t)nc; } return a; }

DEF_ARITH(8, uint8_t, 8, SZP8)
DEF_ARITH(16, uint16_t, 16, SZP16)
DEF_ARITH(32, uint32_t, 32, SZP32)

static inline uint8_t sar8(uint8_t a, unsigned c) { c &= 31; if (!c) return a; { int8_t s = (int8_t)a; int r = c >= 8 ? (s < 0 ? -1 : 0) : s >> (c - 1); cpu.cf = r & 1; r >>= 1; cpu.of = 0; SZP8((uint8_t)r); return (uint8_t)r; } }
static inline uint16_t sar16(uint16_t a, unsigned c) { c &= 31; if (!c) return a; { int16_t s = (int16_t)a; int r = c >= 16 ? (s < 0 ? -1 : 0) : s >> (c - 1); cpu.cf = r & 1; r >>= 1; cpu.of = 0; SZP16((uint16_t)r); return (uint16_t)r; } }
static inline uint32_t sar32(uint32_t a, unsigned c) { c &= 31; if (!c) return a; { int32_t s = (int32_t)a; int64_t r = (int64_t)s >> (c - 1); cpu.cf = r & 1; r >>= 1; cpu.of = 0; SZP32((uint32_t)r); return (uint32_t)r; } }

static inline uint16_t shld16(uint16_t d, uint16_t s, unsigned c) { c &= 31; if (!c) return d; { uint32_t v = ((uint32_t)d << 16) | s; uint32_t r = v << c; cpu.cf = (v >> (32 - c)) & 1; d = (uint16_t)(r >> 16); SZP16(d); return d; } }
static inline uint16_t shrd16(uint16_t d, uint16_t s, unsigned c) { c &= 31; if (!c) return d; { uint32_t v = ((uint32_t)s << 16) | d; cpu.cf = (v >> (c - 1)) & 1; d = (uint16_t)(v >> c); SZP16(d); return d; } }
static inline uint32_t shld32(uint32_t d, uint32_t s, unsigned c) { c &= 31; if (!c) return d; { uint64_t v = ((uint64_t)d << 32) | s; cpu.cf = (v >> (64 - c)) & 1; d = (uint32_t)((v << c) >> 32); SZP32(d); return d; } }
static inline uint32_t shrd32(uint32_t d, uint32_t s, unsigned c) { c &= 31; if (!c) return d; { uint64_t v = ((uint64_t)s << 32) | d; cpu.cf = (v >> (c - 1)) & 1; d = (uint32_t)(v >> c); SZP32(d); return d; } }

static inline uint16_t get_flags(void) {
    return (uint16_t)(cpu.cf | 2 | (PF() << 2) | (cpu.af << 4) | (cpu.zf << 6) | (cpu.sf << 7) |
                      (cpu.ifl << 9) | (cpu.df << 10) | (cpu.of << 11));
}
static inline void set_flags(uint16_t f) {
    cpu.cf = f & 1; cpu.af = (f >> 4) & 1; cpu.zf = (f >> 6) & 1; cpu.sf = (f >> 7) & 1;
    cpu.ifl = (f >> 9) & 1; cpu.df = (f >> 10) & 1; cpu.of = (f >> 11) & 1;
    /* PF: choose a result byte with the requested parity */
    cpu.pres = ((f >> 2) & 1) ? 0 : 1;
}

/* ---- multiply / divide ---- */
void cpu_mul8(uint8_t v);
void cpu_imul8(uint8_t v);
void cpu_mul16(uint16_t v);
void cpu_imul16(uint16_t v);
void cpu_mul32(uint32_t v);
void cpu_imul32(uint32_t v);
void cpu_div8(uint8_t v);
void cpu_idiv8(uint8_t v);
void cpu_div16(uint16_t v);
void cpu_idiv16(uint16_t v);
void cpu_div32(uint32_t v);
void cpu_idiv32(uint32_t v);
uint16_t cpu_imul3_16(uint16_t a, uint16_t b);
uint32_t cpu_imul3_32(uint32_t a, uint32_t b);

/* ---- control transfer into the runtime ---- */
typedef void (*RecompFn)(void);
/* Call the function at sel:off. The caller has already pushed the return
 * address (exactly as the original instruction would). */
void call_far(uint16_t sel, uint16_t off);
/* Look up a recompiled function (NULL if none). */
RecompFn recomp_lookup(uint16_t sel, uint16_t off);
/* `jmp far` through memory. If the target is a function entry it is a tail
 * call; otherwise it is a hand-made return (CRT code that popped its own
 * return address and jumps back to it), so the C function just returns. */
int jmp_far(uint16_t sel, uint16_t off);
void cpu_int(uint8_t n);
void cpu_in_out(void);
_Noreturn void recomp_fatal(const char *fmt, ...);
void recomp_bad_jump(uint16_t sel, uint16_t from, uint16_t to);

#ifdef __cplusplus
}
#endif

#endif /* SB_RECOMP_CPU_H */
