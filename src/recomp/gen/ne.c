/* NE reader: see ne.h. */
#include "ne.h"

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const uint8_t *d;
    size_t n;
    char *err;
    size_t errcap;
    jmp_buf jb;
} R;

static void fail(R *r, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(r->err, r->errcap, fmt, ap);
    va_end(ap);
    longjmp(r->jb, 1);
}

static unsigned b8(R *r, size_t o) {
    if (o >= r->n) fail(r, "truncated NE file (offset 0x%zx)", o);
    return r->d[o];
}
static unsigned w16(R *r, size_t o) { return b8(r, o) | b8(r, o + 1) << 8; }

static void *xcalloc(R *r, size_t n, size_t sz) {
    void *p = calloc(n ? n : 1, sz);
    if (!p) fail(r, "out of memory");
    return p;
}

static char *impname(R *r, size_t impnames, unsigned off) {
    size_t p = impnames + off;
    unsigned n = b8(r, p);
    char *s = xcalloc(r, n + 1, 1);
    for (unsigned i = 0; i < n; i++) s[i] = (char)b8(r, p + 1 + i);
    return s;
}

static void parse_entries(R *r, NEFile *ne, size_t p, size_t length) {
    size_t end = p + length;
    unsigned ordv = 1;
    int cap = 16;
    ne->entries = xcalloc(r, (size_t)cap, sizeof(NEEntry));
    while (p < end) {
        unsigned cnt = b8(r, p);
        if (cnt == 0) break;
        unsigned ind = b8(r, p + 1);
        p += 2;
        for (unsigned k = 0; k < cnt; k++) {
            if (ind == 0) {
                ordv++;
                continue;
            }
            NEEntry e;
            e.ordinal = (uint16_t)ordv;
            if (ind == 0xFF) {
                e.seg = (uint8_t)b8(r, p + 3);
                e.off = (uint16_t)w16(r, p + 4);
                p += 6;
            } else {
                e.seg = (uint8_t)ind;
                e.off = (uint16_t)w16(r, p + 1);
                p += 3;
            }
            if (ne->nentries == cap) {
                cap *= 2;
                NEEntry *n = realloc(ne->entries, (size_t)cap * sizeof(NEEntry));
                if (!n) fail(r, "out of memory");
                ne->entries = n;
            }
            ne->entries[ne->nentries++] = e;
            ordv++;
        }
    }
}

static void parse_relocs(R *r, NEFile *ne, NESegment *seg) {
    size_t rp = seg->file_off + seg->length;
    unsigned count = w16(r, rp);
    seg->relocs = xcalloc(r, count, sizeof(NEReloc));
    seg->nrelocs = (int)count;
    for (unsigned k = 0; k < count; k++) {
        size_t q = rp + 2 + (size_t)k * 8;
        NEReloc *rl = &seg->relocs[k];
        unsigned a = w16(r, q + 4), b = w16(r, q + 6);
        rl->src_type = (uint8_t)b8(r, q);
        rl->flags = (uint8_t)b8(r, q + 1);
        unsigned loc = w16(r, q + 2);
        switch (rl->flags & 3) {
        case 0:
            if ((a & 0xFF) == 0xFF) {
                rl->ttype = NE_T_MOVABLE;   /* entry ordinal b: not supported by the generator */
                rl->off = (uint16_t)b;
            } else {
                rl->ttype = NE_T_INTERNAL;
                rl->seg = (uint16_t)(a & 0xFF);
                rl->off = (uint16_t)b;
            }
            break;
        case 1:
        case 2:
            if (a < 1 || (int)a > ne->nmod) fail(r, "relocation to module %u of %d", a, ne->nmod);
            rl->ttype = (rl->flags & 3) == 1 ? NE_T_ORD : NE_T_NAME;
            rl->module = (uint16_t)(a - 1);
            rl->off = (uint16_t)b;
            break;
        default:
            rl->ttype = NE_T_OSFIX;
            rl->seg = (uint16_t)a;
            rl->off = (uint16_t)b;
            break;
        }
        /* Walk the chain: each location holds the next one, 0xFFFF ends it. */
        int cap = 4;
        rl->locs = xcalloc(r, (size_t)cap, sizeof(uint32_t));
        uint32_t x = loc;
        uint32_t *seen = NULL;
        int nseen = 0, capseen = 0;
        for (;;) {
            if (rl->nlocs == cap) {
                cap *= 2;
                uint32_t *n = realloc(rl->locs, (size_t)cap * sizeof(uint32_t));
                if (!n) { free(seen); fail(r, "out of memory"); }
                rl->locs = n;
            }
            rl->locs[rl->nlocs++] = x;
            if (rl->flags & 4) break;
            if (x + 2 > seg->datalen) break;
            uint32_t nxt = seg->data[x] | (uint32_t)seg->data[x + 1] << 8;
            if (nxt == 0xFFFF) break;
            int loop = 0;
            for (int i = 0; i < nseen; i++)
                if (seen[i] == nxt) { loop = 1; break; }
            if (loop) break;
            if (nseen == capseen) {
                capseen = capseen ? capseen * 2 : 16;
                uint32_t *n = realloc(seen, (size_t)capseen * sizeof(uint32_t));
                if (!n) { free(seen); fail(r, "out of memory"); }
                seen = n;
            }
            seen[nseen++] = x;
            x = nxt;
        }
        free(seen);
    }
}

int ne_parse(NEFile *ne, const uint8_t *buf, size_t len, char *err, size_t errcap) {
    R r;
    r.d = buf;
    r.n = len;
    r.err = err;
    r.errcap = errcap;
    memset(ne, 0, sizeof(*ne));
    if (setjmp(r.jb)) {
        ne_free(ne);
        return -1;
    }
    size_t h = w16(&r, 0x3C);
    if (b8(&r, h) != 'N' || b8(&r, h + 1) != 'E') fail(&r, "not an NE file");
    size_t enttab = h + w16(&r, h + 0x04);
    size_t entlen = w16(&r, h + 0x06);
    ne->ip = (uint16_t)w16(&r, h + 0x14);
    ne->cs = (uint16_t)w16(&r, h + 0x16);
    int nseg = (int)w16(&r, h + 0x1C);
    int nmod = (int)w16(&r, h + 0x1E);
    size_t segtab = h + w16(&r, h + 0x22);
    size_t modref = h + w16(&r, h + 0x28);
    size_t impnames = h + w16(&r, h + 0x2A);
    unsigned align = w16(&r, h + 0x32);
    if (!align) align = 9;
    if (align > 16) fail(&r, "bad segment alignment %u", align);

    ne->nmod = nmod;
    ne->modules = xcalloc(&r, (size_t)nmod, sizeof(char *));
    for (int i = 0; i < nmod; i++) ne->modules[i] = impname(&r, impnames, w16(&r, modref + 2 * (size_t)i));

    parse_entries(&r, ne, enttab, entlen);

    ne->nseg = nseg;
    ne->segs = xcalloc(&r, (size_t)nseg + 1, sizeof(NESegment));
    for (int i = 0; i < nseg; i++) {
        size_t q = segtab + (size_t)i * 8;
        unsigned off = w16(&r, q), ln = w16(&r, q + 2), fl = w16(&r, q + 4);
        NESegment *s = &ne->segs[i + 1];
        s->file_off = (uint32_t)off << align;
        s->length = off ? (ln ? ln : 0x10000u) : 0;
        s->flags = (uint16_t)fl;
        if (off) {
            size_t fo = s->file_off;
            s->data = fo < len ? buf + fo : buf + len;
            s->datalen = fo < len ? (len - fo < s->length ? len - fo : s->length) : 0;
            if (fl & 0x100) parse_relocs(&r, ne, s);
        } else {
            s->data = buf;
            s->datalen = 0;
        }
    }
    return 0;
}

void ne_free(NEFile *ne) {
    if (ne->segs)
        for (int i = 1; i <= ne->nseg; i++) {
            for (int k = 0; k < ne->segs[i].nrelocs; k++) free(ne->segs[i].relocs[k].locs);
            free(ne->segs[i].relocs);
        }
    free(ne->segs);
    if (ne->modules)
        for (int i = 0; i < ne->nmod; i++) free(ne->modules[i]);
    free(ne->modules);
    free(ne->entries);
    memset(ne, 0, sizeof(*ne));
}
