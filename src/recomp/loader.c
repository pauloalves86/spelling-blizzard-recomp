/* NE loader for the recompiled BLIZZARD.EXE and its resource DLL.
 *
 * Loads every segment of the EXE into emulated memory at its fixed selector
 * (segment N -> 0x1000 + 8*(N-1)), applies the NE relocation records (the
 * generated code already has code-segment relocations folded in, but data in
 * code segments, e.g. vtables and jump tables, needs them in memory too), and
 * prepares the task: PSP, environment, DGROUP instance data and registers.
 */
#include "loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p) { return rd16(p) | ((uint32_t)rd16(p + 2) << 16); }

NEModule g_exe;
NEModule g_dll;

static uint8_t *read_file(const char *path, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    uint8_t *buf;
    long n;
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    n = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = (uint8_t *)malloc((size_t)n);
    if (buf && fread(buf, 1, (size_t)n, fp) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(fp);
    *len = (size_t)n;
    return buf;
}

static int parse_ne(NEModule *m, const char *path)
{
    const uint8_t *d;
    const uint8_t *ne;
    unsigned i;
    memset(m, 0, sizeof(*m));
    m->raw = read_file(path, &m->raw_len);
    if (!m->raw) {
        return -1;
    }
    d = m->raw;
    m->ne_off = rd16(d + 0x3C);
    ne = d + m->ne_off;
    if (ne[0] != 'N' || ne[1] != 'E') {
        return -1;
    }
    snprintf(m->path, sizeof(m->path), "%s", path);
    m->autodata = rd16(ne + 0x0E);
    m->heap = rd16(ne + 0x10);
    m->stack = rd16(ne + 0x12);
    m->ip = rd16(ne + 0x14);
    m->cs = rd16(ne + 0x16);
    m->sp = rd16(ne + 0x18);
    m->ss = rd16(ne + 0x1A);
    m->nseg = rd16(ne + 0x1C);
    m->nmod = rd16(ne + 0x1E);
    m->segtab = m->ne_off + rd16(ne + 0x22);
    m->rsrctab = m->ne_off + rd16(ne + 0x24);
    m->restab = m->ne_off + rd16(ne + 0x26);
    m->modref = m->ne_off + rd16(ne + 0x28);
    m->impnames = m->ne_off + rd16(ne + 0x2A);
    m->align = rd16(ne + 0x32);
    if (!m->align) {
        m->align = 9;
    }
    if (m->nseg > MAX_NE_SEGS) {
        return -1;
    }
    for (i = 0; i < m->nseg; ++i) {
        const uint8_t *s = d + m->segtab + i * 8;
        m->seg[i].file_off = (uint32_t)rd16(s) << m->align;
        m->seg[i].len = rd16(s) ? (rd16(s + 2) ? rd16(s + 2) : 0x10000u) : 0;
        m->seg[i].flags = rd16(s + 4);
        m->seg[i].minalloc = rd16(s + 6) ? rd16(s + 6) : 0x10000u;
    }
    /* module name = first resident name */
    {
        unsigned n = d[m->restab];
        if (n >= sizeof(m->name)) {
            n = sizeof(m->name) - 1;
        }
        memcpy(m->name, d + m->restab + 1, n);
        m->name[n] = 0;
    }
    return 0;
}

static void impname(const NEModule *m, uint16_t off, char *out, size_t cap)
{
    const uint8_t *p = m->raw + m->impnames + off;
    size_t n = p[0] < cap - 1 ? p[0] : cap - 1;
    memcpy(out, p + 1, n);
    out[n] = 0;
}

static void apply_relocs(NEModule *m, unsigned segi)
{
    const NESeg *sg = &m->seg[segi];
    const uint8_t *rp = m->raw + sg->file_off + sg->len;
    unsigned count = rd16(rp);
    unsigned k;
    uint32_t base = SEGBASE[sg->sel >> 3];
    rp += 2;
    for (k = 0; k < count; ++k, rp += 8) {
        uint8_t src = rp[0];
        uint8_t flags = rp[1];
        uint16_t loc = rd16(rp + 2);
        uint16_t a = rd16(rp + 4);
        uint16_t b = rd16(rp + 6);
        uint16_t sel_val = 0, off_val = 0;
        int additive = flags & 4;
        switch (flags & 3) {
            case 0: /* internal reference */
                if ((a & 0xFF) == 0xFF) {
                    recomp_fatal("movable entry reloc not supported");
                }
                sel_val = m->seg[(a & 0xFF) - 1].sel;
                off_val = b;
                break;
            case 1: { /* import by ordinal */
                char mod[16];
                int idx;
                impname(m, rd16(m->raw + m->modref + 2 * (a - 1)), mod, sizeof(mod));
                if (strcmp(mod, "KERNEL") == 0 && b == 114) { /* __AHINCR */
                    sel_val = off_val = 8;
                } else if (strcmp(mod, "KERNEL") == 0 && b == 113) { /* __AHSHIFT */
                    sel_val = off_val = 3;
                } else if (strcmp(mod, "KERNEL") == 0 && b == 178) { /* __WINFLAGS */
                    sel_val = off_val = 0x0413;
                } else {
                    idx = recomp_import_index(mod, b);
                    if (idx < 0) {
                        recomp_fatal("relocation to unknown import %s.%u", mod, b);
                    }
                    sel_val = THUNK_SEL;
                    off_val = (uint16_t)(idx * 4);
                }
                break;
            }
            default:
                recomp_fatal("unsupported relocation target type %u", flags & 3);
        }
        for (;;) {
            uint8_t *p = MEM + base + loc;
            uint16_t next = rd16(p);
            switch (src) {
                case 0: p[0] = (uint8_t)(additive ? p[0] + off_val : off_val); break;
                case 2: { uint16_t v = additive ? (uint16_t)(rd16(p) + sel_val) : sel_val; memcpy(p, &v, 2); break; }
                case 3: {
                    uint16_t o = additive ? (uint16_t)(rd16(p) + off_val) : off_val;
                    memcpy(p, &o, 2);
                    memcpy(p + 2, &sel_val, 2);
                    break;
                }
                case 5: { uint16_t v = additive ? (uint16_t)(rd16(p) + off_val) : off_val; memcpy(p, &v, 2); break; }
                default: recomp_fatal("unsupported relocation source type %u", src);
            }
            if (additive || next == 0xFFFF) {
                break;
            }
            loc = next;
        }
    }
}

int loader_load_exe(const char *path)
{
    NEModule *m = &g_exe;
    unsigned i;
    if (parse_ne(m, path) != 0) {
        return -1;
    }
    for (i = 0; i < m->nseg; ++i) {
        NESeg *sg = &m->seg[i];
        uint32_t base = 0x10000u * (i + 1);
        uint32_t size = sg->minalloc;
        sg->sel = EXE_SEL(i + 1);
        if (i + 1 == m->autodata) {
            size = 0x10000u; /* DGROUP gets the full 64K; stack at the documented top */
        }
        sel_set(sg->sel, base, size - 1);
        memset(MEM + base, 0, 0x10000u);
        if (sg->len) {
            memcpy(MEM + base, m->raw + sg->file_off, sg->len);
        }
    }
    for (i = 0; i < m->nseg; ++i) {
        if (m->seg[i].len && (m->seg[i].flags & 0x100)) {
            apply_relocs(m, i);
        }
    }
    /* import thunk "segment": never executed, only used as far-pointer values */
    sel_set(THUNK_SEL, 0xF0000u, 0xFFFF);
    m->hmodule = 0x0E00; /* hModule distinct from hInstance (== DGROUP selector) */
    return 0;
}

/* Load a resource-only DLL (SB_CDRES.DLL). Its code (if any) is never run;
 * only its resource table is used, read straight from the file. */
int loader_load_dll(const char *path, uint16_t hmodule)
{
    if (parse_ne(&g_dll, path) != 0) {
        return -1;
    }
    g_dll.hmodule = hmodule;
    return 0;
}

uint16_t g_psp_sel;
uint16_t g_env_sel;

void loader_setup_task(const char *cmdline, uint16_t ncmdshow)
{
    NEModule *m = &g_exe;
    uint16_t dg = m->seg[m->autodata - 1].sel;
    uint32_t dgsize = m->seg[m->autodata - 1].minalloc + m->heap + m->stack;
    static const char env[] = "PATH=C:\\WINDOWS\0COMSPEC=C:\\COMMAND.COM\0\0\1\0C:\\SB\\BLIZZARD.EXE\0";
    uint8_t *psp;
    size_t n;

    if (dgsize > 0xFFFE) {
        dgsize = 0xFFFE;
    }
    dgsize &= ~1u;
    /* PSP and environment */
    g_env_sel = global_alloc(0, 256);
    memcpy(FARPTR(g_env_sel, 0), env, sizeof(env));
    g_psp_sel = global_alloc(0, 256);
    psp = (uint8_t *)FARPTR(g_psp_sel, 0);
    psp[0] = 0xCD;
    psp[1] = 0x20;
    memcpy(psp + 0x2C, &g_env_sel, 2);
    n = strlen(cmdline);
    if (n > 125) {
        n = 125;
    }
    psp[0x80] = (uint8_t)n;
    memcpy(psp + 0x81, cmdline, n);
    psp[0x81 + n] = 0x0D;

    memset(&cpu, 0, sizeof(cpu));
    cpu.ifl = 1;
    SS = dg;
    DS = dg;
    ES = g_psp_sel;
    SP = (uint16_t)(m->sp ? m->sp : dgsize);
    BX = m->stack;  /* stack size */
    CX = m->heap;   /* local heap size */
    DI = dg;        /* hInstance */
    SI = 0;         /* hPrevInstance */
    m->ncmdshow = ncmdshow;
    m->dgroup_size = (uint16_t)dgsize;
}
