/* Host GDI for the recompiled game.
 *
 * Emulates a Windows 3.1 256-color palette display: device-dependent bitmaps
 * hold system-palette indices, the system palette has the 20 static colors
 * at 0-9 / 246-255, RealizePalette assigns logical entries to free slots,
 * and AnimatePalette changes PC_RESERVED slots in place. BitBlt/StretchBlt
 * evaluate full ROP3 codes on palette indices, with the documented
 * mono<->color conversion rules (text/background colors). Text is rendered
 * with FreeType from metric-compatible Liberation fonts, without antialiasing.
 */
#include "gdi.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "api.h"
#include "win16.h"

uint8_t g_screen[SCREEN_W * SCREEN_H];
RGBQ g_syspal[256];
int g_screen_dirty = 1;

/* ------------------------------------------------------------ objects */

enum { OBJ_FREE, OBJ_PEN, OBJ_BRUSH, OBJ_FONT, OBJ_PAL, OBJ_BITMAP, OBJ_RGN, OBJ_DC };

typedef struct Font {
    int height;    /* CreateFont height (signed) */
    int weight;
    int italic, underline;
    char face[40];
    FT_Face ft;
    int em;        /* pixel em size */
    int ascent, descent, cell;
} Font;

typedef struct Palette {
    int n;
    RGBQ e[256];
    uint8_t map[256]; /* logical -> system index */
} Palette;

typedef struct Bitmap {
    int w, h, bpp; /* bpp 1 or 8 */
    uint8_t *px;   /* 1 byte per pixel */
} Bitmap;

typedef struct Rgn {
    int x0, y0, x1, y1; /* bbox, exclusive */
    uint8_t *m;
} Rgn;

typedef struct DC {
    uint16_t hwnd;     /* window DC if nonzero */
    uint16_t bitmap;   /* memory DC */
    int org_x, org_y;
    uint32_t text_color, bk_color;
    int bk_mode;       /* 1 transparent, 2 opaque */
    uint16_t text_align;
    int stretch_mode;
    uint16_t font, brush, pen, pal;
    Rgn *clip;
    int map_mode;
} DC;

typedef struct Obj {
    int type;
    int stock;
    union {
        uint32_t color; /* brush/pen */
        Font *font;
        Palette *pal;
        Bitmap *bmp;
        Rgn *rgn;
        DC *dc;
    } u;
} Obj;

#define MAX_OBJ 8192
#define H_BASE 0x4000u
static Obj g_obj[MAX_OBJ];
static uint16_t g_stock[20];
static uint16_t g_default_bitmap;

static uint16_t obj_new(int type)
{
    int i;
    for (i = 1; i < MAX_OBJ; ++i) {
        if (g_obj[i].type == OBJ_FREE) {
            memset(&g_obj[i], 0, sizeof(Obj));
            g_obj[i].type = type;
            return (uint16_t)(H_BASE + i * 4);
        }
    }
    recomp_fatal("GDI object table full");
}

static Obj *obj_get(uint16_t h, int type)
{
    unsigned i;
    if (h < H_BASE || (h - H_BASE) % 4) {
        return NULL;
    }
    i = (h - H_BASE) / 4;
    if (i >= MAX_OBJ || g_obj[i].type == OBJ_FREE || (type && g_obj[i].type != type)) {
        return NULL;
    }
    return &g_obj[i];
}

static DC *dc_get(uint16_t h)
{
    Obj *o = obj_get(h, OBJ_DC);
    return o ? o->u.dc : NULL;
}

/* ------------------------------------------------------------ palette */

static const RGBQ k_static[20] = {
    {0, 0, 0, 0}, {128, 0, 0, 0}, {0, 128, 0, 0}, {128, 128, 0, 0}, {0, 0, 128, 0},
    {128, 0, 128, 0}, {0, 128, 128, 0}, {192, 192, 192, 0}, {192, 220, 192, 0}, {166, 202, 240, 0},
    {255, 251, 240, 0}, {160, 160, 164, 0}, {128, 128, 128, 0}, {255, 0, 0, 0}, {0, 255, 0, 0},
    {255, 255, 0, 0}, {0, 0, 255, 0}, {255, 0, 255, 0}, {0, 255, 255, 0}, {255, 255, 255, 0},
};
static uint16_t g_realized_pal; /* foreground palette */

static int is_static(int i) { return i < 10 || i >= 246; }

static int nearest_static(uint8_t r, uint8_t g, uint8_t b)
{
    int best = 0, bd = 1 << 30, i;
    for (i = 0; i < 256; ++i) {
        int d;
        if (!is_static(i)) {
            continue;
        }
        d = (g_syspal[i].r - r) * (g_syspal[i].r - r) + (g_syspal[i].g - g) * (g_syspal[i].g - g) +
            (g_syspal[i].b - b) * (g_syspal[i].b - b);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static int nearest_logical(const Palette *p, uint8_t r, uint8_t g, uint8_t b)
{
    int best = 0, bd = 1 << 30, i;
    for (i = 0; i < p->n; ++i) {
        int d = (p->e[i].r - r) * (p->e[i].r - r) + (p->e[i].g - g) * (p->e[i].g - g) +
                (p->e[i].b - b) * (p->e[i].b - b);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static Palette *pal_of(uint16_t h)
{
    Obj *o = obj_get(h, OBJ_PAL);
    return o ? o->u.pal : NULL;
}

static int realize(uint16_t hpal)
{
    Palette *p = pal_of(hpal);
    int i, next = 10, changed = 0;
    if (!p || hpal == g_stock[15]) {
        return 0;
    }
    for (i = 0; i < p->n; ++i) {
        const RGBQ *e = &p->e[i];
        if (e->flags & 0x02) { /* PC_EXPLICIT */
            p->map[i] = (uint8_t)(e->r | (e->g << 8));
            continue;
        }
        if (!(e->flags & 0x05)) { /* not PC_RESERVED / PC_NOCOLLAPSE: collapse onto static colors */
            int s = nearest_static(e->r, e->g, e->b);
            if (g_syspal[s].r == e->r && g_syspal[s].g == e->g && g_syspal[s].b == e->b) {
                p->map[i] = (uint8_t)s;
                continue;
            }
        }
        if (next > 245) {
            p->map[i] = (uint8_t)nearest_static(e->r, e->g, e->b);
            continue;
        }
        if (g_syspal[next].r != e->r || g_syspal[next].g != e->g || g_syspal[next].b != e->b) {
            changed++;
        }
        g_syspal[next] = *e;
        p->map[i] = (uint8_t)next++;
    }
    g_realized_pal = hpal;
    g_screen_dirty = 1;
    return changed;
}

/* COLORREF -> device index for a DC (8-bit) */
static uint8_t color_index8(DC *dc, uint32_t cr)
{
    Palette *p = pal_of(dc->pal);
    uint8_t r = (uint8_t)cr, g = (uint8_t)(cr >> 8), b = (uint8_t)(cr >> 16);
    switch (cr >> 24) {
        case 1: {
            unsigned n = cr & 0xFFFF;
            if (p && n < (unsigned)p->n) {
                return p->map[n];
            }
            return 0;
        }
        case 2:
            if (p && dc->pal != g_stock[15]) {
                return p->map[nearest_logical(p, r, g, b)];
            }
            return (uint8_t)nearest_static(r, g, b);
        default:
            return (uint8_t)nearest_static(r, g, b);
    }
}

static int cr_is_white(DC *dc, uint32_t cr)
{
    uint8_t r = (uint8_t)cr, g = (uint8_t)(cr >> 8), b = (uint8_t)(cr >> 16);
    if ((cr >> 24) == 1) {
        Palette *p = pal_of(dc->pal);
        unsigned n = cr & 0xFFFF;
        if (p && n < (unsigned)p->n) {
            r = p->e[n].r; g = p->e[n].g; b = p->e[n].b;
        }
    }
    return (r + g + b) >= 3 * 128;
}

/* ------------------------------------------------------------ surfaces */

typedef struct Surf {
    uint8_t *px;
    int w, h, mono;
    int ox, oy;            /* device (0,0) in surface coordinates */
    int cx0, cy0, cx1, cy1; /* clip in surface coordinates */
} Surf;

static Bitmap *bmp_of(uint16_t h)
{
    Obj *o = obj_get(h, OBJ_BITMAP);
    return o ? o->u.bmp : NULL;
}

static int dc_surf(DC *dc, Surf *s)
{
    memset(s, 0, sizeof(*s));
    if (dc->hwnd) {
        int x, y, w, h;
        if (!user_client_area(dc->hwnd, &x, &y, &w, &h)) {
            return 0;
        }
        s->px = g_screen;
        s->w = SCREEN_W;
        s->h = SCREEN_H;
        s->ox = x;
        s->oy = y;
        s->cx0 = x < 0 ? 0 : x;
        s->cy0 = y < 0 ? 0 : y;
        s->cx1 = x + w > SCREEN_W ? SCREEN_W : x + w;
        s->cy1 = y + h > SCREEN_H ? SCREEN_H : y + h;
        g_screen_dirty = 1;
        return 1;
    } else {
        Bitmap *b = bmp_of(dc->bitmap);
        if (!b) {
            return 0;
        }
        s->px = b->px;
        s->w = b->w;
        s->h = b->h;
        s->mono = b->bpp == 1;
        s->cx1 = b->w;
        s->cy1 = b->h;
        return 1;
    }
}

static int clip_ok(DC *dc, const Surf *s, int sx, int sy)
{
    Rgn *r = dc->clip;
    int dx, dy;
    if (!r) {
        return 1;
    }
    dx = sx - s->ox;
    dy = sy - s->oy;
    if (dx < r->x0 || dy < r->y0 || dx >= r->x1 || dy >= r->y1) {
        return 0;
    }
    return r->m[(dy - r->y0) * (r->x1 - r->x0) + (dx - r->x0)];
}

static uint8_t dc_color(DC *dc, const Surf *s, uint32_t cr)
{
    return s->mono ? (uint8_t)cr_is_white(dc, cr) : color_index8(dc, cr);
}

uint32_t gdi_brush_color(uint16_t hbr, int *ok)
{
    Obj *o = obj_get(hbr, OBJ_BRUSH);
    *ok = 1;
    if (o) {
        if (o->u.color == 0xFFFFFFFFu) {
            *ok = 0;
        }
        return o->u.color;
    }
    if (hbr >= 1 && hbr <= 21) { /* COLOR_xxx + 1 system colors */
        static const uint32_t syscol[21] = {
            0xC0C0C0, 0x808000, 0x800000, 0x808080, 0xFFFFFF, 0xFFFFFF, 0x000000, 0x000000,
            0x000000, 0xC0C0C0, 0xC0C0C0, 0x808080, 0x800000, 0xFFFFFF, 0xC0C0C0, 0x808080,
            0x808080, 0x000000, 0xC0C0C0, 0xFFFFFF, 0x000000};
        return syscol[hbr - 1];
    }
    *ok = 0;
    return 0;
}

/* ------------------------------------------------------------ ROP3 */

static inline uint8_t rop3(uint8_t rop, uint8_t p, uint8_t s, uint8_t d)
{
    uint8_t r = 0;
    int m;
    switch (rop) {
        case 0xCC: return s;
        case 0x88: return s & d;
        case 0xEE: return s | d;
        case 0x66: return s ^ d;
        case 0xF0: return p;
        case 0x00: return 0;
        case 0xFF: return 0xFF;
        case 0x55: return (uint8_t)~d;
        case 0x33: return (uint8_t)~s;
        case 0xAA: return d;
        case 0x22: return (uint8_t)(~s & d);
        case 0xBB: return (uint8_t)(~s | d);
        case 0x44: return (uint8_t)(s & ~d);
        case 0xB8: return (uint8_t)((p & s) | (~s & d)); /* PSDPxax */
        case 0xE2: return (uint8_t)((d & s) | (~s & p)); /* DSPDxax */
        case 0x5A: return p ^ d;
        default: break;
    }
    for (m = 0; m < 8; ++m) {
        if (rop & (1 << m)) {
            uint8_t a = (m & 4) ? p : (uint8_t)~p;
            uint8_t b = (m & 2) ? s : (uint8_t)~s;
            uint8_t c = (m & 1) ? d : (uint8_t)~d;
            r |= a & b & c;
        }
    }
    return r;
}

static int rop_uses_src(uint8_t rop) { return ((rop >> 2) & 0x33) != (rop & 0x33); }
static int rop_uses_pat(uint8_t rop) { return ((rop >> 4) & 0x0F) != (rop & 0x0F); }

/* Generic (stretch) blit. Coordinates are logical. */
static void do_blit(uint16_t hdst, int x, int y, int w, int h, uint16_t hsrc, int sx, int sy, int sw, int sh,
                    uint32_t dwrop)
{
    DC *dd = dc_get(hdst), *sd = hsrc ? dc_get(hsrc) : NULL;
    Surf ds, ss;
    uint8_t rop = (uint8_t)(dwrop >> 16);
    int use_src = rop_uses_src(rop), use_pat = rop_uses_pat(rop);
    uint8_t pat = 0, s_fg = 0, s_bg = 0, src_bk = 0;
    int i, j;
    int mirror_x = 0, mirror_y = 0;
    if (!dd || !dc_surf(dd, &ds)) {
        return;
    }
    if (use_src) {
        if (!sd || !dc_surf(sd, &ss)) {
            return;
        }
    }
    if (use_pat) {
        int ok;
        uint32_t c = gdi_brush_color(dd->brush, &ok);
        pat = ok ? dc_color(dd, &ds, c) : 0;
    }
    if (use_src && ss.mono && !ds.mono) {
        s_fg = color_index8(dd, dd->text_color); /* 0 bits */
        s_bg = color_index8(dd, dd->bk_color);   /* 1 bits */
    }
    if (use_src && !ss.mono && ds.mono) {
        src_bk = color_index8(sd, sd->bk_color);
    }
    /* logical -> device */
    x -= dd->org_x;
    y -= dd->org_y;
    if (use_src) {
        sx -= sd->org_x;
        sy -= sd->org_y;
    }
    if (w < 0) { x += w; w = -w; mirror_x ^= 1; }
    if (h < 0) { y += h; h = -h; mirror_y ^= 1; }
    if (sw < 0) { sx += sw; sw = -sw; mirror_x ^= 1; }
    if (sh < 0) { sy += sh; sh = -sh; mirror_y ^= 1; }
    if (!w || !h || (use_src && (!sw || !sh))) {
        return;
    }
    for (j = 0; j < h; ++j) {
        int ty = ds.oy + y + j;
        int syy;
        if (ty < ds.cy0 || ty >= ds.cy1) {
            continue;
        }
        syy = (int)(((long)(mirror_y ? h - 1 - j : j) * sh) / h);
        for (i = 0; i < w; ++i) {
            int tx = ds.ox + x + i;
            uint8_t s = 0, d, r;
            uint8_t *dp;
            if (tx < ds.cx0 || tx >= ds.cx1) {
                continue;
            }
            if (!clip_ok(dd, &ds, tx, ty)) {
                continue;
            }
            if (use_src) {
                int sxx = (int)(((long)(mirror_x ? w - 1 - i : i) * sw) / w);
                int px = ss.ox + sx + sxx, py = ss.oy + sy + syy;
                if (px < 0 || py < 0 || px >= ss.w || py >= ss.h) {
                    continue;
                }
                s = ss.px[py * ss.w + px];
                if (ss.mono && !ds.mono) {
                    s = s ? s_bg : s_fg;
                } else if (!ss.mono && ds.mono) {
                    s = s == src_bk ? 1 : 0;
                }
            }
            dp = &ds.px[ty * ds.w + tx];
            d = *dp;
            if (ds.mono) {
                uint8_t sm = s ? 0xFF : 0, dm = d ? 0xFF : 0, pm = pat ? 0xFF : 0;
                r = (uint8_t)(rop3(rop, pm, sm, dm) & 1);
            } else {
                r = rop3(rop, pat, s, d);
            }
            *dp = r;
        }
    }
}

/* ------------------------------------------------------------ regions */

static Rgn *rgn_of(uint16_t h)
{
    Obj *o = obj_get(h, OBJ_RGN);
    return o ? o->u.rgn : NULL;
}

static Rgn *rgn_new_rect(int x0, int y0, int x1, int y1)
{
    Rgn *r = (Rgn *)calloc(1, sizeof(Rgn));
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { int t = y0; y0 = y1; y1 = t; }
    /* clamp absurd extents */
    if (x0 < -2048) x0 = -2048;
    if (y0 < -2048) y0 = -2048;
    if (x1 > 4096) x1 = 4096;
    if (y1 > 4096) y1 = 4096;
    if (x1 <= x0 || y1 <= y0) {
        x0 = y0 = x1 = y1 = 0;
    }
    r->x0 = x0; r->y0 = y0; r->x1 = x1; r->y1 = y1;
    r->m = (uint8_t *)malloc((size_t)((x1 - x0) * (y1 - y0)) + 1);
    memset(r->m, 1, (size_t)((x1 - x0) * (y1 - y0)));
    return r;
}

static int rgn_get(const Rgn *r, int x, int y)
{
    if (x < r->x0 || y < r->y0 || x >= r->x1 || y >= r->y1) {
        return 0;
    }
    return r->m[(y - r->y0) * (r->x1 - r->x0) + (x - r->x0)];
}

static void rgn_free(Rgn *r)
{
    if (r) {
        free(r->m);
        free(r);
    }
}

static Rgn *rgn_copy(const Rgn *s)
{
    Rgn *r = (Rgn *)calloc(1, sizeof(Rgn));
    size_t n = (size_t)((s->x1 - s->x0) * (s->y1 - s->y0));
    *r = *s;
    r->m = (uint8_t *)malloc(n + 1);
    memcpy(r->m, s->m, n);
    return r;
}

/* shrink bbox to content; return region type */
static int rgn_normalize(Rgn *r)
{
    int x, y, nx0 = r->x1, ny0 = r->y1, nx1 = r->x0, ny1 = r->y0, w = r->x1 - r->x0;
    long count = 0;
    for (y = r->y0; y < r->y1; ++y) {
        for (x = r->x0; x < r->x1; ++x) {
            if (r->m[(y - r->y0) * w + (x - r->x0)]) {
                if (x < nx0) nx0 = x;
                if (y < ny0) ny0 = y;
                if (x + 1 > nx1) nx1 = x + 1;
                if (y + 1 > ny1) ny1 = y + 1;
                count++;
            }
        }
    }
    if (!count) {
        free(r->m);
        r->x0 = r->y0 = r->x1 = r->y1 = 0;
        r->m = (uint8_t *)malloc(1);
        return 1; /* NULLREGION */
    }
    if (nx0 != r->x0 || ny0 != r->y0 || nx1 != r->x1 || ny1 != r->y1) {
        int nw = nx1 - nx0;
        uint8_t *m = (uint8_t *)malloc((size_t)(nw * (ny1 - ny0)) + 1);
        for (y = ny0; y < ny1; ++y) {
            memcpy(m + (y - ny0) * nw, r->m + (y - r->y0) * w + (nx0 - r->x0), (size_t)nw);
        }
        free(r->m);
        r->m = m;
        r->x0 = nx0; r->y0 = ny0; r->x1 = nx1; r->y1 = ny1;
    }
    return count == (long)(r->x1 - r->x0) * (r->y1 - r->y0) ? 2 : 3;
}

static int rgn_combine(Rgn *dst, const Rgn *a, const Rgn *b, int mode)
{
    int x0, y0, x1, y1, x, y, w;
    uint8_t *m;
    if (mode == 5) { /* RGN_COPY */
        Rgn *c = rgn_copy(a);
        free(dst->m);
        *dst = *c;
        free(c);
        return rgn_normalize(dst);
    }
    x0 = a->x0 < b->x0 ? a->x0 : b->x0;
    y0 = a->y0 < b->y0 ? a->y0 : b->y0;
    x1 = a->x1 > b->x1 ? a->x1 : b->x1;
    y1 = a->y1 > b->y1 ? a->y1 : b->y1;
    if (a->x1 <= a->x0) { x0 = b->x0; y0 = b->y0; x1 = b->x1; y1 = b->y1; }
    if (b->x1 <= b->x0 && a->x1 > a->x0) { x0 = a->x0; y0 = a->y0; x1 = a->x1; y1 = a->y1; }
    if (x1 < x0) x1 = x0;
    if (y1 < y0) y1 = y0;
    w = x1 - x0;
    m = (uint8_t *)malloc((size_t)(w * (y1 - y0)) + 1);
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1; ++x) {
            int va = rgn_get(a, x, y), vb = rgn_get(b, x, y), v;
            switch (mode) {
                case 1: v = va && vb; break;
                case 2: v = va || vb; break;
                case 3: v = va != vb; break;
                case 4: v = va && !vb; break;
                default: v = va; break;
            }
            m[(y - y0) * w + (x - x0)] = (uint8_t)v;
        }
    }
    free(dst->m);
    dst->m = m;
    dst->x0 = x0; dst->y0 = y0; dst->x1 = x1; dst->y1 = y1;
    return rgn_normalize(dst);
}

/* ------------------------------------------------------------ fonts */

static FT_Library g_ft;
static char g_font_dir[512];
typedef struct FaceCache { char file[64]; FT_Face face; } FaceCache;
static FaceCache g_faces[16];

static FT_Face load_face(const char *file)
{
    int i;
    char path[1024];
    for (i = 0; i < 16 && g_faces[i].face; ++i) {
        if (!strcmp(g_faces[i].file, file)) {
            return g_faces[i].face;
        }
    }
    if (i == 16) {
        return g_faces[0].face;
    }
    snprintf(path, sizeof(path), "%s/%s", g_font_dir, file);
    if (FT_New_Face(g_ft, path, 0, &g_faces[i].face) != 0) {
        recomp_fatal("cannot load font %s (set SB_FONT_DIR)", path);
    }
    snprintf(g_faces[i].file, sizeof(g_faces[i].file), "%s", file);
    return g_faces[i].face;
}

static void font_setup(Font *f)
{
    const char *base = "LiberationSans";
    const char *style;
    char file[64];
    TT_OS2 *os2;
    int upem, wa, wd;
    if (!strncasecmp(f->face, "Times", 5)) {
        base = "LiberationSerif";
    } else if (!strncasecmp(f->face, "Courier", 7)) {
        base = "LiberationMono";
    }
    if (strstr(f->face, "Bold")) {
        f->weight = 700;
    }
    if (f->weight >= 600) {
        style = f->italic ? "BoldItalic" : "Bold";
    } else {
        style = f->italic ? "Italic" : "Regular";
    }
    snprintf(file, sizeof(file), "%s-%s.ttf", base, style);
    f->ft = load_face(file);
    upem = f->ft->units_per_EM;
    os2 = (TT_OS2 *)FT_Get_Sfnt_Table(f->ft, FT_SFNT_OS2);
    wa = os2 ? os2->usWinAscent : f->ft->ascender;
    wd = os2 ? os2->usWinDescent : -f->ft->descender;
    if (f->height == 0) {
        f->height = -12;
    }
    if (f->height < 0) {
        f->em = -f->height;
    } else {
        f->em = (int)lround((double)f->height * upem / (wa + wd));
    }
    if (f->em < 1) {
        f->em = 1;
    }
    f->ascent = (int)lround((double)wa * f->em / upem);
    f->descent = (int)lround((double)wd * f->em / upem);
    f->cell = f->ascent + f->descent;
}

static Font *font_of(DC *dc)
{
    Obj *o = obj_get(dc->font, OBJ_FONT);
    if (!o) {
        o = obj_get(g_stock[13], OBJ_FONT);
    }
    return o->u.font;
}

static unsigned cp1252(unsigned char c)
{
    static const unsigned short hi[32] = {
        0x20AC, 0x81, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x8D,
        0x017D, 0x8F, 0x90, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A,
        0x0153, 0x9D, 0x017E, 0x0178};
    return (c >= 0x80 && c < 0xA0) ? hi[c - 0x80] : c;
}

static int glyph_advance(Font *f, unsigned char c)
{
    FT_Set_Pixel_Sizes(f->ft, 0, (FT_UInt)f->em);
    if (FT_Load_Char(f->ft, cp1252(c), FT_LOAD_DEFAULT | FT_LOAD_TARGET_MONO) != 0) {
        return 0;
    }
    return (int)((f->ft->glyph->advance.x + 32) >> 6);
}

static int text_width(Font *f, const unsigned char *s, int n)
{
    int w = 0, i;
    for (i = 0; i < n; ++i) {
        w += glyph_advance(f, s[i]);
    }
    return w;
}

static void draw_text(DC *dc, int x, int y, const unsigned char *s, int n)
{
    Surf sf;
    Font *f = font_of(dc);
    int w = text_width(f, s, n), i, pen;
    uint8_t fg, bg;
    int align = dc->text_align;
    if (!dc_surf(dc, &sf)) {
        return;
    }
    fg = dc_color(dc, &sf, dc->text_color);
    bg = dc_color(dc, &sf, dc->bk_color);
    x -= dc->org_x;
    y -= dc->org_y;
    if ((align & 6) == 6) {
        x -= w / 2;
    } else if (align & 2) {
        x -= w;
    }
    if ((align & 24) == 24) {
        y -= f->ascent;
    } else if (align & 8) {
        y -= f->cell;
    }
    /* opaque background box */
    if (dc->bk_mode == 2) {
        int yy, xx;
        for (yy = y; yy < y + f->cell; ++yy) {
            for (xx = x; xx < x + w; ++xx) {
                int tx = sf.ox + xx, ty = sf.oy + yy;
                if (tx >= sf.cx0 && tx < sf.cx1 && ty >= sf.cy0 && ty < sf.cy1 && clip_ok(dc, &sf, tx, ty)) {
                    sf.px[ty * sf.w + tx] = bg;
                }
            }
        }
    }
    FT_Set_Pixel_Sizes(f->ft, 0, (FT_UInt)f->em);
    pen = x;
    for (i = 0; i < n; ++i) {
        FT_GlyphSlot g;
        int gx, gy;
        if (FT_Load_Char(f->ft, cp1252(s[i]), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) != 0) {
            continue;
        }
        g = f->ft->glyph;
        for (gy = 0; gy < (int)g->bitmap.rows; ++gy) {
            for (gx = 0; gx < (int)g->bitmap.width; ++gx) {
                const uint8_t *row = g->bitmap.buffer + gy * g->bitmap.pitch;
                int on = (g->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) ? (row[gx >> 3] >> (7 - (gx & 7))) & 1
                                                                       : row[gx] >= 128;
                int tx = sf.ox + pen + g->bitmap_left + gx;
                int ty = sf.oy + y + f->ascent - g->bitmap_top + gy;
                if (on && tx >= sf.cx0 && tx < sf.cx1 && ty >= sf.cy0 && ty < sf.cy1 && clip_ok(dc, &sf, tx, ty)) {
                    sf.px[ty * sf.w + tx] = fg;
                }
            }
        }
        pen += (int)((g->advance.x + 32) >> 6);
    }
    if (f->underline) {
        int xx, ty = sf.oy + y + f->ascent + 1;
        for (xx = x; xx < x + w; ++xx) {
            int tx = sf.ox + xx;
            if (tx >= sf.cx0 && tx < sf.cx1 && ty >= sf.cy0 && ty < sf.cy1) {
                sf.px[ty * sf.w + tx] = fg;
            }
        }
    }
}

/* ------------------------------------------------------------ init */

static uint16_t make_brush(uint32_t c, int stock)
{
    uint16_t h = obj_new(OBJ_BRUSH);
    obj_get(h, 0)->u.color = c;
    obj_get(h, 0)->stock = stock;
    return h;
}

static uint16_t make_font(const char *face, int height, int weight, int stock)
{
    uint16_t h = obj_new(OBJ_FONT);
    Font *f = (Font *)calloc(1, sizeof(Font));
    snprintf(f->face, sizeof(f->face), "%s", face);
    f->height = height;
    f->weight = weight;
    font_setup(f);
    obj_get(h, 0)->u.font = f;
    obj_get(h, 0)->stock = stock;
    return h;
}

void gdi_init(const char *font_dir)
{
    int i;
    Palette *p;
    uint16_t h;
    snprintf(g_font_dir, sizeof(g_font_dir), "%s", font_dir);
    FT_Init_FreeType(&g_ft);
    memset(g_obj, 0, sizeof(g_obj));
    for (i = 0; i < 256; ++i) {
        g_syspal[i].r = g_syspal[i].g = g_syspal[i].b = 0;
    }
    for (i = 0; i < 10; ++i) {
        g_syspal[i] = k_static[i];
        g_syspal[246 + i] = k_static[10 + i];
    }
    g_stock[0] = make_brush(0xFFFFFF, 1);
    g_stock[1] = make_brush(0xC0C0C0, 1);
    g_stock[2] = make_brush(0x808080, 1);
    g_stock[3] = make_brush(0x404040, 1);
    g_stock[4] = make_brush(0x000000, 1);
    g_stock[5] = make_brush(0xFFFFFFFFu, 1);
    for (i = 6; i <= 8; ++i) {
        g_stock[i] = obj_new(OBJ_PEN);
        obj_get(g_stock[i], 0)->stock = 1;
    }
    g_stock[10] = make_font("Courier New", 12, 400, 1);
    g_stock[11] = make_font("Courier New", 13, 400, 1);
    g_stock[12] = make_font("Arial", 13, 400, 1);
    g_stock[13] = make_font("Arial", 16, 700, 1); /* SYSTEM_FONT */
    g_stock[14] = g_stock[13];
    g_stock[16] = g_stock[10];
    h = obj_new(OBJ_PAL);
    p = (Palette *)calloc(1, sizeof(Palette));
    p->n = 20;
    for (i = 0; i < 20; ++i) {
        p->e[i] = k_static[i];
        p->map[i] = (uint8_t)(i < 10 ? i : 236 + i);
    }
    obj_get(h, 0)->u.pal = p;
    obj_get(h, 0)->stock = 1;
    g_stock[15] = h;
    g_default_bitmap = obj_new(OBJ_BITMAP);
    {
        Bitmap *b = (Bitmap *)calloc(1, sizeof(Bitmap));
        b->w = b->h = 1;
        b->bpp = 1;
        b->px = (uint8_t *)calloc(1, 1);
        obj_get(g_default_bitmap, 0)->u.bmp = b;
        obj_get(g_default_bitmap, 0)->stock = 1;
    }
    memset(g_screen, 0, sizeof(g_screen));
}

void gdi_shutdown(void) {}

static uint16_t dc_new(uint16_t hwnd)
{
    uint16_t h = obj_new(OBJ_DC);
    DC *dc = (DC *)calloc(1, sizeof(DC));
    dc->hwnd = hwnd;
    dc->bitmap = g_default_bitmap;
    dc->text_color = 0x000000;
    dc->bk_color = 0xFFFFFF;
    dc->bk_mode = 2;
    dc->stretch_mode = 1;
    dc->font = g_stock[13];
    dc->brush = g_stock[0];
    dc->pen = g_stock[7];
    dc->pal = g_stock[15];
    dc->map_mode = 1;
    obj_get(h, 0)->u.dc = dc;
    return h;
}

uint16_t gdi_create_window_dc(uint16_t hwnd) { return dc_new(hwnd); }

static void dc_free(uint16_t h)
{
    Obj *o = obj_get(h, OBJ_DC);
    if (!o) {
        return;
    }
    rgn_free(o->u.dc->clip);
    free(o->u.dc);
    o->type = OBJ_FREE;
}

void gdi_release_window_dc(uint16_t hdc) { dc_free(hdc); }

void gdi_fill_screen_rect(int x0, int y0, int x1, int y1, uint32_t c, int is_brush)
{
    int x, y, ok = 1;
    uint8_t idx;
    if (is_brush) {
        c = gdi_brush_color((uint16_t)c, &ok);
        if (!ok) {
            return;
        }
    }
    idx = (uint8_t)nearest_static((uint8_t)c, (uint8_t)(c >> 8), (uint8_t)(c >> 16));
    for (y = y0 < 0 ? 0 : y0; y < y1 && y < SCREEN_H; ++y) {
        for (x = x0 < 0 ? 0 : x0; x < x1 && x < SCREEN_W; ++x) {
            g_screen[y * SCREEN_W + x] = idx;
        }
    }
    g_screen_dirty = 1;
}

/* ============================================================ API */

#define RECT_AT(fp) ((int16_t *)farp(fp))

void api_GDI_GETSTOCKOBJECT(void)
{
    ArgReader a = args_pascal(2);
    int16_t n = arg_i(&a);
    AX = (n >= 0 && n < 20) ? g_stock[n] : 0;
    api_ret(2);
}

void api_GDI_CREATECOMPATIBLEDC(void)
{
    ArgReader a = args_pascal(2);
    (void)arg_w(&a);
    AX = dc_new(0);
    api_ret(2);
}

void api_GDI_DELETEDC(void)
{
    ArgReader a = args_pascal(2);
    dc_free(arg_w(&a));
    AX = 1;
    api_ret(2);
}

void api_GDI_CREATEBITMAP(void)
{
    ArgReader a = args_pascal(12);
    int16_t w = arg_i(&a), h = arg_i(&a);
    uint8_t planes = (uint8_t)arg_w(&a), bpp = (uint8_t)arg_w(&a);
    uint32_t bits = arg_d(&a);
    uint16_t hb;
    Bitmap *b;
    if (w <= 0 || h <= 0 || planes != 1 || (bpp != 1 && bpp != 8)) {
        if (w <= 0 || h <= 0) {
            AX = 0;
            api_ret(12);
            return;
        }
        recomp_fatal("CreateBitmap(%d, %d, planes %u, bpp %u) unsupported", w, h, planes, bpp);
    }
    hb = obj_new(OBJ_BITMAP);
    b = (Bitmap *)calloc(1, sizeof(Bitmap));
    b->w = w;
    b->h = h;
    b->bpp = bpp;
    b->px = (uint8_t *)calloc((size_t)w * (size_t)h, 1);
    obj_get(hb, 0)->u.bmp = b;
    AX = hb;
    api_ret(12);
    if (bits) {
        /* same as SetBitmapBits */
        int stride = bpp == 8 ? ((w + 1) & ~1) : ((w + 15) / 16) * 2;
        const uint8_t *src = (const uint8_t *)farp(bits);
        int x, y;
        for (y = 0; y < h; ++y) {
            for (x = 0; x < w; ++x) {
                b->px[y * w + x] = bpp == 8 ? src[y * stride + x] : (uint8_t)((src[y * stride + (x >> 3)] >> (7 - (x & 7))) & 1);
            }
        }
    }
}

static int32_t bitmap_bits(uint16_t hbm, uint32_t cb, uint32_t buf, int set)
{
    Bitmap *b = bmp_of(hbm);
    int stride, x, y, rows;
    uint8_t *p;
    if (!b || !buf) {
        return 0;
    }
    stride = b->bpp == 8 ? ((b->w + 1) & ~1) : ((b->w + 15) / 16) * 2;
    /* cb is unsigned; callers pass oversized counts (16-bit size overflow) and
     * GDI simply copies at most the whole bitmap. */
    rows = cb / (uint32_t)stride > (uint32_t)b->h ? b->h : (int)(cb / (uint32_t)stride);
    for (y = 0; y < rows; ++y) {
        uint32_t lin = SEGBASE[(uint16_t)((buf >> 16) + 8 * (((uint32_t)(uint16_t)buf + (uint32_t)y * stride) >> 16)) >> 3];
        /* huge pointer: compute linear address of row y */
        uint32_t off = (uint32_t)(uint16_t)buf + (uint32_t)y * (uint32_t)stride;
        uint16_t sel = (uint16_t)((buf >> 16) + 8u * (off >> 16));
        (void)lin;
        p = MEM + SEGBASE[sel >> 3] + (off & 0xFFFF);
        if (b->bpp == 8) {
            if (set) {
                memcpy(b->px + y * b->w, p, (size_t)b->w);
            } else {
                memcpy(p, b->px + y * b->w, (size_t)b->w);
                if (stride > b->w) p[b->w] = 0;
            }
        } else {
            for (x = 0; x < stride * 8; ++x) {
                if (set) {
                    if (x < b->w) {
                        b->px[y * b->w + x] = (uint8_t)((p[x >> 3] >> (7 - (x & 7))) & 1);
                    }
                } else {
                    uint8_t bit = (uint8_t)(x < b->w ? b->px[y * b->w + x] : 0);
                    if ((x & 7) == 0) p[x >> 3] = 0;
                    p[x >> 3] |= (uint8_t)(bit << (7 - (x & 7)));
                }
            }
        }
    }
    return rows * stride;
}

void api_GDI_SETBITMAPBITS(void)
{
    ArgReader a = args_pascal(10);
    uint16_t h = arg_w(&a);
    uint32_t cb = arg_d(&a);
    uint32_t buf = arg_d(&a);
    ret_d((uint32_t)bitmap_bits(h, cb, buf, 1));
    api_log("SetBitmapBits(%04x, %u, %08x) = %u [%dx%d]", h, cb, buf, ((uint32_t)DX << 16) | AX,
            bmp_of(h) ? bmp_of(h)->w : -1, bmp_of(h) ? bmp_of(h)->h : -1);
    g_screen_dirty = 1;
    api_ret(10);
}

void api_GDI_GETBITMAPBITS(void)
{
    ArgReader a = args_pascal(10);
    uint16_t h = arg_w(&a);
    uint32_t cb = arg_d(&a);
    uint32_t buf = arg_d(&a);
    ret_d((uint32_t)bitmap_bits(h, cb, buf, 0));
    api_log("GetBitmapBits(%04x, %u, %08x) = %u [%dx%d]", h, cb, buf, ((uint32_t)DX << 16) | AX,
            bmp_of(h) ? bmp_of(h)->w : -1, bmp_of(h) ? bmp_of(h)->h : -1);
    api_ret(10);
}

void api_GDI_GETOBJECT(void)
{
    ArgReader a = args_pascal(8);
    uint16_t h = arg_w(&a);
    int16_t cb = arg_i(&a);
    uint32_t buf = arg_d(&a);
    Obj *o = obj_get(h, 0);
    uint8_t tmp[64];
    int n = 0;
    memset(tmp, 0, sizeof(tmp));
    if (o && o->type == OBJ_BITMAP) {
        Bitmap *b = o->u.bmp;
        int16_t v[4] = {0, (int16_t)b->w, (int16_t)b->h,
                        (int16_t)(b->bpp == 8 ? ((b->w + 1) & ~1) : ((b->w + 15) / 16) * 2)};
        memcpy(tmp, v, 8);
        tmp[8] = 1;
        tmp[9] = (uint8_t)b->bpp;
        n = 14;
    } else if (o && o->type == OBJ_PAL) {
        uint16_t c = (uint16_t)o->u.pal->n;
        memcpy(tmp, &c, 2);
        n = 2;
    } else if (o && o->type == OBJ_FONT) {
        Font *f = o->u.font;
        int16_t ht = (int16_t)f->height, wt = (int16_t)f->weight;
        memcpy(tmp, &ht, 2);
        memcpy(tmp + 8, &wt, 2);
        tmp[10] = (uint8_t)f->italic;
        tmp[11] = (uint8_t)f->underline;
        snprintf((char *)tmp + 18, 32, "%s", f->face);
        n = 50;
    } else if (o && o->type == OBJ_BRUSH) {
        uint32_t c = o->u.color;
        memcpy(tmp + 2, &c, 4);
        n = 8;
    }
    if (n > cb) {
        n = cb;
    }
    if (buf && n > 0) {
        memcpy(farp(buf), tmp, (size_t)n);
    }
    AX = (uint16_t)n;
    api_ret(8);
}

void api_GDI_SELECTOBJECT(void)
{
    ArgReader a = args_pascal(4);
    uint16_t hdc = arg_w(&a), h = arg_w(&a);
    DC *dc = dc_get(hdc);
    Obj *o = obj_get(h, 0);
    AX = 0;
    if (dc && o) {
        switch (o->type) {
            case OBJ_BITMAP: AX = dc->bitmap; dc->bitmap = h; break;
            case OBJ_FONT: AX = dc->font; dc->font = h; break;
            case OBJ_BRUSH: AX = dc->brush; dc->brush = h; break;
            case OBJ_PEN: AX = dc->pen; dc->pen = h; break;
            case OBJ_RGN:
                rgn_free(dc->clip);
                dc->clip = rgn_copy(o->u.rgn);
                AX = (uint16_t)rgn_normalize(dc->clip);
                break;
            default: break;
        }
    }
    api_ret(4);
}

void api_GDI_DELETEOBJECT(void)
{
    ArgReader a = args_pascal(2);
    uint16_t h = arg_w(&a);
    Obj *o = obj_get(h, 0);
    AX = 0;
    if (o && !o->stock) {
        switch (o->type) {
            case OBJ_BITMAP: free(o->u.bmp->px); free(o->u.bmp); break;
            case OBJ_FONT: free(o->u.font); break;
            case OBJ_PAL: free(o->u.pal); break;
            case OBJ_RGN: rgn_free(o->u.rgn); break;
            case OBJ_DC: dc_free(h); AX = 1; api_ret(2); return;
            default: break;
        }
        o->type = OBJ_FREE;
        AX = 1;
    }
    api_ret(2);
}

void api_GDI_SETBKCOLOR(void)
{
    ArgReader a = args_pascal(6);
    DC *dc = dc_get(arg_w(&a));
    uint32_t c = arg_d(&a);
    ret_d(dc ? dc->bk_color : 0x80000000u);
    if (dc) dc->bk_color = c;
    api_ret(6);
}

void api_GDI_SETTEXTCOLOR(void)
{
    ArgReader a = args_pascal(6);
    DC *dc = dc_get(arg_w(&a));
    uint32_t c = arg_d(&a);
    ret_d(dc ? dc->text_color : 0x80000000u);
    if (dc) dc->text_color = c;
    api_ret(6);
}

void api_GDI_SETTEXTALIGN(void)
{
    ArgReader a = args_pascal(4);
    DC *dc = dc_get(arg_w(&a));
    uint16_t v = arg_w(&a);
    AX = dc ? dc->text_align : 0;
    if (dc) dc->text_align = v;
    api_ret(4);
}

void api_GDI_SETMAPMODE(void)
{
    ArgReader a = args_pascal(4);
    DC *dc = dc_get(arg_w(&a));
    uint16_t v = arg_w(&a);
    AX = dc ? (uint16_t)dc->map_mode : 0;
    if (dc) dc->map_mode = v;
    api_ret(4);
}

void api_GDI_SETSTRETCHBLTMODE(void)
{
    ArgReader a = args_pascal(4);
    DC *dc = dc_get(arg_w(&a));
    uint16_t v = arg_w(&a);
    AX = dc ? (uint16_t)dc->stretch_mode : 0;
    if (dc) dc->stretch_mode = v;
    api_ret(4);
}

void api_GDI_SETWINDOWORG(void)
{
    ArgReader a = args_pascal(6);
    DC *dc = dc_get(arg_w(&a));
    int16_t x = arg_i(&a), y = arg_i(&a);
    ret_d(dc ? ((uint32_t)(uint16_t)dc->org_y << 16) | (uint16_t)dc->org_x : 0);
    if (dc) { dc->org_x = x; dc->org_y = y; }
    api_ret(6);
}

void api_GDI_GETWINDOWORG(void)
{
    ArgReader a = args_pascal(2);
    DC *dc = dc_get(arg_w(&a));
    ret_d(dc ? ((uint32_t)(uint16_t)dc->org_y << 16) | (uint16_t)dc->org_x : 0);
    api_ret(2);
}

void api_GDI_GETWINDOWORGEX(void)
{
    ArgReader a = args_pascal(6);
    DC *dc = dc_get(arg_w(&a));
    uint32_t pt = arg_d(&a);
    if (dc && pt) {
        int16_t *p = (int16_t *)farp(pt);
        p[0] = (int16_t)dc->org_x;
        p[1] = (int16_t)dc->org_y;
    }
    AX = dc ? 1 : 0;
    api_ret(6);
}

void api_GDI_GETDEVICECAPS(void)
{
    ArgReader a = args_pascal(4);
    (void)arg_w(&a);
    int16_t i = arg_i(&a);
    int v = 0;
    switch (i) {
        case 0: v = 0x300; break;     /* DRIVERVERSION */
        case 2: v = 1; break;         /* TECHNOLOGY: DT_RASDISPLAY */
        case 4: v = 208; break;       /* HORZSIZE mm */
        case 6: v = 156; break;       /* VERTSIZE */
        case 8: v = SCREEN_W; break;  /* HORZRES */
        case 10: v = SCREEN_H; break; /* VERTRES */
        case 12: v = 8; break;        /* BITSPIXEL */
        case 14: v = 1; break;        /* PLANES */
        case 16: v = -1; break;       /* NUMBRUSHES */
        case 18: v = 100; break;      /* NUMPENS */
        case 22: v = 0; break;        /* NUMFONTS */
        case 24: v = 20; break;       /* NUMCOLORS */
        case 38: v = 0x0001 | 0x0008 | 0x0080 | 0x0100 | 0x0200 | 0x0800; break; /* RASTERCAPS */
        case 40: v = 36; break;       /* ASPECTX */
        case 42: v = 36; break;
        case 44: v = 51; break;
        case 88: v = 96; break;       /* LOGPIXELSX */
        case 90: v = 96; break;
        case 104: v = 256; break;     /* SIZEPALETTE */
        case 106: v = 20; break;      /* NUMRESERVED */
        case 108: v = 18; break;      /* COLORRES */
        default: v = 0; break;
    }
    AX = (uint16_t)v;
    api_ret(4);
}

void api_GDI_BITBLT(void)
{
    ArgReader a = args_pascal(20);
    uint16_t hd = arg_w(&a);
    int16_t x = arg_i(&a), y = arg_i(&a), w = arg_i(&a), h = arg_i(&a);
    uint16_t hs = arg_w(&a);
    int16_t sx = arg_i(&a), sy = arg_i(&a);
    uint32_t rop = arg_d(&a);
    do_blit(hd, x, y, w, h, hs, sx, sy, w, h, rop);
    AX = 1;
    api_ret(20);
}

void api_GDI_STRETCHBLT(void)
{
    ArgReader a = args_pascal(24);
    uint16_t hd = arg_w(&a);
    int16_t x = arg_i(&a), y = arg_i(&a), w = arg_i(&a), h = arg_i(&a);
    uint16_t hs = arg_w(&a);
    int16_t sx = arg_i(&a), sy = arg_i(&a), sw = arg_i(&a), sh = arg_i(&a);
    uint32_t rop = arg_d(&a);
    do_blit(hd, x, y, w, h, hs, sx, sy, sw, sh, rop);
    AX = 1;
    api_ret(24);
}

static void fill_rect_brush(uint16_t hdc, const int16_t *rc, uint16_t hbr)
{
    DC *dc = dc_get(hdc);
    uint16_t old;
    if (!dc) {
        return;
    }
    old = dc->brush;
    if (obj_get(hbr, OBJ_BRUSH) || (hbr >= 1 && hbr <= 21)) {
        dc->brush = hbr;
    }
    if (hbr >= 1 && hbr <= 21 && !obj_get(hbr, OBJ_BRUSH)) {
        /* system color brush: make a temporary */
        int ok;
        uint16_t t = make_brush(gdi_brush_color(hbr, &ok), 0);
        dc->brush = t;
        do_blit(hdc, rc[0], rc[1], rc[2] - rc[0], rc[3] - rc[1], 0, 0, 0, 0, 0, 0xF00021u);
        obj_get(t, 0)->type = OBJ_FREE;
    } else {
        do_blit(hdc, rc[0], rc[1], rc[2] - rc[0], rc[3] - rc[1], 0, 0, 0, 0, 0, 0xF00021u);
    }
    dc->brush = old;
}

void api_USER_FILLRECT(void)
{
    ArgReader a = args_pascal(8);
    uint16_t hdc = arg_w(&a);
    uint32_t rc = arg_d(&a);
    uint16_t hbr = arg_w(&a);
    int16_t r[4];
    memcpy(r, farp(rc), 8);
    fill_rect_brush(hdc, r, hbr);
    AX = 1;
    api_ret(8);
}

void api_GDI_FILLRGN(void)
{
    ArgReader a = args_pascal(6);
    uint16_t hdc = arg_w(&a), hr = arg_w(&a), hbr = arg_w(&a);
    DC *dc = dc_get(hdc);
    Rgn *r = rgn_of(hr);
    AX = 0;
    if (dc && r && r->x1 > r->x0) {
        Rgn *saved = dc->clip;
        int16_t rc[4];
        Rgn *tmp = rgn_copy(r);
        if (saved) {
            rgn_combine(tmp, r, saved, 1);
        }
        /* region coordinates are logical: convert to device for the clip */
        {
            Rgn *dev = rgn_copy(tmp);
            dev->x0 -= dc->org_x; dev->x1 -= dc->org_x;
            dev->y0 -= dc->org_y; dev->y1 -= dc->org_y;
            dc->clip = dev;
            rc[0] = (int16_t)tmp->x0; rc[1] = (int16_t)tmp->y0; rc[2] = (int16_t)tmp->x1; rc[3] = (int16_t)tmp->y1;
            fill_rect_brush(hdc, rc, hbr);
            rgn_free(dev);
        }
        rgn_free(tmp);
        dc->clip = saved;
        AX = 1;
    }
    api_ret(6);
}

void api_GDI_CREATERECTRGN(void)
{
    ArgReader a = args_pascal(8);
    int16_t x0 = arg_i(&a), y0 = arg_i(&a), x1 = arg_i(&a), y1 = arg_i(&a);
    uint16_t h = obj_new(OBJ_RGN);
    obj_get(h, 0)->u.rgn = rgn_new_rect(x0, y0, x1, y1);
    AX = h;
    api_ret(8);
}

void api_GDI_CREATERECTRGNINDIRECT(void)
{
    ArgReader a = args_pascal(4);
    int16_t r[4];
    uint16_t h;
    memcpy(r, farp(arg_d(&a)), 8);
    h = obj_new(OBJ_RGN);
    obj_get(h, 0)->u.rgn = rgn_new_rect(r[0], r[1], r[2], r[3]);
    AX = h;
    api_ret(4);
}

void api_GDI_COMBINERGN(void)
{
    ArgReader a = args_pascal(8);
    uint16_t hd = arg_w(&a), h1 = arg_w(&a), h2 = arg_w(&a);
    int16_t mode = arg_i(&a);
    Rgn *d = rgn_of(hd), *r1 = rgn_of(h1), *r2 = rgn_of(h2);
    AX = 0;
    if (d && r1 && (r2 || mode == 5)) {
        Rgn *c1 = rgn_copy(r1);
        Rgn *c2 = r2 ? rgn_copy(r2) : NULL;
        AX = (uint16_t)rgn_combine(d, c1, c2 ? c2 : c1, mode);
        rgn_free(c1);
        rgn_free(c2);
    }
    api_ret(8);
}

void api_GDI_OFFSETRGN(void)
{
    ArgReader a = args_pascal(6);
    Rgn *r = rgn_of(arg_w(&a));
    int16_t dx = arg_i(&a), dy = arg_i(&a);
    AX = 0;
    if (r) {
        if (r->x1 > r->x0) {
            r->x0 += dx; r->x1 += dx; r->y0 += dy; r->y1 += dy;
            AX = 2;
        } else {
            AX = 1;
        }
    }
    api_ret(6);
}

void api_GDI_SELECTCLIPRGN(void)
{
    ArgReader a = args_pascal(4);
    DC *dc = dc_get(arg_w(&a));
    uint16_t hr = arg_w(&a);
    Rgn *r = rgn_of(hr);
    AX = 0;
    if (dc) {
        rgn_free(dc->clip);
        dc->clip = NULL;
        if (r) {
            dc->clip = rgn_copy(r);
            AX = (uint16_t)rgn_normalize(dc->clip);
        } else {
            AX = 2;
        }
    }
    api_ret(4);
}

/* ------------------------------------------------------------ palettes */

void api_GDI_CREATEPALETTE(void)
{
    ArgReader a = args_pascal(4);
    uint32_t lp = arg_d(&a);
    uint8_t *p = (uint8_t *)farp(lp);
    uint16_t n;
    uint16_t h;
    Palette *pal;
    int i;
    memcpy(&n, p + 2, 2);
    if (n > 256) {
        n = 256;
    }
    h = obj_new(OBJ_PAL);
    pal = (Palette *)calloc(1, sizeof(Palette));
    pal->n = n;
    for (i = 0; i < n; ++i) {
        pal->e[i].r = p[4 + 4 * i];
        pal->e[i].g = p[5 + 4 * i];
        pal->e[i].b = p[6 + 4 * i];
        pal->e[i].flags = p[7 + 4 * i];
        pal->map[i] = (uint8_t)nearest_static(pal->e[i].r, pal->e[i].g, pal->e[i].b);
    }
    obj_get(h, 0)->u.pal = pal;
    AX = h;
    api_ret(4);
}

void api_USER_SELECTPALETTE(void)
{
    ArgReader a = args_pascal(6);
    DC *dc = dc_get(arg_w(&a));
    uint16_t hp = arg_w(&a);
    (void)arg_w(&a);
    AX = 0;
    if (dc && pal_of(hp)) {
        AX = dc->pal;
        dc->pal = hp;
    }
    api_ret(6);
}

void api_USER_REALIZEPALETTE(void)
{
    ArgReader a = args_pascal(2);
    DC *dc = dc_get(arg_w(&a));
    AX = dc ? (uint16_t)realize(dc->pal) : 0;
    api_ret(2);
}

void api_GDI_ANIMATEPALETTE(void)
{
    ArgReader a = args_pascal(10);
    uint16_t hp = arg_w(&a);
    uint16_t start = arg_w(&a), n = arg_w(&a);
    uint32_t lp = arg_d(&a);
    Palette *p = pal_of(hp);
    const uint8_t *e = (const uint8_t *)farp(lp);
    int i;
    if (p) {
        for (i = 0; i < n && start + i < p->n; ++i) {
            RGBQ *d = &p->e[start + i];
            if (d->flags & 0x01) { /* PC_RESERVED: animate in place */
                d->r = e[4 * i];
                d->g = e[4 * i + 1];
                d->b = e[4 * i + 2];
                if (g_realized_pal == hp) {
                    g_syspal[p->map[start + i]] = *d;
                    g_screen_dirty = 1;
                }
            }
        }
    }
    api_ret(10);
}

void api_GDI_GETSYSTEMPALETTEENTRIES(void)
{
    ArgReader a = args_pascal(10);
    (void)arg_w(&a);
    uint16_t start = arg_w(&a), n = arg_w(&a);
    uint32_t lp = arg_d(&a);
    int i;
    if (lp) {
        uint8_t *e = (uint8_t *)farp(lp);
        for (i = 0; i < n && start + i < 256; ++i) {
            e[4 * i] = g_syspal[start + i].r;
            e[4 * i + 1] = g_syspal[start + i].g;
            e[4 * i + 2] = g_syspal[start + i].b;
            e[4 * i + 3] = 0;
        }
    }
    AX = n;
    api_ret(10);
}

/* ------------------------------------------------------------ text */

void api_GDI_CREATEFONT(void)
{
    ArgReader a = args_pascal(30);
    int16_t height = arg_i(&a), width = arg_i(&a), esc = arg_i(&a), orient = arg_i(&a), weight = arg_i(&a);
    uint8_t italic = (uint8_t)arg_w(&a), underline = (uint8_t)arg_w(&a), strike = (uint8_t)arg_w(&a);
    uint8_t charset = (uint8_t)arg_w(&a), outp = (uint8_t)arg_w(&a), clipp = (uint8_t)arg_w(&a);
    uint8_t quality = (uint8_t)arg_w(&a), pitch = (uint8_t)arg_w(&a);
    char face[40];
    uint16_t h;
    Font *f;
    (void)width; (void)esc; (void)orient; (void)strike; (void)charset; (void)outp; (void)clipp; (void)quality; (void)pitch;
    far_to_c(arg_d(&a), face, sizeof(face));
    h = obj_new(OBJ_FONT);
    f = (Font *)calloc(1, sizeof(Font));
    snprintf(f->face, sizeof(f->face), "%s", face);
    f->height = height;
    f->weight = weight ? weight : 400;
    f->italic = italic;
    f->underline = underline;
    font_setup(f);
    obj_get(h, 0)->u.font = f;
    api_log("CreateFont(%d, w%d, '%s') -> em %d", height, weight, face, f->em);
    AX = h;
    api_ret(30);
}

void api_GDI_TEXTOUT(void)
{
    ArgReader a = args_pascal(12);
    uint16_t hdc = arg_w(&a);
    int16_t x = arg_i(&a), y = arg_i(&a);
    uint32_t s = arg_d(&a);
    int16_t n = arg_i(&a);
    DC *dc = dc_get(hdc);
    if (dc && s && n > 0) {
        draw_text(dc, x, y, (const unsigned char *)farp(s), n);
    }
    AX = 1;
    api_ret(12);
}

/* Logical units per device pixel (96 dpi) for the fixed map modes. */
static double map_scale(DC *dc)
{
    switch (dc->map_mode) {
        case 2: return 254.0 / 96.0;   /* MM_LOMETRIC */
        case 3: return 2540.0 / 96.0;  /* MM_HIMETRIC */
        case 4: return 100.0 / 96.0;   /* MM_LOENGLISH */
        case 5: return 1000.0 / 96.0;  /* MM_HIENGLISH */
        case 6: return 1440.0 / 96.0;  /* MM_TWIPS */
        default: return 1.0;
    }
}

void api_GDI_GETTEXTEXTENT(void)
{
    ArgReader a = args_pascal(8);
    DC *dc = dc_get(arg_w(&a));
    uint32_t s = arg_d(&a);
    int16_t n = arg_i(&a);
    if (dc) {
        Font *f = font_of(dc);
        int w = (s && n > 0) ? text_width(f, (const unsigned char *)farp(s), n) : 0;
        double k = map_scale(dc);
        ret_d(((uint32_t)(uint16_t)lround(f->cell * k) << 16) | (uint16_t)lround(w * k));
    } else {
        ret_d(0);
    }
    api_ret(8);
}

void api_GDI_GETTEXTMETRICS(void)
{
    ArgReader a = args_pascal(6);
    DC *dc = dc_get(arg_w(&a));
    uint32_t lp = arg_d(&a);
    if (dc && lp) {
        Font *f = font_of(dc);
        uint8_t *t = (uint8_t *)farp(lp);
        int16_t v[8];
        int avg = glyph_advance(f, 'x'), mx = glyph_advance(f, 'W');
        int16_t tail[3] = {0, 96, 96};
        v[0] = (int16_t)f->cell;
        v[1] = (int16_t)f->ascent;
        v[2] = (int16_t)f->descent;
        v[3] = (int16_t)(f->cell - f->em);
        v[4] = 0;
        v[5] = (int16_t)avg;
        v[6] = (int16_t)mx;
        v[7] = (int16_t)f->weight;
        if (map_scale(dc) != 1.0) {
            int k;
            for (k = 0; k < 7; ++k) v[k] = (int16_t)lround(v[k] * map_scale(dc));
        }
        memcpy(t, v, 16);
        t[16] = (uint8_t)f->italic;
        t[17] = (uint8_t)f->underline;
        t[18] = 0;
        t[19] = 0x20;
        t[20] = 0xFF;
        t[21] = 0x1F;
        t[22] = 0x20;
        t[23] = 0x26; /* TMPF_TRUETYPE|VARIABLE_PITCH, FF_SWISS */
        t[24] = 0;
        memcpy(t + 25, tail, 6);
        AX = 1;
    } else {
        AX = 0;
    }
    api_ret(6);
}
