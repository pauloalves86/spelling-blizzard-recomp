/* Runtime services shared by the recompiled program and the Win16 API layer. */
#ifndef SB_RECOMP_RUNTIME_H
#define SB_RECOMP_RUNTIME_H

#include <stdio.h>
#include "cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Fixed selectors. EXE segment N -> 0x1000 + 8*(N-1) (Ghidra's numbering). */
#define EXE_SEL(n) ((uint16_t)(0x1000u + 8u * ((n) - 1u)))
#define DGROUP_SEL 0x1060u  /* segment 13, automatic data (DS == SS) */
#define THUNK_SEL 0x10A0u   /* far pointers to imported APIs: THUNK_SEL:(index*4) */

/* The recompiled program: every function, sorted by address, and the NE
 * entry point. */
typedef struct RecompProgram {
    const RecompEntry *table;
    unsigned count;
    RecompFn start;
} RecompProgram;
extern RecompProgram recomp_program;
/* Provides the program for the EXE at exe_path: either compiled into this
 * executable ahead of time (aot.c) or recompiled and compiled now, in memory
 * (jit.c). Returns 0, or -1 with a message in err. */
int recomp_load_program(const char *exe_path, char *err, size_t errcap);
/* Checks that compiling in memory works on this machine (jit.c). */
int recomp_self_test(void);

extern int g_recomp_trace;

void recomp_init_memory(void);
void recomp_dump_stack(FILE *fp);
void recomp_callback(uint16_t sel, uint16_t off);
int recomp_import_index(const char *module, int ordinal);
const char *recomp_import_name(unsigned idx);

/* Selectors and the Win16 global heap. Handle == first selector. */
uint16_t sel_alloc(unsigned count);
void sel_set(uint16_t sel, uint32_t base, uint32_t limit);
void sel_free(uint16_t sel, unsigned count);
uint16_t global_alloc(uint16_t flags, uint32_t size);
int global_free(uint16_t sel);
uint32_t global_size(uint16_t sel);
uint16_t global_realloc(uint16_t sel, uint32_t size);
uint16_t global_handle(uint16_t sel);

/* DOS services (INT 21h / DOS3CALL). */
void dos_int21(void);

/* Argument access for Win16 API implementations. When an API runs, the
 * caller has pushed its arguments and then the far return address, so the
 * last-pushed argument is at SS:SP+4. */
typedef struct ArgReader {
    uint16_t p;
} ArgReader;
/* Pascal: arguments pushed left to right; read them in declaration order. */
static inline ArgReader args_pascal(unsigned argbytes) { ArgReader a; a.p = (uint16_t)(SP + 4 + argbytes); return a; }
static inline uint16_t arg_w(ArgReader *a) { a->p -= 2; return RW(SS, a->p); }
static inline int16_t arg_i(ArgReader *a) { return (int16_t)arg_w(a); }
static inline uint32_t arg_d(ArgReader *a) { a->p -= 4; return RD(SS, a->p); }
/* cdecl (wsprintf): arguments pushed right to left; read upward. */
static inline ArgReader args_cdecl(void) { ArgReader a; a.p = (uint16_t)(SP + 4); return a; }
static inline uint16_t carg_w(ArgReader *a) { uint16_t v = RW(SS, a->p); a->p += 2; return v; }
static inline uint32_t carg_d(ArgReader *a) { uint32_t v = RD(SS, a->p); a->p += 4; return v; }
/* Return from a Pascal API: pop return address and arguments. */
static inline void api_ret(unsigned argbytes) { SP = (uint16_t)(SP + 4 + argbytes); }
static inline void api_ret_cdecl(void) { SP = (uint16_t)(SP + 4); }
static inline void ret_w(uint16_t v) { AX = v; }
static inline void ret_d(uint32_t v) { AX = (uint16_t)v; DX = (uint16_t)(v >> 16); }

/* Far pointer (as a 32-bit sel:off value) to host pointer; NULL for 0:0. */
static inline void *farp(uint32_t fp) { return fp ? FARPTR((uint16_t)(fp >> 16), (uint16_t)fp) : NULL; }
static inline uint32_t mkfar(uint16_t sel, uint16_t off) { return ((uint32_t)sel << 16) | off; }

#ifdef __cplusplus
}
#endif

#endif /* SB_RECOMP_RUNTIME_H */
