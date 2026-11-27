/* x86 real/16-bit protected-mode instruction decoder for the static recompiler.
 *
 * Covers the 8086/186/286 integer set plus the 386 extensions a 1994 Win16
 * compiler emits (0x66 operand-size prefix, 0F xx: movzx/movsx/jcc near/setcc/
 * shld/shrd/bt* /imul r,rm/push fs,gs). x87 escape opcodes are decoded for
 * length only. Anything unknown is a decode error, so gaps are never silently
 * skipped.
 */
#ifndef SB_GEN_X86_H
#define SB_GEN_X86_H

#include <stddef.h>
#include <stdint.h>

enum { OPK_NONE, OPK_REG, OPK_SREG, OPK_IMM, OPK_MEM };

/* Segment registers, in encoding order. */
enum { SR_ES, SR_CS, SR_SS, SR_DS, SR_FS, SR_GS };

typedef struct X86Op {
    uint8_t kind;
    uint8_t size;         /* reg/imm: 1, 2, 4; mem: access size (0 = n/a, e.g. lea) */
    uint8_t idx;          /* reg or segment register index */
    uint8_t seg;          /* mem: effective segment register */
    uint8_t nbase;        /* mem: number of base registers */
    uint8_t base[2];      /* mem: 16-bit register indices summed */
    uint8_t explicit_seg; /* mem: a segment prefix was given */
    uint16_t disp;        /* mem: 16-bit displacement */
    uint32_t val;         /* imm */
} X86Op;

#define X86_MNEMONICS(X) \
    X(ADD, "add") X(OR, "or") X(ADC, "adc") X(SBB, "sbb") X(AND, "and") X(SUB, "sub") X(XOR, "xor") \
    X(CMP, "cmp") X(PUSH, "push") X(POP, "pop") X(DAA, "daa") X(DAS, "das") X(AAA, "aaa") \
    X(AAS, "aas") X(INC, "inc") X(DEC, "dec") X(PUSHA, "pusha") X(POPA, "popa") X(BOUND, "bound") \
    X(IMUL3, "imul3") X(INS, "ins") X(OUTS, "outs") X(JCC, "jcc") X(TEST, "test") X(XCHG, "xchg") \
    X(MOV, "mov") X(LEA, "lea") X(NOP, "nop") X(CBW, "cbw") X(CWDE, "cwde") X(CWD, "cwd") \
    X(CDQ, "cdq") X(CALLF, "callf") X(WAIT, "wait") X(PUSHF, "pushf") X(POPF, "popf") \
    X(SAHF, "sahf") X(LAHF, "lahf") X(MOVS, "movs") X(CMPS, "cmps") X(STOS, "stos") \
    X(LODS, "lods") X(SCAS, "scas") X(ROL, "rol") X(ROR, "ror") X(RCL, "rcl") X(RCR, "rcr") \
    X(SHL, "shl") X(SHR, "shr") X(SAL, "sal") X(SAR, "sar") X(RET, "ret") X(LES, "les") \
    X(LDS, "lds") X(ENTER, "enter") X(LEAVE, "leave") X(RETF, "retf") X(INT, "int") \
    X(INTO, "into") X(IRET, "iret") X(AAM, "aam") X(AAD, "aad") X(XLAT, "xlat") X(FPU, "fpu") \
    X(LOOPNE, "loopne") X(LOOPE, "loope") X(LOOP, "loop") X(JCXZ, "jcxz") X(IN, "in") \
    X(OUT, "out") X(CALL, "call") X(JMP, "jmp") X(JMPF, "jmpf") X(HLT, "hlt") X(CMC, "cmc") \
    X(NOT, "not") X(NEG, "neg") X(MUL, "mul") X(IMUL, "imul") X(DIV, "div") X(IDIV, "idiv") \
    X(CLC, "clc") X(STC, "stc") X(CLI, "cli") X(STI, "sti") X(CLD, "cld") X(STD, "std") \
    X(CALL_IND, "call_ind") X(CALLF_IND, "callf_ind") X(JMP_IND, "jmp_ind") \
    X(JMPF_IND, "jmpf_ind") X(SETCC, "setcc") X(MOVZX, "movzx") X(MOVSX, "movsx") \
    X(IMUL2, "imul2") X(SHLD, "shld") X(SHRD, "shrd") X(BT, "bt") X(BTS, "bts") X(BTR, "btr") \
    X(BTC, "btc") X(LSS, "lss") X(LFS, "lfs") X(LGS, "lgs")

enum X86Mnemonic {
#define X86_ENUM(id, name) X86_##id,
    X86_MNEMONICS(X86_ENUM)
#undef X86_ENUM
    X86_COUNT
};

enum { REP_NONE, REP_REP, REP_REPE, REP_REPNE };

typedef struct X86Instr {
    uint32_t addr;
    uint8_t length;
    uint8_t mnem;
    uint8_t nops;
    uint8_t rep;
    uint8_t opsize;      /* operand size for string ops etc. */
    int8_t cc;           /* condition code for jcc/setcc, -1 = none */
    uint8_t src_seg;     /* string ops and xlat: source segment register */
    uint8_t has_far;     /* direct far call/jmp */
    int8_t imm_off;      /* offset in the instruction of the 16-bit immediate, -1 = none */
    int8_t disp_off;     /* offset in the instruction of the 16-bit displacement, -1 = none */
    int32_t target;      /* near branch target, -1 = none */
    uint16_t far_seg, far_off;
    X86Op ops[3];
} X86Instr;

extern const char *const x86_mnemonic_names[X86_COUNT];
extern const char *const x86_reg8[8], *const x86_reg16[8], *const x86_reg32[8];
extern const char *const x86_sreg[6], *const x86_cc[16];

/* Decodes the instruction at addr. Returns 0, or -1 with a message in err. */
int x86_decode(const uint8_t *code, size_t len, uint32_t addr, X86Instr *out, char *err, size_t errcap);

/* The instruction as text, e.g. "mov ax, ds:[bx+0x4]". */
void x86_format(const X86Instr *ins, char *buf, size_t cap);

int x86_op_equal(const X86Op *a, const X86Op *b);

#endif
