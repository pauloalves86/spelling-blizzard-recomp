/* Static-recompilation runtime core: memory, selectors, NE loader, dispatch.
 */
#include "runtime.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "recomp_gen.h"

CPU cpu;
uint8_t *MEM;
uint32_t SEGBASE[MAX_SELECTORS];
uint32_t SEGLIMIT[MAX_SELECTORS];

int g_recomp_trace = 0;

/* ------------------------------------------------------------ diagnostics */

_Noreturn void recomp_fatal(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "\n*** recomp fatal: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n    AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x SP=%04x DS=%04x ES=%04x SS=%04x\n",
            AX, BX, CX, DX, SI, DI, BP, SP, DS, ES, SS);
    recomp_dump_stack(stderr);
    fflush(stderr);
    abort();
}

void recomp_dump_stack(FILE *fp)
{
    /* Walk the BP chain. Far frames save an odd BP (Win16 `inc bp` prologue). */
    uint16_t bp = BP;
    int n;
    fprintf(fp, "    call stack (BP chain, innermost first):\n");
    for (n = 0; n < 32 && bp != 0 && bp < 0xFFF0; ++n) {
        uint16_t saved = RW(SS, bp);
        uint16_t ip = RW(SS, (uint16_t)(bp + 2));
        if (saved & 1) {
            fprintf(fp, "      return to %04x:%04x\n", RW(SS, (uint16_t)(bp + 4)), ip);
        } else {
            fprintf(fp, "      return to (near) %04x\n", ip);
        }
        bp = saved & ~1u;
    }
}

void recomp_bad_jump(uint16_t sel, uint16_t from, uint16_t to)
{
    recomp_fatal("indirect jump at %04x:%04x to unknown target %04x", sel, from, to);
}

/* ------------------------------------------------------------ dispatch */

typedef struct ImportEntry {
    const char *module;
    const char *name;
    int ordinal;
    int purge;
    void (*fn)(void);
} ImportEntry;

#define X(idx, mod, name, ord, purge) {#mod, #name, ord, purge, api_##mod##_##name},
static const ImportEntry g_imports[] = {
#include "gen_import_list.h"
};
#undef X
static const unsigned g_import_count = sizeof(g_imports) / sizeof(g_imports[0]);

int recomp_import_index(const char *module, int ordinal)
{
    unsigned i;
    for (i = 0; i < g_import_count; ++i) {
        if (g_imports[i].ordinal == ordinal && strcmp(g_imports[i].module, module) == 0) {
            return (int)i;
        }
    }
    return -1;
}

const char *recomp_import_name(unsigned idx)
{
    return idx < g_import_count ? g_imports[idx].name : "?";
}

RecompFn recomp_lookup(uint16_t sel, uint16_t off)
{
    unsigned lo = 0, hi = recomp_table_count;
    uint32_t key = ((uint32_t)sel << 16) | off;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        uint32_t k = ((uint32_t)recomp_table[mid].sel << 16) | recomp_table[mid].off;
        if (k == key) {
            return recomp_table[mid].fn;
        }
        if (k < key) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return NULL;
}

void call_far(uint16_t sel, uint16_t off)
{
    RecompFn fn;
    if (sel == THUNK_SEL) {
        unsigned idx = off / 4u;
        if (idx >= g_import_count) {
            recomp_fatal("call to bad import thunk %04x:%04x", sel, off);
        }
        g_imports[idx].fn();
        return;
    }
    fn = recomp_lookup(sel, off);
    if (!fn) {
        recomp_fatal("call to %04x:%04x: no recompiled function there "
                     "(add it to tools/recomp/extra_entries.txt)", sel, off);
    }
    fn();
}

int jmp_far(uint16_t sel, uint16_t off)
{
    RecompFn fn = sel == THUNK_SEL ? NULL : recomp_lookup(sel, off);
    if (sel == THUNK_SEL) {
        call_far(sel, off);
        return 1;
    }
    if (fn) {
        fn();
        return 1;
    }
    if (g_recomp_trace) {
        fprintf(stderr, "[recomp] jmp far %04x:%04x treated as return\n", sel, off);
    }
    return 0;
}

/* Call into recompiled code from the runtime (window procedures, callbacks).
 * Arguments must already be pushed. A sentinel return address is pushed so
 * the callee's `retf` pops it, exactly as if Windows had called it. */
void recomp_callback(uint16_t sel, uint16_t off)
{
    uint16_t saved_ds = DS, saved_es = ES;
    uint16_t saved_si = SI, saved_di = DI, saved_bp = BP;
    PUSH16(THUNK_SEL);
    PUSH16(0xFFFF);
    /* Win16 exported-function prologue expects AX = DS of the instance. */
    AX = DGROUP_SEL;
    DS = DGROUP_SEL;
    call_far(sel, off);
    DS = saved_ds;
    ES = saved_es;
    SI = saved_si;
    DI = saved_di;
    BP = saved_bp;
}

/* ------------------------------------------------------------ arithmetic */

static void divide_error(void) { recomp_fatal("divide error"); }

void cpu_mul8(uint8_t v) { AX = (uint16_t)(AL * v); cpu.cf = cpu.of = AH != 0; }
void cpu_imul8(uint8_t v) { int16_t r = (int16_t)((int8_t)AL * (int8_t)v); AX = (uint16_t)r; cpu.cf = cpu.of = r != (int8_t)r; }
void cpu_mul16(uint16_t v) { uint32_t r = (uint32_t)AX * v; AX = (uint16_t)r; DX = (uint16_t)(r >> 16); cpu.cf = cpu.of = DX != 0; }
void cpu_imul16(uint16_t v) { int32_t r = (int32_t)(int16_t)AX * (int16_t)v; AX = (uint16_t)r; DX = (uint16_t)((uint32_t)r >> 16); cpu.cf = cpu.of = r != (int16_t)r; }
void cpu_mul32(uint32_t v) { uint64_t r = (uint64_t)EAX * v; EAX = (uint32_t)r; EDX = (uint32_t)(r >> 32); cpu.cf = cpu.of = EDX != 0; }
void cpu_imul32(uint32_t v) { int64_t r = (int64_t)(int32_t)EAX * (int32_t)v; EAX = (uint32_t)r; EDX = (uint32_t)((uint64_t)r >> 32); cpu.cf = cpu.of = r != (int32_t)r; }
void cpu_div8(uint8_t v) { uint16_t q; if (!v) divide_error(); q = AX / v; if (q > 0xFF) divide_error(); AH = (uint8_t)(AX % v); AL = (uint8_t)q; }
void cpu_idiv8(uint8_t v) { int16_t a = (int16_t)AX, q; if (!v) divide_error(); q = (int16_t)(a / (int8_t)v); if (q != (int8_t)q) divide_error(); AH = (uint8_t)(int8_t)(a % (int8_t)v); AL = (uint8_t)q; }
void cpu_div16(uint16_t v) { uint32_t a = ((uint32_t)DX << 16) | AX, q; if (!v) divide_error(); q = a / v; if (q > 0xFFFF) divide_error(); DX = (uint16_t)(a % v); AX = (uint16_t)q; }
void cpu_idiv16(uint16_t v) { int32_t a = (int32_t)(((uint32_t)DX << 16) | AX), q; if (!v) divide_error(); q = a / (int16_t)v; if (q != (int16_t)q) divide_error(); DX = (uint16_t)(a % (int16_t)v); AX = (uint16_t)q; }
void cpu_div32(uint32_t v) { uint64_t a = ((uint64_t)EDX << 32) | EAX, q; if (!v) divide_error(); q = a / v; if (q > 0xFFFFFFFFu) divide_error(); EDX = (uint32_t)(a % v); EAX = (uint32_t)q; }
void cpu_idiv32(uint32_t v) { int64_t a = (int64_t)(((uint64_t)EDX << 32) | EAX), q; if (!v) divide_error(); q = a / (int32_t)v; if (q != (int32_t)q) divide_error(); EDX = (uint32_t)(a % (int32_t)v); EAX = (uint32_t)q; }
uint16_t cpu_imul3_16(uint16_t a, uint16_t b) { int32_t r = (int32_t)(int16_t)a * (int16_t)b; cpu.cf = cpu.of = r != (int16_t)r; return (uint16_t)r; }
uint32_t cpu_imul3_32(uint32_t a, uint32_t b) { int64_t r = (int64_t)(int32_t)a * (int32_t)b; cpu.cf = cpu.of = r != (int32_t)r; return (uint32_t)r; }

void cpu_in_out(void) { recomp_fatal("port I/O instruction"); }

void cpu_int(uint8_t n)
{
    if (n == 0x21) {
        dos_int21();
        return;
    }
    recomp_fatal("int %02xh", n);
}

/* ------------------------------------------------------------ selectors & global heap */

static uint8_t g_sel_used[MAX_SELECTORS];

typedef struct Block {
    uint32_t base;
    uint32_t size;
    uint16_t sel;      /* first selector; also the handle */
    uint16_t nsel;
    uint16_t flags;
    uint16_t lock;
    int used;
} Block;

#define MAX_BLOCKS 16384
static Block g_blocks[MAX_BLOCKS];
static int g_nblocks;
#define HEAP_START (1u << 20)

uint16_t sel_alloc(unsigned count)
{
    unsigned i, k;
    for (i = 0x400; i + count <= MAX_SELECTORS; ++i) {
        for (k = 0; k < count && !g_sel_used[i + k]; ++k) {
        }
        if (k == count) {
            for (k = 0; k < count; ++k) {
                g_sel_used[i + k] = 1;
            }
            return (uint16_t)(i << 3);
        }
        i += k;
    }
    recomp_fatal("out of selectors");
}

void sel_set(uint16_t sel, uint32_t base, uint32_t limit)
{
    SEGBASE[sel >> 3] = base;
    SEGLIMIT[sel >> 3] = limit;
    g_sel_used[sel >> 3] = 1;
}

void sel_free(uint16_t sel, unsigned count)
{
    unsigned k;
    for (k = 0; k < count; ++k) {
        g_sel_used[(sel >> 3) + k] = 0;
        SEGBASE[(sel >> 3) + k] = 0;
    }
}

static int cmp_block(const void *a, const void *b)
{
    const Block *x = a, *y = b;
    return (x->base > y->base) - (x->base < y->base);
}

static uint32_t heap_find(uint32_t size)
{
    uint32_t at = HEAP_START;
    int i;
    for (i = 0; i < g_nblocks; ++i) {
        if (g_blocks[i].base >= at + size) {
            return at;
        }
        if (g_blocks[i].base + g_blocks[i].size > at) {
            at = (g_blocks[i].base + g_blocks[i].size + 15u) & ~15u;
        }
    }
    if (at + size > MEM_SIZE) {
        recomp_fatal("out of emulated memory (%u bytes requested)", size);
    }
    return at;
}

static Block *block_by_sel(uint16_t sel)
{
    int i;
    sel &= ~7u;
    for (i = 0; i < g_nblocks; ++i) {
        if (sel >= g_blocks[i].sel && sel < g_blocks[i].sel + 8u * g_blocks[i].nsel) {
            return &g_blocks[i];
        }
    }
    return NULL;
}

static void map_block(Block *b)
{
    unsigned k;
    for (k = 0; k < b->nsel; ++k) {
        uint32_t remain = b->size - k * 0x10000u;
        sel_set((uint16_t)(b->sel + 8 * k), b->base + k * 0x10000u, remain - 1);
    }
}

uint16_t global_alloc(uint16_t flags, uint32_t size)
{
    Block b;
    uint32_t sz = size ? size : 1;
    sz = (sz + 15u) & ~15u;
    if (g_nblocks >= MAX_BLOCKS) {
        return 0;
    }
    memset(&b, 0, sizeof(b));
    b.base = heap_find(sz);
    b.size = sz;
    b.nsel = (uint16_t)((sz + 0xFFFFu) / 0x10000u);
    b.sel = sel_alloc(b.nsel);
    b.flags = flags;
    b.used = 1;
    memset(MEM + b.base, 0, sz); /* GMEM_ZEROINIT or not: zeroing is safe */
    map_block(&b);
    g_blocks[g_nblocks++] = b;
    qsort(g_blocks, (size_t)g_nblocks, sizeof(Block), cmp_block);
    return b.sel;
}

int global_free(uint16_t sel)
{
    Block *b = block_by_sel(sel);
    if (!b) {
        return 0;
    }
    sel_free(b->sel, b->nsel);
    *b = g_blocks[--g_nblocks];
    qsort(g_blocks, (size_t)g_nblocks, sizeof(Block), cmp_block);
    return 1;
}

uint32_t global_size(uint16_t sel)
{
    Block *b = block_by_sel(sel);
    return b ? b->size : 0;
}

uint16_t global_realloc(uint16_t sel, uint32_t size)
{
    Block *b = block_by_sel(sel);
    uint32_t sz = (size ? size : 1);
    uint32_t newbase;
    uint16_t nsel;
    Block old;
    if (!b) {
        return 0;
    }
    sz = (sz + 15u) & ~15u;
    nsel = (uint16_t)((sz + 0xFFFFu) / 0x10000u);
    old = *b;
    /* remove, find space, copy */
    *b = g_blocks[--g_nblocks];
    qsort(g_blocks, (size_t)g_nblocks, sizeof(Block), cmp_block);
    newbase = heap_find(sz);
    memmove(MEM + newbase, MEM + old.base, old.size < sz ? old.size : sz);
    if (sz > old.size) {
        memset(MEM + newbase + old.size, 0, sz - old.size);
    }
    old.base = newbase;
    old.size = sz;
    if (nsel > old.nsel) {
        unsigned k;
        int ok = 1;
        for (k = old.nsel; k < nsel; ++k) {
            if (((old.sel >> 3) + k) >= MAX_SELECTORS || g_sel_used[(old.sel >> 3) + k]) {
                ok = 0;
            }
        }
        if (!ok) {
            sel_free(old.sel, old.nsel);
            old.sel = sel_alloc(nsel);
        }
    } else if (nsel < old.nsel) {
        sel_free((uint16_t)(old.sel + 8 * nsel), (unsigned)(old.nsel - nsel));
    }
    old.nsel = nsel;
    map_block(&old);
    g_blocks[g_nblocks++] = old;
    qsort(g_blocks, (size_t)g_nblocks, sizeof(Block), cmp_block);
    return old.sel;
}

uint16_t global_handle(uint16_t sel)
{
    Block *b = block_by_sel(sel);
    return b ? b->sel : 0;
}

/* ------------------------------------------------------------ init */

void recomp_init_memory(void)
{
    MEM = (uint8_t *)calloc(1, MEM_SIZE);
    if (!MEM) {
        fprintf(stderr, "cannot allocate emulated memory\n");
        exit(1);
    }
    memset(SEGBASE, 0, sizeof(SEGBASE));
    memset(g_sel_used, 0, sizeof(g_sel_used));
    g_sel_used[0] = 1;
    /* Selector 0 maps to a zero page so null-pointer reads are harmless and
     * visible; writes there are detected by recomp_check_null_page(). */
    SEGBASE[0] = 0;
    g_nblocks = 0;
}
