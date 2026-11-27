/* The program compiled now, in memory: the player's BLIZZARD.EXE is
 * recompiled to C (src/recomp/gen) and the C is compiled by TinyCC (libtcc)
 * into executable memory, in a fraction of a second. Nothing from the game is
 * written to disk, and nothing from it ships with this program. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "api.h"
#include "gen.h"
#include "libtcc.h"
#include "runtime.h"

/* jit_prelude.h, cpu.h and api_decls.h, and jit_selftest.h, built in (CMake). */
extern const char sb_jit_prelude[];
extern const char sb_jit_selftest[];

/* The self-test's code, compiled here by the host compiler too. */
#include "jit_selftest.h"

typedef struct Sym {
    const char *name;
    const void *addr;
} Sym;

/* Everything the generated code may refer to outside itself. */
static const Sym runtime_syms[] = {
    {"cpu", &cpu},
    {"MEM", &MEM},
    {"SEGBASE", SEGBASE},
    {"SEGLIMIT", SEGLIMIT},
    {"cpu_mul8", (const void *)cpu_mul8},
    {"cpu_imul8", (const void *)cpu_imul8},
    {"cpu_mul16", (const void *)cpu_mul16},
    {"cpu_imul16", (const void *)cpu_imul16},
    {"cpu_mul32", (const void *)cpu_mul32},
    {"cpu_imul32", (const void *)cpu_imul32},
    {"cpu_div8", (const void *)cpu_div8},
    {"cpu_idiv8", (const void *)cpu_idiv8},
    {"cpu_div16", (const void *)cpu_div16},
    {"cpu_idiv16", (const void *)cpu_idiv16},
    {"cpu_div32", (const void *)cpu_div32},
    {"cpu_idiv32", (const void *)cpu_idiv32},
    {"cpu_imul3_16", (const void *)cpu_imul3_16},
    {"cpu_imul3_32", (const void *)cpu_imul3_32},
    {"call_far", (const void *)call_far},
    {"recomp_lookup", (const void *)recomp_lookup},
    {"jmp_far", (const void *)jmp_far},
    {"cpu_int", (const void *)cpu_int},
    {"cpu_in_out", (const void *)cpu_in_out},
    {"recomp_fatal", (const void *)recomp_fatal},
    {"recomp_bad_jump", (const void *)recomp_bad_jump},
    {"memcpy", (const void *)memcpy},
    {"memmove", (const void *)memmove},
    {"memset", (const void *)memset},
#define X(idx, mod, name, ord, purge) {"api_" #mod "_" #name, (const void *)api_##mod##_##name},
#include "import_list.h"
#undef X
};

typedef struct Src {
    char *p;
    size_t n, cap;
} Src;

static int src_put(Src *s, const char *text, size_t len)
{
    if (s->n + len + 1 > s->cap) {
        size_t cap = s->cap ? s->cap : 1 << 20;
        while (s->n + len + 1 > cap) cap *= 2;
        char *p = realloc(s->p, cap);
        if (!p) return -1;
        s->p = p;
        s->cap = cap;
    }
    memcpy(s->p + s->n, text, len);
    s->n += len;
    s->p[s->n] = 0;
    return 0;
}

/* Appends text without its #include lines (everything they would bring in is
 * already in the prelude). */
static int src_add(Src *s, const char *text, size_t len)
{
    const char *p = text, *end = text + len;
    while (p < end) {
        const char *eol = memchr(p, '\n', (size_t)(end - p));
        const char *next = eol ? eol + 1 : end;
        if (!((size_t)(next - p) >= 8 && !memcmp(p, "#include", 8)) && src_put(s, p, (size_t)(next - p)))
            return -1;
        p = next;
    }
    return src_put(s, "\n", 1);
}

typedef struct ErrCtx {
    char *err;
    size_t errcap;
    int set;
} ErrCtx;

static void tcc_error(void *opaque, const char *msg)
{
    ErrCtx *e = opaque;
    if (!e->set) snprintf(e->err, e->errcap, "%s", msg);
    e->set = 1;
}

static double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static char *read_file(const char *path, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    char *buf = NULL;
    size_t n = 0, cap = 0, r;
    do {
        if (n == cap) {
            cap = cap ? cap * 2 : 1 << 18;
            char *nb = realloc(buf, cap);
            if (!nb) {
                free(buf);
                fclose(fp);
                return NULL;
            }
            buf = nb;
        }
        r = fread(buf + n, 1, cap - n, fp);
        n += r;
    } while (r);
    fclose(fp);
    *len = n;
    return buf;
}

/* The compiled code lives as long as the program. */
static TCCState *g_tcc;

/* Compiles src into executable memory; NULL (with the message in e) on error. */
static TCCState *compile(const char *src, ErrCtx *e)
{
    TCCState *s = tcc_new();
    if (!s) {
        snprintf(e->err, e->errcap, "can't start the compiler");
        return NULL;
    }
    tcc_set_error_func(s, e, tcc_error);
    tcc_set_options(s, "-nostdinc -nostdlib -w");
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    for (size_t k = 0; k < sizeof runtime_syms / sizeof runtime_syms[0]; k++)
        tcc_add_symbol(s, runtime_syms[k].name, runtime_syms[k].addr);
    if (tcc_compile_string(s, src) || tcc_relocate(s)) {
        if (!e->set) snprintf(e->err, e->errcap, "compiling failed");
        tcc_delete(s);
        return NULL;
    }
    return s;
}

int recomp_load_program(const char *exe_path, char *err, size_t errcap)
{
    double t0 = now_ms();
    size_t len;
    char *exe = read_file(exe_path, &len);
    if (!exe) {
        snprintf(err, errcap, "can't read the file");
        return -1;
    }
    GenOutput out;
    int rc = gen_recompile((const uint8_t *)exe, len, NULL, &out, err, errcap);
    free(exe);
    if (rc) return -1;
    double t1 = now_ms();

    /* One translation unit: prelude, prototypes, function bodies, table. */
    Src src = {0};
    rc = src_add(&src, sb_jit_prelude, strlen(sb_jit_prelude));
    for (int k = 0; k < out.nfiles && !rc; k++) rc = src_add(&src, out.files[k].data, out.files[k].len);
    unsigned nfuncs = (unsigned)out.nfuncs;
    gen_output_free(&out);
    if (rc) {
        free(src.p);
        snprintf(err, errcap, "out of memory");
        return -1;
    }

    ErrCtx ectx = {err, errcap, 0};
    TCCState *s = compile(src.p, &ectx);
    free(src.p);
    if (!s) return -1;
    const RecompEntry *table = tcc_get_symbol(s, "recomp_table");
    const unsigned *count = tcc_get_symbol(s, "recomp_table_count");
    RecompFn start = (RecompFn)tcc_get_symbol(s, "recomp_start");
    if (!table || !count || !start || *count != nfuncs) {
        snprintf(err, errcap, "the compiled code has no function table");
        tcc_delete(s);
        return -1;
    }
    g_tcc = s;
    recomp_program.table = table;
    recomp_program.count = *count;
    recomp_program.start = start;
    fprintf(stderr, "[recomp] recompiled %u functions in %.0f ms (C generated in %.0f ms)\n", nfuncs, now_ms() - t0,
            t1 - t0);
    return 0;
}

int recomp_self_test(void)
{
    char err[512];
    ErrCtx ectx = {err, sizeof err, 0};
    Src src = {0};
    if (src_add(&src, sb_jit_prelude, strlen(sb_jit_prelude)) ||
        src_add(&src, sb_jit_selftest, strlen(sb_jit_selftest))) {
        fprintf(stderr, "self-test: out of memory\n");
        return -1;
    }
    double t0 = now_ms();
    TCCState *s = compile(src.p, &ectx);
    free(src.p);
    if (!s) {
        fprintf(stderr, "self-test: compiling in memory failed: %s\n", err);
        return -1;
    }
    uint32_t (*jit_mix)(uint32_t, uint32_t) = (uint32_t (*)(uint32_t, uint32_t))tcc_get_symbol(s, "selftest_mix");
    if (!jit_mix) {
        fprintf(stderr, "self-test: no selftest_mix in the compiled code\n");
        tcc_delete(s);
        return -1;
    }
    /* a scratch 64 KB at selector 0x10 for the memory and stack checks */
    uint8_t *saved_mem = MEM, *scratch = calloc(1, 0x10000);
    uint32_t saved_base = SEGBASE[2];
    CPU saved_cpu = cpu;
    if (!scratch) {
        tcc_delete(s);
        return -1;
    }
    MEM = scratch;
    SEGBASE[2] = 0;
    int bad = 0;
    uint32_t a = 0x12345678u, b = 0x9abcdef0u, sum = 0;
    for (int k = 0; k < 20000 && !bad; k++) {
        a = a * 1664525u + 1013904223u;
        b = b ^ (b << 13);
        b ^= b >> 17;
        b ^= b << 5;
        memset(&cpu, 0, sizeof cpu);
        uint32_t want = selftest_mix(a, b);
        CPU cpu_want = cpu;
        memset(&cpu, 0, sizeof cpu);
        uint32_t got = jit_mix(a, b);
        if (got != want || memcmp(&cpu, &cpu_want, sizeof cpu)) {
            fprintf(stderr, "self-test: different result for (0x%08x, 0x%08x): 0x%08x compiled in memory, 0x%08x by "
                            "the host compiler\n", a, b, got, want);
            bad = 1;
        }
        sum ^= got;
    }
    MEM = saved_mem;
    SEGBASE[2] = saved_base;
    cpu = saved_cpu;
    free(scratch);
    tcc_delete(s);
    if (!bad)
        printf("self-test: compiling in memory works (%.0f ms to compile, 20000 checks, 0x%08x)\n", now_ms() - t0,
               sum);
    return bad ? -1 : 0;
}
