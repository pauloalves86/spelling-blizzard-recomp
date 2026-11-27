/* Static recompiler: see gen.h. */
#include "gen.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exports.h"
#include "ne.h"
#include "x86.h"

/* ------------------------------------------------------------ basics */

typedef struct G G;

typedef struct Buf {
    char *p;
    size_t n, cap;
} Buf;

/* uint32 -> int32 hash map (also used as a set). */
#define MAP_EMPTY 0xFFFFFFFFu
typedef struct Map {
    uint32_t *k;
    int32_t *v;
    uint32_t cap, n;
} Map;

typedef struct Ins {
    X86Instr i;
    char *bad;  /* decode error, or NULL */
} Ins;

typedef struct Func {
    int seg;
    uint32_t entry;
    Ins *ins;
    int nins, capins;
    Map at;       /* address -> index in ins */
    Map labels;   /* addresses that need a label */
    Map tail;     /* jmp targets that are other functions (tail calls) */
    Map tables;   /* address of a jmp_ind -> index in tab */
    uint16_t **tab;
    int *tabn;
    int ntab, captab;
    uint32_t *calls;  /* (seg << 16) | off of direct call targets */
    int ncalls, capcalls;
} Func;

enum { RK_SEL = 1, RK_FARPTR, RK_OFF, RK_IMPORT, RK_IMPORT_OFF, RK_IMPORT_SEL };
typedef struct Rel {
    uint8_t kind;
    uint16_t a, b;  /* sel/farptr/off: segment, offset; imports: import index */
} Rel;

typedef struct Import {
    const char *module;
    int ordinal;
    const char *export_name;  /* the export's name when known */
    int purge;
    int known;                /* found in the export table */
    int used;                 /* called directly by generated code */
    char fallback[48];        /* MODULE_ORDINAL otherwise */
} Import;

static const char *imp_name(const Import *im) { return im->known ? im->export_name : im->fallback; }

struct G {
    jmp_buf jb;
    char *err;
    size_t errcap;
    NEFile ne;
    int nseg;
    uint8_t *is_code;     /* [segnum] */
    uint8_t **entries;    /* [segnum] -> bitmap of 0x10000 offsets */
    Map reloc;            /* (seg << 16) | location -> index in rels */
    Rel *rels;
    int nrels, caprels;
    Import *imports;
    int nimports, capimports;
    Map done;             /* (seg << 16) | entry -> index in funcs */
    Func **funcs;         /* first-pass functions */
    int nfuncs, capfuncs;
    Map new_entries;
    GenOutput *out;
    int capfiles;
    /* Temporaries, owned here so that an error (a longjmp) frees them too. */
    Func *cur;              /* function being decoded */
    uint32_t *work;         /* its work list */
    int capwork;
    Map found, good;        /* pointer-discovered entries */
    uint8_t **inside;       /* [segnum] -> bytes inside decoded instructions */
    uint32_t *keys;         /* sorted map keys */
    const Ins **sorted;     /* a function's instructions by address */
    uint32_t *sw;           /* switch targets */
    Buf buf;
};

__attribute__((noreturn, format(printf, 2, 3))) static void fail(G *g, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g->err, g->errcap, fmt, ap);
    va_end(ap);
    longjmp(g->jb, 1);
}

static void *xrealloc(G *g, void *p, size_t n) {
    void *q = realloc(p, n ? n : 1);
    if (!q) fail(g, "out of memory");
    return q;
}
static void *xcalloc(G *g, size_t n, size_t sz) {
    void *p = calloc(n ? n : 1, sz);
    if (!p) fail(g, "out of memory");
    return p;
}
#define GROW(g, ptr, n, cap)                                              \
    do {                                                                  \
        if ((n) == (cap)) {                                               \
            (cap) = (cap) ? (cap) * 2 : 16;                               \
            (ptr) = xrealloc((g), (ptr), (size_t)(cap) * sizeof(*(ptr))); \
        }                                                                 \
    } while (0)

static void bput(G *g, Buf *b, const char *s, size_t n) {
    if (b->n + n + 1 > b->cap) {
        size_t cap = b->cap ? b->cap : 4096;
        while (b->n + n + 1 > cap) cap *= 2;
        b->p = xrealloc(g, b->p, cap);
        b->cap = cap;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}
static void bputs(G *g, Buf *b, const char *s) { bput(g, b, s, strlen(s)); }
static void bprintf(G *g, Buf *b, const char *fmt, ...) {
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (n < 0) fail(g, "format error");
    if ((size_t)n < sizeof tmp) {
        bput(g, b, tmp, (size_t)n);
        return;
    }
    char *big = xcalloc(g, (size_t)n + 1, 1);
    va_start(ap, fmt);
    vsnprintf(big, (size_t)n + 1, fmt, ap);
    va_end(ap);
    bput(g, b, big, (size_t)n);
    free(big);
}

static uint32_t hash32(uint32_t k) {
    k ^= k >> 16;
    k *= 0x7feb352dU;
    k ^= k >> 15;
    k *= 0x846ca68bU;
    k ^= k >> 16;
    return k;
}
static int32_t *map_find(const Map *m, uint32_t key) {
    if (!m->cap) return NULL;
    uint32_t mask = m->cap - 1;
    for (uint32_t i = hash32(key) & mask;; i = (i + 1) & mask) {
        if (m->k[i] == key) return &m->v[i];
        if (m->k[i] == MAP_EMPTY) return NULL;
    }
}
static int map_has(const Map *m, uint32_t key) { return map_find(m, key) != NULL; }
static void map_put(G *g, Map *m, uint32_t key, int32_t val);
static void map_grow(G *g, Map *m) {
    Map old = *m;
    m->cap = old.cap ? old.cap * 2 : 64;
    m->n = 0;
    m->k = xrealloc(g, NULL, m->cap * sizeof(uint32_t));
    m->v = xrealloc(g, NULL, m->cap * sizeof(int32_t));
    memset(m->k, 0xFF, m->cap * sizeof(uint32_t));
    for (uint32_t i = 0; i < old.cap; i++)
        if (old.k[i] != MAP_EMPTY) map_put(g, m, old.k[i], old.v[i]);
    free(old.k);
    free(old.v);
}
static void map_put(G *g, Map *m, uint32_t key, int32_t val) {
    int32_t *v = map_find(m, key);
    if (v) {
        *v = val;
        return;
    }
    if ((m->n + 1) * 4 > m->cap * 3) map_grow(g, m);
    uint32_t mask = m->cap - 1, i = hash32(key) & mask;
    while (m->k[i] != MAP_EMPTY) i = (i + 1) & mask;
    m->k[i] = key;
    m->v[i] = val;
    m->n++;
}
static void map_free(Map *m) {
    free(m->k);
    free(m->v);
    memset(m, 0, sizeof(*m));
}

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

/* All keys of a map, sorted. */
static uint32_t *map_keys(G *g, const Map *m, int *n) {
    free(g->keys);
    g->keys = NULL;
    uint32_t *keys = g->keys = xcalloc(g, m->n, sizeof(uint32_t));
    int k = 0;
    for (uint32_t i = 0; i < m->cap; i++)
        if (m->k[i] != MAP_EMPTY) keys[k++] = m->k[i];
    qsort(keys, (size_t)k, sizeof(uint32_t), cmp_u32);
    *n = k;
    return keys;
}

/* ------------------------------------------------------------ program */

static int entry_has(const G *g, int seg, uint32_t off) {
    if (seg < 1 || seg > g->nseg || off > 0xFFFF) return 0;
    return (g->entries[seg][off >> 3] >> (off & 7)) & 1;
}
static void entry_add(G *g, int seg, uint32_t off) {
    if (seg < 1 || seg > g->nseg) fail(g, "entry point in segment %d of %d", seg, g->nseg);
    if (off > 0xFFFF) fail(g, "entry point offset 0x%x", (unsigned)off);
    g->entries[seg][off >> 3] |= (uint8_t)(1u << (off & 7));
}

static const Rel *reloc_at(const G *g, int seg, uint32_t loc) {
    if (loc > 0xFFFF) return NULL;
    int32_t *v = map_find(&g->reloc, (uint32_t)seg << 16 | loc);
    return v ? &g->rels[*v] : NULL;
}

/* The relocation on the instruction's 16-bit immediate / displacement. */
static const Rel *imm_value(const G *g, int seg, const X86Instr *i) {
    return i->imm_off < 0 ? NULL : reloc_at(g, seg, i->addr + (uint32_t)i->imm_off);
}
static const Rel *disp_value(const G *g, int seg, const X86Instr *i) {
    return i->disp_off < 0 ? NULL : reloc_at(g, seg, i->addr + (uint32_t)i->disp_off);
}

static const GenExport *export_lookup(const char *module, int ordinal) {
    unsigned lo = 0, hi = gen_export_count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        int c = strcmp(gen_exports[mid].module, module);
        if (!c) c = gen_exports[mid].ordinal < ordinal ? -1 : gen_exports[mid].ordinal > ordinal;
        if (!c) return &gen_exports[mid];
        if (c < 0) lo = mid + 1;
        else hi = mid;
    }
    return NULL;
}

static int import_index(G *g, const char *module, int ordinal) {
    for (int i = 0; i < g->nimports; i++)
        if (g->imports[i].ordinal == ordinal && !strcmp(g->imports[i].module, module)) return i;
    GROW(g, g->imports, g->nimports, g->capimports);
    Import *im = &g->imports[g->nimports];
    memset(im, 0, sizeof(*im));
    im->module = module;
    im->ordinal = ordinal;
    const GenExport *e = export_lookup(module, ordinal);
    if (e) {
        im->export_name = e->name;
        im->purge = e->purge;
        im->known = 1;
    } else {
        snprintf(im->fallback, sizeof im->fallback, "%s_%d", module, ordinal);
        im->purge = -1;
    }
    return g->nimports++;
}

static void add_reloc(G *g, int seg, uint32_t loc, Rel r) {
    if (loc > 0xFFFF) fail(g, "relocation at %d:0x%x", seg, (unsigned)loc);
    GROW(g, g->rels, g->nrels, g->caprels);
    g->rels[g->nrels] = r;
    map_put(g, &g->reloc, (uint32_t)seg << 16 | loc, g->nrels);
    g->nrels++;
}

static void parse_functions(G *g, const char *txt) {
    const char *p = txt;
    while (*p) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        char line[128];
        if (len >= sizeof line) fail(g, "functions list: line too long");
        memcpy(line, p, len);
        line[len] = 0;
        p += len + (eol ? 1 : 0);
        char *hash = strchr(line, '#');
        if (hash) *hash = 0;
        char *s = line;
        while (*s == ' ' || *s == '\t' || *s == '\r') s++;
        if (!*s) continue;
        char *end;
        unsigned long sel = strtoul(s, &end, 16);
        if (*end != ':') fail(g, "functions list: bad line '%s'", s);
        unsigned long off = strtoul(end + 1, &end, 16);
        while (*end == ' ' || *end == '\t' || *end == '\r') end++;
        if (*end || sel < 0x1000 || off > 0xFFFF) fail(g, "functions list: bad line '%s'", s);
        long segn = (long)(sel - 0x1000) / 8 + 1;
        if (segn >= 1 && segn <= g->nseg && g->is_code[segn]) entry_add(g, (int)segn, (uint32_t)off);
    }
}

static void program_init(G *g, const uint8_t *exe, size_t len, const char *functions_txt) {
    if (ne_parse(&g->ne, exe, len, g->err, g->errcap)) longjmp(g->jb, 1);
    g->nseg = g->ne.nseg;
    g->is_code = xcalloc(g, (size_t)g->nseg + 1, 1);
    g->entries = xcalloc(g, (size_t)g->nseg + 1, sizeof(uint8_t *));
    for (int n = 1; n <= g->nseg; n++) {
        const NESegment *s = &g->ne.segs[n];
        g->is_code[n] = !(s->flags & 1) && s->datalen > 0;
        g->entries[n] = xcalloc(g, 0x10000 / 8, 1);
    }
    for (int n = 1; n <= g->nseg; n++) {
        const NESegment *s = &g->ne.segs[n];
        for (int k = 0; k < s->nrelocs; k++) {
            const NEReloc *r = &s->relocs[k];
            for (int l = 0; l < r->nlocs; l++) {
                Rel rel = {0};
                if (r->ttype == NE_T_INTERNAL) {
                    rel.a = r->seg;
                    rel.b = r->off;
                    if (r->src_type == 2) rel.kind = RK_SEL;
                    else if (r->src_type == 3) rel.kind = RK_FARPTR;
                    else if (r->src_type == 5) rel.kind = RK_OFF;
                    else fail(g, "unhandled internal relocation (source type %d)", r->src_type);
                } else if (r->ttype == NE_T_ORD) {
                    rel.a = (uint16_t)import_index(g, g->ne.modules[r->module], r->off);
                    if (r->src_type == 3) rel.kind = RK_IMPORT;
                    else if (r->src_type == 5) rel.kind = RK_IMPORT_OFF;
                    else if (r->src_type == 2) rel.kind = RK_IMPORT_SEL;
                    else fail(g, "unhandled import relocation (source type %d)", r->src_type);
                } else {
                    fail(g, "unhandled relocation target type %d in segment %d", r->ttype, n);
                }
                add_reloc(g, n, r->locs[l], rel);
            }
        }
    }
    parse_functions(g, functions_txt ? functions_txt : gen_functions_txt);
    entry_add(g, g->ne.cs, g->ne.ip);
    for (int i = 0; i < g->ne.nentries; i++) entry_add(g, g->ne.entries[i].seg, g->ne.entries[i].off);
}

/* ------------------------------------------------------------ functions */

static void func_free(Func *f) {
    if (!f) return;
    for (int i = 0; i < f->nins; i++) free(f->ins[i].bad);
    free(f->ins);
    map_free(&f->at);
    map_free(&f->labels);
    map_free(&f->tail);
    map_free(&f->tables);
    for (int i = 0; i < f->ntab; i++) free(f->tab[i]);
    free(f->tab);
    free(f->tabn);
    free(f->calls);
    free(f);
}

static const Ins *func_ins(const Func *f, int64_t addr) {
    if (addr < 0 || addr > 0xFFFFFFFE) return NULL;
    int32_t *v = map_find(&f->at, (uint32_t)addr);
    return v ? &f->ins[*v] : NULL;
}

static void add_call(G *g, Func *f, int seg, uint32_t off) {
    GROW(g, f->calls, f->ncalls, f->capcalls);
    f->calls[f->ncalls++] = (uint32_t)seg << 16 | (off & 0xFFFF);
}

/* Targets of a `jmp cs:[bx+table]` switch, bounded by a preceding `cmp reg16, N`. */
static int jump_table(G *g, Func *f, const X86Instr *i, uint16_t **out) {
    const X86Op *m = &i->ops[0];
    if (!(m->kind == OPK_MEM && m->seg == SR_CS && m->nbase == 1 && m->base[0] == 3)) return 0;
    int found = 0;
    uint32_t n = 0;
    for (int back = 1; back < 20; back++) {
        const Ins *p = func_ins(f, (int64_t)i->addr - back);
        if (p && !p->bad && p->i.mnem == X86_CMP && p->i.ops[0].kind == OPK_REG && p->i.ops[1].kind == OPK_IMM &&
            p->i.ops[0].size == 2) {
            n = p->i.ops[1].val;
            found = 1;
            break;
        }
    }
    if (!found) return 0;
    const NESegment *s = &g->ne.segs[f->seg];
    uint16_t *t = xcalloc(g, n + 1, sizeof(uint16_t));
    for (uint32_t k = 0; k <= n; k++) {
        size_t o = (size_t)m->disp + 2 * k;
        if (o + 2 > s->datalen) {
            free(t);
            fail(g, "jump table at %04x:%04x runs past the segment", ne_sel_of(f->seg), (unsigned)i->addr);
        }
        t[k] = (uint16_t)(s->data[o] | s->data[o + 1] << 8);
    }
    *out = t;
    return (int)n + 1;
}

static Func *func_decode(G *g, int seg, uint32_t entry) {
    if (seg < 1 || seg > g->nseg || !g->is_code[seg]) fail(g, "function in non-code segment %d", seg);
    Func *f = g->cur = xcalloc(g, 1, sizeof(Func));
    f->seg = seg;
    f->entry = entry;
    const NESegment *s = &g->ne.segs[seg];
    int nwork = 0;
    GROW(g, g->work, nwork, g->capwork);
    g->work[nwork++] = entry;
    while (nwork) {
        uint32_t a = g->work[--nwork];
        while (!map_has(&f->at, a)) {
            Ins in;
            char err[96];
            in.bad = NULL;
            GROW(g, f->ins, f->nins, f->capins);
            if (x86_decode(s->data, s->datalen, a, &in.i, err, sizeof err)) {
                in.i.addr = a;
                in.bad = xcalloc(g, strlen(err) + 1, 1);
                memcpy(in.bad, err, strlen(err));
                f->ins[f->nins] = in;
                map_put(g, &f->at, a, f->nins++);
                break;
            }
            f->ins[f->nins] = in;
            map_put(g, &f->at, a, f->nins++);
            const X86Instr *i = &in.i;
            int m = i->mnem, stop = 0;
            if (m == X86_JCC || m == X86_LOOP || m == X86_LOOPE || m == X86_LOOPNE || m == X86_JCXZ) {
                map_put(g, &f->labels, (uint32_t)i->target, 1);
                GROW(g, g->work, nwork, g->capwork);
                g->work[nwork++] = (uint32_t)i->target;
            } else if (m == X86_JMP) {
                if (entry_has(g, seg, (uint32_t)i->target) && (uint32_t)i->target != entry) {
                    map_put(g, &f->tail, (uint32_t)i->target, 1);
                } else {
                    map_put(g, &f->labels, (uint32_t)i->target, 1);
                    GROW(g, g->work, nwork, g->capwork);
                    g->work[nwork++] = (uint32_t)i->target;
                }
                break;
            } else if (m == X86_CALL) {
                add_call(g, f, seg, (uint32_t)i->target);
            } else if (m == X86_JMP_IND) {
                uint16_t *t = NULL;
                GROW(g, f->tab, f->ntab, f->captab);
                f->tabn = xrealloc(g, f->tabn, (size_t)f->captab * sizeof(int));
                int n = jump_table(g, f, i, &t);
                if (n) {
                    f->tab[f->ntab] = t;
                    f->tabn[f->ntab] = n;
                    map_put(g, &f->tables, a, f->ntab++);
                    for (int k = 0; k < n; k++) {
                        map_put(g, &f->labels, t[k], 1);
                        GROW(g, g->work, nwork, g->capwork);
                        g->work[nwork++] = t[k];
                    }
                }
                break;
            }
            if (m == X86_RET || m == X86_RETF || m == X86_IRET || m == X86_JMPF || m == X86_JMPF_IND || m == X86_HLT)
                stop = 1;
            if (stop) break;
            if (m == X86_CALLF) {
                const Rel *rs = reloc_at(g, seg, i->addr + (uint32_t)i->imm_off + 2);
                if (rs && rs->kind == RK_SEL) add_call(g, f, rs->a, i->far_off);
            }
            a += i->length;
        }
    }
    g->cur = NULL;
    return f;
}

/* ------------------------------------------------------------ discovery */

static const uint8_t *const prologues[] = {(const uint8_t *)"\x55\x8b\xec", (const uint8_t *)"\x8c\xd8\x90",
                                           (const uint8_t *)"\x1e\x58\x90", (const uint8_t *)"\x45\x55",
                                           (const uint8_t *)"\x8c\xd8\x45"};
static const int prologue_len[] = {3, 3, 3, 2, 3};

static int looks_like_entry(const G *g, int seg, uint32_t off) {
    if (seg < 1 || seg > g->nseg || !g->is_code[seg]) return 0;
    const NESegment *s = &g->ne.segs[seg];
    if ((int64_t)off >= (int64_t)s->datalen - 4) return 0;
    if (s->data[off] == 0xC8 && s->data[off + 3] == 0) return 1;  /* enter N, 0 */
    for (int k = 0; k < 5; k++)
        if (!memcmp(s->data + off, prologues[k], (size_t)prologue_len[k])) return 1;
    return 0;
}

static int cmp_ins_addr(const void *a, const void *b) {
    const Ins *x = *(const Ins *const *)a, *y = *(const Ins *const *)b;
    return x->i.addr < y->i.addr ? -1 : x->i.addr > y->i.addr;
}

/* The function's instructions sorted by address (bad ones too when keep_bad). */
static const Ins **sorted_ins(G *g, const Func *f, int keep_bad, int *n) {
    free(g->sorted);
    g->sorted = NULL;
    const Ins **v = g->sorted = xcalloc(g, (size_t)f->nins, sizeof(Ins *));
    int k = 0;
    for (int i = 0; i < f->nins; i++)
        if (keep_bad || !f->ins[i].bad) v[k++] = &f->ins[i];
    qsort(v, (size_t)k, sizeof(Ins *), cmp_ins_addr);
    *n = k;
    return v;
}

/* Function pointers built from a relocated selector and a nearby offset
 * (window procedures, callbacks, vtables) that no direct call reaches. */
static void discover_pointer_entries(G *g) {
    Map *found = &g->found;
    size_t bmsize = 0x10100 / 8;
    uint8_t **inside = g->inside = xcalloc(g, (size_t)g->nseg + 1, sizeof(uint8_t *));
    for (int n = 1; n <= g->nseg; n++) inside[n] = xcalloc(g, bmsize, 1);
    /* bytes covered by already-decoded instructions (a real entry is never mid-instruction) */
    for (int fi = 0; fi < g->nfuncs; fi++) {
        const Func *f = g->funcs[fi];
        for (int k = 0; k < f->nins; k++) {
            const Ins *in = &f->ins[k];
            if (in->bad) continue;
            for (uint32_t b = in->i.addr + 1; b < in->i.addr + in->i.length; b++)
                if (b < 0x10100) inside[f->seg][b >> 3] |= (uint8_t)(1u << (b & 7));
        }
    }
    /* 1. far pointer cells (offset word followed by a relocated selector word) */
    for (uint32_t s = 0; s < g->reloc.cap; s++) {
        uint32_t key = g->reloc.k[s];
        if (key == MAP_EMPTY) continue;
        const Rel *rel = &g->rels[g->reloc.v[s]];
        int n = (int)(key >> 16);
        uint32_t loc = key & 0xFFFF;
        if (rel->kind != RK_SEL || rel->a < 1 || rel->a > g->nseg || !g->is_code[rel->a] || loc < 2) continue;
        const NESegment *sg = &g->ne.segs[n];
        if (loc > sg->datalen) fail(g, "relocation at %d:0x%x outside the segment", n, (unsigned)loc);
        uint32_t off = sg->data[loc - 2] | (uint32_t)sg->data[loc - 1] << 8;
        if (!entry_has(g, rel->a, off) && looks_like_entry(g, rel->a, off))
            map_put(g, found, (uint32_t)rel->a << 16 | off, 1);
    }
    /* 2. instruction pairs: `mov [x], OFF` / `push OFF` near a relocated selector immediate */
    for (int fi = 0; fi < g->nfuncs; fi++) {
        const Func *f = g->funcs[fi];
        int na;
        const Ins **v = sorted_ins(g, f, 0, &na);
        for (int k = 0; k < na; k++) {
            const Rel *r = imm_value(g, f->seg, &v[k]->i);
            if (!r || r->kind != RK_SEL || r->a < 1 || r->a > g->nseg || !g->is_code[r->a]) continue;
            int lo = k - 4 < 0 ? 0 : k - 4, hi = k + 5 < na ? k + 5 : na;
            for (int j = lo; j < hi; j++) {
                if (j == k) continue;
                const X86Instr *o = &v[j]->i;
                for (int q = 0; q < o->nops; q++) {
                    const X86Op *op = &o->ops[q];
                    if (op->kind == OPK_IMM && op->size == 2 && !imm_value(g, f->seg, o) &&
                        !entry_has(g, r->a, op->val) && looks_like_entry(g, r->a, op->val))
                        map_put(g, found, (uint32_t)r->a << 16 | op->val, 1);
                }
            }
        }
    }
    int nf;
    uint32_t *keys = map_keys(g, found, &nf);
    for (int k = 0; k < nf; k++) {
        int seg = (int)(keys[k] >> 16);
        uint32_t off = keys[k] & 0xFFFF;
        if ((inside[seg][off >> 3] >> (off & 7)) & 1) continue;
        Func *f = func_decode(g, seg, off);
        int ok = 1;
        for (int i = 0; i < f->nins; i++)
            if (f->ins[i].bad) ok = 0;
        g->cur = f;  /* freed by g_free if map_put fails */
        if (ok) map_put(g, &g->good, keys[k], 1);
        g->cur = NULL;
        func_free(f);
    }
}

/* Decodes every entry not decoded yet, adding direct call targets as new
 * entries, until nothing new turns up. */
static void decode_to_fixed_point(G *g) {
    for (;;) {
        int any = 0;
        for (int s = 1; s <= g->nseg; s++)
            for (uint32_t e = 0; e <= 0xFFFF; e++) {
                if (!entry_has(g, s, e) || map_has(&g->done, (uint32_t)s << 16 | e)) continue;
                GROW(g, g->funcs, g->nfuncs, g->capfuncs);
                Func *f = g->funcs[g->nfuncs] = func_decode(g, s, e);
                map_put(g, &g->done, (uint32_t)s << 16 | e, g->nfuncs++);
                for (int c = 0; c < f->ncalls; c++) entry_add(g, (int)(f->calls[c] >> 16), f->calls[c] & 0xFFFF);
                any = 1;
            }
        if (!any) return;
    }
}

/* ------------------------------------------------------------ emission */

typedef struct S {
    char s[256];
} S;

static S sf(const char *fmt, ...) {
    S r;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(r.s, sizeof r.s, fmt, ap);
    va_end(ap);
    return r;
}

static const char *const R8[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
static const char *const R16[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
static const char *const R32[8] = {"EAX", "ECX", "EDX", "EBX", "cpu.r[R_ESP].e", "cpu.r[R_EBP].e",
                                   "cpu.r[R_ESI].e", "cpu.r[R_EDI].e"};
static const char *const CCX[16] = {
    "cpu.of", "!cpu.of", "cpu.cf", "!cpu.cf", "cpu.zf", "!cpu.zf", "(cpu.cf || cpu.zf)", "(!cpu.cf && !cpu.zf)",
    "cpu.sf", "!cpu.sf", "PF()", "!PF()", "(cpu.sf != cpu.of)", "(cpu.sf == cpu.of)",
    "(cpu.zf || cpu.sf != cpu.of)", "(!cpu.zf && cpu.sf == cpu.of)"};

typedef struct E {
    G *g;
    Func *f;
    unsigned cs;
    Buf *out;
    int first;
} E;

static const char *T(E *e, int size) {
    switch (size) {
    case 1: return "uint8_t";
    case 2: return "uint16_t";
    case 4: return "uint32_t";
    }
    fail(e->g, "bad operand size %d at %04x", size, e->cs);
    return NULL;
}
static const char *RDF(E *e, int size) {
    switch (size) {
    case 1: return "RB";
    case 2: return "RW";
    case 4: return "RD";
    }
    fail(e->g, "bad memory size %d at %04x", size, e->cs);
    return NULL;
}
static const char *WRF(E *e, int size) {
    switch (size) {
    case 1: return "WB";
    case 2: return "WW";
    case 4: return "WD";
    }
    fail(e->g, "bad memory size %d at %04x", size, e->cs);
    return NULL;
}
static const char *bits(E *e, int size) {
    switch (size) {
    case 1: return "8";
    case 2: return "16";
    case 4: return "32";
    }
    fail(e->g, "bad operand size %d at %04x", size, e->cs);
    return NULL;
}

static void line_raw(E *e, const char *s) {
    if (!e->first) bput(e->g, e->out, "\n", 1);
    e->first = 0;
    bputs(e->g, e->out, s);
}
static void w(E *e, const char *fmt, ...) {
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof tmp) fail(e->g, "generated line too long at %04x", e->cs);
    if (!e->first) bput(e->g, e->out, "\n", 1);
    e->first = 0;
    bput(e->g, e->out, "    ", 4);
    bput(e->g, e->out, tmp, (size_t)n);
}

static S segval(E *e, int seg) {
    switch (seg) {
    case SR_CS: return sf("0x%04x", e->cs);
    case SR_ES: return sf("ES");
    case SR_SS: return sf("SS");
    case SR_DS: return sf("DS");
    case SR_FS: return sf("cpu.s[S_FS]");
    case SR_GS: return sf("cpu.s[S_GS]");
    }
    fail(e->g, "bad segment register %d", seg);
    return sf("");
}

static uint32_t reloc_word(E *e, const Rel *rel) {
    switch (rel->kind) {
    case RK_SEL: return ne_sel_of(rel->a);
    case RK_IMPORT_OFF: {
        const Import *im = &e->g->imports[rel->a];
        const char *name = im->known ? im->export_name : "?";
        if (!strcmp(name, "__AHINCR")) return 8;
        if (!strcmp(name, "__AHSHIFT")) return 3;
        if (!strcmp(name, "__WINFLAGS")) return 0x0413;  /* WF_PMODE|WF_CPU386|WF_ENHANCED */
        fail(e->g, "unhandled import offset %s", name);
    }
    case RK_OFF: return rel->b;
    }
    fail(e->g, "unhandled relocation in operand (kind %d)", rel->kind);
    return 0;
}

static S ea(E *e, const X86Op *m, const X86Instr *ins) {
    S r;
    size_t n = 0;
    int parts = 0;
    r.s[0] = 0;
    n += (size_t)snprintf(r.s, sizeof r.s, "(uint16_t)(");
    for (int k = 0; k < m->nbase; k++, parts++)
        n += (size_t)snprintf(r.s + n, sizeof r.s - n, "%s%s", parts ? " + " : "", R16[m->base[k]]);
    uint32_t disp = m->disp;
    const Rel *rel = disp_value(e->g, e->f->seg, ins);
    if (rel) disp = reloc_word(e, rel);
    if (disp) n += (size_t)snprintf(r.s + n, sizeof r.s - n, "%s0x%04x", parts++ ? " + " : "", (unsigned)disp);
    if (!parts) return sf("0");
    snprintf(r.s + n, sizeof r.s - n, ")");
    return r;
}

static S rd(E *e, const X86Op *o, const X86Instr *ins, int size) {
    switch (o->kind) {
    case OPK_REG: return sf("%s", (o->size == 1 ? R8 : o->size == 2 ? R16 : R32)[o->idx]);
    case OPK_SREG: {
        static const char *const sr[6] = {"ES", "", "SS", "DS", "cpu.s[S_FS]", "cpu.s[S_GS]"};
        if (o->idx == 1) return sf("0x%04x", e->cs);
        if (o->idx >= 6) fail(e->g, "bad segment register %d", o->idx);
        return sf("%s", sr[o->idx]);
    }
    case OPK_IMM: {
        const Rel *rel = imm_value(e->g, e->f->seg, ins);
        uint32_t v = o->val;
        if (rel && o->size == 2) v = reloc_word(e, rel);
        return o->size != 1 ? sf("0x%xu", (unsigned)v) : sf("0x%x", (unsigned)v);
    }
    case OPK_MEM: {
        int sz = size ? size : o->size;
        S sv = segval(e, o->seg), a = ea(e, o, ins);
        return sf("%s(%s, %s)", RDF(e, sz), sv.s, a.s);
    }
    }
    fail(e->g, "bad operand");
    return sf("");
}

static S wr(E *e, const X86Op *o, const char *val, const X86Instr *ins, int size) {
    switch (o->kind) {
    case OPK_REG: {
        S r = rd(e, o, ins, 0);
        return sf("%s = (%s)(%s);", r.s, T(e, o->size), val);
    }
    case OPK_SREG: {
        if (o->idx == 1) fail(e->g, "write to cs");
        S r = rd(e, o, ins, 0);
        return sf("%s = (uint16_t)(%s);", r.s, val);
    }
    case OPK_MEM: {
        int sz = size ? size : o->size;
        S sv = segval(e, o->seg), a = ea(e, o, ins);
        return sf("%s(%s, %s, (%s)(%s));", WRF(e, sz), sv.s, a.s, T(e, sz), val);
    }
    }
    fail(e->g, "bad destination");
    return sf("");
}

/* Read-modify-write with a single address computation: x = pre x post. */
static void rmw(E *e, const X86Op *o, const X86Instr *ins, const char *pre, const char *post) {
    if (o->kind == OPK_MEM) {
        int sz = o->size;
        S sv = segval(e, o->seg), a = ea(e, o, ins);
        w(e, "{ uint16_t s_ = %s, a_ = %s; %s(s_, a_, %s%s(s_, a_)%s); }", sv.s, a.s, WRF(e, sz), pre, RDF(e, sz),
          post);
    } else {
        S r = rd(e, o, ins, 0);
        w(e, "%s = %s%s%s;", r.s, pre, r.s, post);
    }
}

static int size_of(const X86Instr *i) {
    for (int k = 0; k < i->nops; k++)
        if ((i->ops[k].kind == OPK_REG || i->ops[k].kind == OPK_MEM) && i->ops[k].size) return i->ops[k].size;
    return i->opsize;
}

static void call_internal(E *e, int seg, uint32_t off, int far, uint32_t ret_ip) {
    if (far) w(e, "PUSH16(0x%04x); PUSH16(0x%04x);", e->cs, (unsigned)ret_ip);
    else w(e, "PUSH16(0x%04x);", (unsigned)ret_ip);
    if (entry_has(e->g, seg, off)) {
        w(e, "f_%04x_%04x();", ne_sel_of(seg), (unsigned)off);
    } else {
        map_put(e->g, &e->g->new_entries, (uint32_t)seg << 16 | off, 1);
        w(e, "call_far(0x%04x, 0x%04x);", ne_sel_of(seg), (unsigned)off);
    }
}

static void emit_string(E *e, const X86Instr *i) {
    int sz = i->opsize;
    S step = sf("(cpu.df ? -%d : %d)", sz, sz);
    S src = segval(e, i->src_seg);
    const char *reg = sz == 1 ? "AL" : sz == 2 ? "AX" : "EAX";
    S body;
    T(e, sz);
    switch (i->mnem) {
    case X86_MOVS:
        body = sf("%s(ES, DI, %s(%s, SI)); SI += %s; DI += %s;", WRF(e, sz), RDF(e, sz), src.s, step.s, step.s);
        break;
    case X86_STOS: body = sf("%s(ES, DI, %s); DI += %s;", WRF(e, sz), reg, step.s); break;
    case X86_LODS: body = sf("%s = %s(%s, SI); SI += %s;", reg, RDF(e, sz), src.s, step.s); break;
    case X86_SCAS: body = sf("sub%d(%s, %s(ES, DI)); DI += %s;", sz * 8, reg, RDF(e, sz), step.s); break;
    default:
        body = sf("sub%d(%s(%s, SI), %s(ES, DI)); SI += %s; DI += %s;", sz * 8, RDF(e, sz), src.s, RDF(e, sz),
                  step.s, step.s);
        break;
    }
    int m = i->mnem;
    if (i->rep == REP_NONE) w(e, "%s", body.s);
    else if (i->rep == REP_REP || m == X86_MOVS || m == X86_STOS || m == X86_LODS)
        w(e, "while (CX) { %s CX--; }", body.s);
    else if (i->rep == REP_REPE) w(e, "while (CX) { %s CX--; if (!cpu.zf) break; }", body.s);
    else w(e, "while (CX) { %s CX--; if (cpu.zf) break; }", body.s);
}

/* Emits one instruction; returns whether execution falls through. */
static int emit_ins(E *e, const X86Instr *i) {
    const X86Op *o = i->ops;
    int m = i->mnem;
    uint32_t next_ip = i->addr + i->length;
    G *g = e->g;
    const char *name = x86_mnemonic_names[m];
    S a, b, c;

    switch (m) {
    case X86_MOV:
        if (o[0].kind == OPK_SREG && o[1].kind == OPK_MEM) {
            a = rd(e, &o[1], i, 2);
            b = wr(e, &o[0], a.s, i, 0);
        } else if (o[1].kind == OPK_SREG && o[0].kind == OPK_MEM) {
            a = rd(e, &o[1], i, 0);
            b = wr(e, &o[0], a.s, i, 2);
        } else {
            a = rd(e, &o[1], i, 0);
            b = wr(e, &o[0], a.s, i, 0);
        }
        w(e, "%s", b.s);
        break;
    case X86_ADD: case X86_OR: case X86_ADC: case X86_SBB: case X86_AND: case X86_SUB: case X86_XOR: {
        const char *bb = bits(e, o[0].size);
        S src = rd(e, &o[1], i, 0);
        if (m == X86_XOR && x86_op_equal(&o[0], &o[1])) {
            a = rd(e, &o[0], i, 0);
            w(e, "%s = 0; cpu.cf = cpu.of = 0; cpu.zf = 1; cpu.sf = 0; cpu.pres = 0;", a.s);
        } else {
            w(e, "{ %s v_ = (%s)%s;", T(e, o[0].size), T(e, o[0].size), src.s);
            S pre = sf("%s%s(", name, bb);
            rmw(e, &o[0], i, pre.s, ", v_)");
            w(e, "}");
        }
        break;
    }
    case X86_CMP: case X86_TEST:
        a = rd(e, &o[0], i, 0);
        b = rd(e, &o[1], i, 0);
        w(e, "%s%s(%s, (%s)%s);", m == X86_CMP ? "sub" : "and", bits(e, o[0].size), a.s, T(e, o[0].size), b.s);
        break;
    case X86_INC: case X86_DEC: case X86_NEG: {
        S pre = sf("%s%s(", name, bits(e, o[0].size));
        rmw(e, &o[0], i, pre.s, ")");
        break;
    }
    case X86_NOT: {
        S pre = sf("(%s)~", T(e, o[0].size));
        rmw(e, &o[0], i, pre.s, "");
        break;
    }
    case X86_ROL: case X86_ROR: case X86_RCL: case X86_RCR: case X86_SHL: case X86_SHR: case X86_SAL:
    case X86_SAR: {
        a = rd(e, &o[1], i, 0);
        w(e, "{ unsigned c_ = %s;", a.s);
        S pre = sf("%s%s(", m == X86_SAL ? "shl" : name, bits(e, o[0].size));
        rmw(e, &o[0], i, pre.s, ", c_)");
        w(e, "}");
        break;
    }
    case X86_SHLD: case X86_SHRD: {
        a = rd(e, &o[2], i, 0);
        b = rd(e, &o[1], i, 0);
        w(e, "{ unsigned c_ = %s; %s s2_ = %s;", a.s, T(e, o[0].size), b.s);
        S pre = sf("%s%s(", name, bits(e, o[0].size));
        rmw(e, &o[0], i, pre.s, ", s2_, c_)");
        w(e, "}");
        break;
    }
    case X86_XCHG: {
        int sz = size_of(i);
        if (x86_op_equal(&o[0], &o[1])) break;
        if (o[0].kind == OPK_MEM) {
            S sv = segval(e, o[0].seg), ad = ea(e, &o[0], i);
            b = rd(e, &o[1], i, 0);
            w(e, "{ uint16_t s_ = %s, a_ = %s; %s t_ = %s(s_, a_); %s(s_, a_, %s); %s = t_; }", sv.s, ad.s, T(e, sz),
              RDF(e, sz), WRF(e, sz), b.s, b.s);
        } else {
            a = rd(e, &o[0], i, 0);
            b = rd(e, &o[1], i, 0);
            w(e, "{ %s t_ = %s; %s = %s; %s = t_; }", T(e, sz), a.s, a.s, b.s, b.s);
        }
        break;
    }
    case X86_LEA:
        a = rd(e, &o[0], i, 0);
        b = ea(e, &o[1], i);
        w(e, "%s = %s;", a.s, b.s);
        break;
    case X86_LES: case X86_LDS: case X86_LSS: case X86_LFS: case X86_LGS: {
        const char *sreg = m == X86_LES ? "ES" : m == X86_LDS ? "DS" : m == X86_LSS ? "SS"
                         : m == X86_LFS ? "cpu.s[S_FS]" : "cpu.s[S_GS]";
        S sv = segval(e, o[1].seg), ad = ea(e, &o[1], i);
        a = rd(e, &o[0], i, 0);
        w(e, "{ uint16_t s_ = %s, a_ = %s; %s = %s(s_, a_); %s = RW(s_, (uint16_t)(a_ + %d)); }", sv.s, ad.s, a.s,
          RDF(e, o[0].size), sreg, o[0].size);
        break;
    }
    case X86_MOVZX:
        a = rd(e, &o[0], i, 0);
        b = rd(e, &o[1], i, 0);
        w(e, "%s = (%s)%s;", a.s, T(e, o[0].size), b.s);
        break;
    case X86_MOVSX: {
        const char *st = o[1].size == 1 ? "int8_t" : o[1].size == 2 ? "int16_t" : NULL;
        if (!st) fail(g, "movsx from size %d", o[1].size);
        a = rd(e, &o[0], i, 0);
        b = rd(e, &o[1], i, 0);
        w(e, "%s = (%s)(%s)%s;", a.s, T(e, o[0].size), st, b.s);
        break;
    }
    case X86_PUSH: {
        const X86Op *x = &o[0];
        a = rd(e, x, i, 0);
        if (x->kind == OPK_SREG) w(e, "PUSH16(%s);", a.s);
        else if (x->kind == OPK_IMM) w(e, "PUSH%s(%s);", i->opsize == 4 ? "32" : "16", a.s);
        else if (x->kind == OPK_REG && x->size == 2 && x->idx == 4) w(e, "{ uint16_t t_ = SP; PUSH16(t_); }");
        else w(e, "PUSH%s(%s);", bits(e, x->size), a.s);
        break;
    }
    case X86_POP: {
        const X86Op *x = &o[0];
        if (x->kind == OPK_SREG) {
            a = rd(e, x, i, 0);
            w(e, "%s = POP16();", a.s);
        } else if (x->kind == OPK_MEM) {
            b = wr(e, x, "t_", i, 0);
            w(e, "{ %s t_ = POP%s(); %s }", T(e, x->size), bits(e, x->size), b.s);
        } else {
            a = rd(e, x, i, 0);
            w(e, "%s = POP%s();", a.s, bits(e, x->size));
        }
        break;
    }
    case X86_PUSHA:
        w(e, "{ uint16_t t_ = SP; PUSH16(AX); PUSH16(CX); PUSH16(DX); PUSH16(BX); PUSH16(t_); PUSH16(BP); "
             "PUSH16(SI); PUSH16(DI); }");
        break;
    case X86_POPA:
        w(e, "DI = POP16(); SI = POP16(); BP = POP16(); SP += 2; BX = POP16(); DX = POP16(); CX = POP16(); "
             "AX = POP16();");
        break;
    case X86_PUSHF: w(e, "PUSH16(get_flags());"); break;
    case X86_POPF: w(e, "set_flags(POP16());"); break;
    case X86_LAHF: w(e, "AH = (uint8_t)get_flags();"); break;
    case X86_SAHF: w(e, "set_flags((uint16_t)((get_flags() & 0xff00) | AH));"); break;
    case X86_CBW: w(e, "AX = (uint16_t)(int16_t)(int8_t)AL;"); break;
    case X86_CWDE: w(e, "EAX = (uint32_t)(int32_t)(int16_t)AX;"); break;
    case X86_CWD: w(e, "DX = (AX & 0x8000) ? 0xffff : 0;"); break;
    case X86_CDQ: w(e, "EDX = (EAX & 0x80000000u) ? 0xffffffffu : 0;"); break;
    case X86_MUL: case X86_IMUL: case X86_DIV: case X86_IDIV:
        a = rd(e, &o[0], i, 0);
        w(e, "cpu_%s%s(%s);", name, bits(e, o[0].size), a.s);
        break;
    case X86_IMUL3:
        a = rd(e, &o[0], i, 0);
        b = rd(e, &o[1], i, 0);
        c = rd(e, &o[2], i, 0);
        w(e, "%s = cpu_imul3_%s(%s, %s);", a.s, bits(e, o[0].size), b.s, c.s);
        break;
    case X86_IMUL2:
        a = rd(e, &o[0], i, 0);
        b = rd(e, &o[1], i, 0);
        w(e, "%s = cpu_imul3_%s(%s, %s);", a.s, bits(e, o[0].size), a.s, b.s);
        break;
    case X86_CLC: w(e, "cpu.cf = 0;"); break;
    case X86_STC: w(e, "cpu.cf = 1;"); break;
    case X86_CMC: w(e, "cpu.cf ^= 1;"); break;
    case X86_CLD: w(e, "cpu.df = 0;"); break;
    case X86_STD: w(e, "cpu.df = 1;"); break;
    case X86_CLI: w(e, "cpu.ifl = 0;"); break;
    case X86_STI: w(e, "cpu.ifl = 1;"); break;
    case X86_NOP: case X86_WAIT: break;
    case X86_SETCC: {
        S v = sf("%s ? 1 : 0", CCX[i->cc]);
        a = wr(e, &o[0], v.s, i, 0);
        w(e, "%s", a.s);
        break;
    }
    case X86_BT: case X86_BTS: case X86_BTR: case X86_BTC: {
        int nb = o[0].size * 8;
        bits(e, o[0].size);
        b = rd(e, &o[1], i, 0);
        w(e, "{ unsigned n_ = (%s) & %d; ", b.s, nb - 1);
        a = rd(e, &o[0], i, 0);
        if (m == X86_BT) {
            w(e, "cpu.cf = (%s >> n_) & 1; }", a.s);
        } else {
            w(e, "cpu.cf = (%s >> n_) & 1;", a.s);
            S pre = sf("(%s)(", T(e, o[0].size));
            rmw(e, &o[0], i, pre.s,
                m == X86_BTS ? " | (1u << n_))" : m == X86_BTR ? " & ~(1u << n_))" : " ^ (1u << n_))");
            w(e, "}");
        }
        break;
    }
    case X86_ENTER:
        if (o[1].val != 0) fail(g, "enter level %u at %04x:%04x", (unsigned)o[1].val, e->cs, (unsigned)i->addr);
        w(e, "PUSH16(BP); BP = SP; SP -= 0x%x;", (unsigned)o[0].val);
        break;
    case X86_LEAVE: w(e, "SP = BP; BP = POP16();"); break;
    case X86_XLAT:
        a = segval(e, i->src_seg);
        w(e, "AL = RB(%s, (uint16_t)(BX + AL));", a.s);
        break;
    case X86_MOVS: case X86_STOS: case X86_LODS: case X86_SCAS: case X86_CMPS: emit_string(e, i); break;
    case X86_JCC: w(e, "if (%s) goto L_%04x;", CCX[i->cc], (unsigned)i->target); break;
    case X86_JCXZ: w(e, "if (CX == 0) goto L_%04x;", (unsigned)i->target); break;
    case X86_LOOP: w(e, "if (--CX != 0) goto L_%04x;", (unsigned)i->target); break;
    case X86_LOOPE: w(e, "if (--CX != 0 && cpu.zf) goto L_%04x;", (unsigned)i->target); break;
    case X86_LOOPNE: w(e, "if (--CX != 0 && !cpu.zf) goto L_%04x;", (unsigned)i->target); break;
    case X86_JMP:
        if (map_has(&e->f->tail, (uint32_t)i->target)) w(e, "f_%04x_%04x(); return;", e->cs, (unsigned)i->target);
        else w(e, "goto L_%04x;", (unsigned)i->target);
        return 0;
    case X86_JMP_IND: {
        int32_t *t = map_find(&e->f->tables, i->addr);
        a = rd(e, &o[0], i, 0);
        if (t) {
            int n = e->f->tabn[*t];
            free(g->sw);
            uint32_t *v = g->sw = xcalloc(g, (size_t)n, sizeof(uint32_t));
            for (int k = 0; k < n; k++) v[k] = e->f->tab[*t][k];
            qsort(v, (size_t)n, sizeof(uint32_t), cmp_u32);
            w(e, "switch (%s) {", a.s);
            for (int k = 0; k < n; k++)
                if (!k || v[k] != v[k - 1]) w(e, "case 0x%04x: goto L_%04x;", (unsigned)v[k], (unsigned)v[k]);
            w(e, "default: recomp_bad_jump(0x%04x, 0x%04x, %s);", e->cs, (unsigned)i->addr, a.s);
            w(e, "}");
        } else {
            w(e, "recomp_bad_jump(0x%04x, 0x%04x, %s);", e->cs, (unsigned)i->addr, a.s);
        }
        return 0;
    }
    case X86_JMPF_IND: {
        S sv = segval(e, o[0].seg), ad = ea(e, &o[0], i);
        w(e, "{ uint16_t s_ = %s, a_ = %s; if (jmp_far(RW(s_, (uint16_t)(a_ + 2)), RW(s_, a_))) return; return; }",
          sv.s, ad.s);
        return 0;
    }
    case X86_JMPF: {
        const Rel *rel = reloc_at(g, e->f->seg, i->addr + (uint32_t)i->imm_off + 2);
        if (!rel || rel->kind != RK_SEL) fail(g, "unresolved jmpf at %04x:%04x", e->cs, (unsigned)i->addr);
        w(e, "f_%04x_%04x(); return;", ne_sel_of(rel->a), (unsigned)i->far_off);
        return 0;
    }
    case X86_CALL: call_internal(e, e->f->seg, (uint32_t)i->target, 0, next_ip); break;
    case X86_CALL_IND:
        a = rd(e, &o[0], i, 0);
        w(e, "{ uint16_t t_ = %s; PUSH16(0x%04x); call_far(0x%04x, t_); }", a.s, (unsigned)next_ip, e->cs);
        break;
    case X86_CALLF: {
        const Rel *ro = reloc_at(g, e->f->seg, i->addr + (uint32_t)i->imm_off);
        const Rel *rs = reloc_at(g, e->f->seg, i->addr + (uint32_t)i->imm_off + 2);
        if (ro && ro->kind == RK_IMPORT) {
            Import *im = &g->imports[ro->a];
            im->used = 1;
            w(e, "PUSH16(0x%04x); PUSH16(0x%04x); api_%s_%s();", e->cs, (unsigned)next_ip, im->module, imp_name(im));
        } else if (rs && rs->kind == RK_SEL) {
            call_internal(e, rs->a, i->far_off, 1, next_ip);
        } else {
            fail(g, "unresolved callf at %04x:%04x", e->cs, (unsigned)i->addr);
        }
        break;
    }
    case X86_CALLF_IND: {
        S sv = segval(e, o[0].seg), ad = ea(e, &o[0], i);
        w(e, "{ uint16_t s_ = %s, a_ = %s; uint16_t to_ = RW(s_, a_), ts_ = RW(s_, (uint16_t)(a_ + 2)); "
             "PUSH16(0x%04x); PUSH16(0x%04x); call_far(ts_, to_); }",
          sv.s, ad.s, e->cs, (unsigned)next_ip);
        break;
    }
    case X86_RET: w(e, "SP += %u; return;", 2 + (i->nops ? (unsigned)o[0].val : 0)); return 0;
    case X86_RETF: w(e, "SP += %u; return;", 4 + (i->nops ? (unsigned)o[0].val : 0)); return 0;
    case X86_IRET: w(e, "SP += 4; set_flags(POP16()); return;"); return 0;
    case X86_INT: w(e, "cpu_int(0x%02x);", (unsigned)o[0].val); break;
    case X86_IN: case X86_OUT: case X86_INS: case X86_OUTS: w(e, "cpu_in_out();"); break;
    case X86_HLT: w(e, "recomp_fatal(\"hlt\");"); return 0;
    default: {
        char txt[128];
        x86_format(i, txt, sizeof txt);
        fail(g, "unhandled op %s at %04x:%04x: %s", name, e->cs, (unsigned)i->addr, txt);
    }
    }
    return 1;
}

static void emit_func(G *g, Func *f, Buf *out) {
    E e = {g, f, ne_sel_of(f->seg), out, 1};
    char txt[160];
    snprintf(txt, sizeof txt, "void f_%04x_%04x(void) {", e.cs, (unsigned)f->entry);
    line_raw(&e, txt);
    int n;
    const Ins **v = sorted_ins(g, f, 1, &n);
    /* the entry may not be the lowest address: jump to it first */
    if (n && v[0]->i.addr != f->entry) {
        map_put(g, &f->labels, f->entry, 1);
        w(&e, "goto L_%04x;", (unsigned)f->entry);
    }
    for (int k = 0; k < n; k++) {
        const Ins *in = v[k];
        uint32_t a = in->i.addr;
        if (map_has(&f->labels, a)) {
            snprintf(txt, sizeof txt, "L_%04x:;", (unsigned)a);
            line_raw(&e, txt);
        }
        if (in->bad) {
            w(&e, "recomp_fatal(\"bad instruction at %04x:%04x: %s\");", e.cs, (unsigned)a, in->bad);
            continue;
        }
        x86_format(&in->i, txt, sizeof txt);
        w(&e, "/* %04x: %s */", (unsigned)a, txt);
        int falls = emit_ins(&e, &in->i);
        uint32_t end = a + in->i.length;
        if (falls && (k + 1 >= n || end != v[k + 1]->i.addr)) {
            /* fall-through to an address that is not the next emitted instruction */
            if (map_has(&f->at, end)) {
                map_put(g, &f->labels, end, 1);
                w(&e, "goto L_%04x;", (unsigned)end);
            } else {
                w(&e, "recomp_fatal(\"fell off %04x:%04x\");", e.cs, (unsigned)end);
            }
        }
    }
    line_raw(&e, "}");
}

/* ------------------------------------------------------------ output */

static GenFile *new_file(G *g, const char *name) {
    GenOutput *o = g->out;
    GROW(g, o->files, o->nfiles, g->capfiles);
    GenFile *f = &o->files[o->nfiles++];
    memset(f, 0, sizeof(*f));
    snprintf(f->name, sizeof f->name, "%s", name);
    return f;
}

static void set_file(GenFile *f, Buf *b) {
    f->data = b->p;
    f->len = b->n;
    b->p = NULL;
    b->n = b->cap = 0;
}

static void write_outputs(G *g, Func **funcs, int nf) {
    GenOutput *o = g->out;
    Buf *b = &g->buf;
    char name[32];

    bputs(g, b, "/* Generated by the recompiler (src/recomp/gen) - do not edit. */\n#pragma once\n#include \"cpu.h\"\n"
                 "#include \"api.h\"\n");
    for (int k = 0; k < nf; k++)
        bprintf(g, b, "void f_%04x_%04x(void);\n", ne_sel_of(funcs[k]->seg), (unsigned)funcs[k]->entry);
    set_file(new_file(g, "recomp_gen.h"), b);

    /* function bodies, 0x2000 bytes of each segment per file */
    for (int k = 0; k < nf;) {
        int seg = funcs[k]->seg;
        uint32_t chunk = funcs[k]->entry / 0x2000;
        bputs(g, b, "/* Generated by the recompiler (src/recomp/gen) from BLIZZARD.EXE - do not edit. */\n"
                     "#ifdef __GNUC__\n#pragma GCC diagnostic ignored \"-Wunused-label\"\n#endif\n"
                     "#include \"recomp_gen.h\"\n\n");
        int first = 1;
        for (; k < nf && funcs[k]->seg == seg && funcs[k]->entry / 0x2000 == chunk; k++) {
            if (!first) bputs(g, b, "\n\n");
            first = 0;
            emit_func(g, funcs[k], b);
        }
        bputs(g, b, "\n");
        snprintf(name, sizeof name, "gen_%04x_%u.c", ne_sel_of(seg), (unsigned)chunk);
        set_file(new_file(g, name), b);
    }

    bputs(g, b, "/* Generated function table. */\n#include \"recomp_gen.h\"\n\nconst RecompEntry recomp_table[] = {\n");
    for (int k = 0; k < nf; k++)
        bprintf(g, b, "    {0x%04x, 0x%04x, f_%04x_%04x},\n", ne_sel_of(funcs[k]->seg), (unsigned)funcs[k]->entry,
                ne_sel_of(funcs[k]->seg), (unsigned)funcs[k]->entry);
    bputs(g, b, "};\n");
    bprintf(g, b, "const unsigned recomp_table_count = %d;\n", nf);
    bprintf(g, b, "\n/* NE entry point %04x:%04x */\n", ne_sel_of(g->ne.cs), (unsigned)g->ne.ip);
    bprintf(g, b, "void recomp_start(void) { f_%04x_%04x(); }\n", ne_sel_of(g->ne.cs), (unsigned)g->ne.ip);
    set_file(new_file(g, "gen_table.c"), b);

    int nused = 0;
    for (int k = 0; k < g->nimports; k++) nused += g->imports[k].used;
    o->nimports_called = nused;

    /* every import (also ones only referenced from data), in relocation order */
    bputs(g, b, "/* All imports in relocation order: X(index, module, name, ordinal, purge) */\n");
    for (int k = 0; k < g->nimports; k++) {
        const Import *im = &g->imports[k];
        bprintf(g, b, "X(%d, %s, %s, %d, %d)\n", k, im->module, imp_name(im), im->ordinal, im->purge);
    }
    snprintf(o->import_list.name, sizeof o->import_list.name, "import_list.h");
    set_file(&o->import_list, b);
    bputs(g, b, "/* Prototypes for every imported API (generated). */\n");
    for (int k = 0; k < g->nimports; k++)
        bprintf(g, b, "void api_%s_%s(void);\n", g->imports[k].module, imp_name(&g->imports[k]));
    snprintf(o->api_decls.name, sizeof o->api_decls.name, "api_decls.h");
    set_file(&o->api_decls, b);
}

/* ------------------------------------------------------------ driver */

static void g_free(G *g, Func **funcs, int nf) {
    func_free(g->cur);
    free(g->work);
    map_free(&g->found);
    map_free(&g->good);
    if (g->inside)
        for (int n = 1; n <= g->nseg; n++) free(g->inside[n]);
    free(g->inside);
    free(g->keys);
    free(g->sorted);
    free(g->sw);
    free(g->buf.p);
    for (int k = 0; k < g->nfuncs; k++) func_free(g->funcs[k]);
    free(g->funcs);
    if (funcs)
        for (int k = 0; k < nf; k++) func_free(funcs[k]);
    free(funcs);
    if (g->entries)
        for (int n = 1; n <= g->nseg; n++) free(g->entries[n]);
    free(g->entries);
    free(g->is_code);
    map_free(&g->reloc);
    map_free(&g->done);
    map_free(&g->new_entries);
    free(g->rels);
    free(g->imports);
    ne_free(&g->ne);
}

int gen_recompile(const uint8_t *exe, size_t len, const char *functions_txt, GenOutput *out, char *err,
                  size_t errcap) {
    G *g = calloc(1, sizeof(G));
    if (!g) {
        snprintf(err, errcap, "out of memory");
        return -1;
    }
    /* volatile: modified between setjmp and a possible longjmp */
    Func **volatile funcs = NULL;
    volatile int nf = 0;
    memset(out, 0, sizeof(*out));
    g->err = err;
    g->errcap = errcap;
    g->out = out;
    if (setjmp(g->jb)) {
        g_free(g, funcs, nf);
        free(g);
        gen_output_free(out);
        return -1;
    }
    program_init(g, exe, len, functions_txt);

    /* discover functions, adding direct call targets as new entries until fixed point */
    decode_to_fixed_point(g);
    discover_pointer_entries(g);
    int ngood;
    uint32_t *keys = map_keys(g, &g->good, &ngood);
    for (int k = 0; k < ngood; k++) entry_add(g, (int)(keys[k] >> 16), keys[k] & 0xFFFF);
    if (ngood) decode_to_fixed_point(g);

    /* second pass: tail-jump detection depends on the final entry set */
    int ndone;
    keys = map_keys(g, &g->done, &ndone);
    for (int k = 0; k < g->nfuncs; k++) func_free(g->funcs[k]);
    g->nfuncs = 0;
    funcs = xcalloc(g, (size_t)ndone, sizeof(Func *));
    for (int k = 0; k < ndone; k++) {
        funcs[k] = func_decode(g, (int)(keys[k] >> 16), keys[k] & 0xFFFF);
        nf = k + 1;
    }

    write_outputs(g, funcs, nf);
    out->nfuncs = nf;
    out->nunresolved = (int)g->new_entries.n;
    g_free(g, funcs, nf);
    free(g);
    return 0;
}

void gen_output_free(GenOutput *out) {
    for (int k = 0; k < out->nfiles; k++) free(out->files[k].data);
    free(out->files);
    free(out->import_list.data);
    free(out->api_decls.data);
    memset(out, 0, sizeof(*out));
}
