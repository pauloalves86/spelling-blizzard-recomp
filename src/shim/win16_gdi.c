#include "win16_gdi.h"

#include <stdlib.h>
#include <string.h>

#define SB_MAX_GDI_BITMAPS 512
#define SB_MAX_GDI_DCS     256
#define SB_MAX_GDI_PALS    64

/* Handle tags so SelectObject/DeleteObject can distinguish bitmaps and palettes */
#define SB_HANDLE_TAG_BITMAP  0x1000u
#define SB_HANDLE_TAG_DC      0x2000u
#define SB_HANDLE_TAG_PALETTE 0x3000u

typedef struct SBGdiBitmapSlot {
    uint8_t in_use;
    int16_t width;
    int16_t height;
    int16_t stride;
    uint8_t bpp;
    uint8_t *pixels; /* Stored as 1 byte per pixel (8-bit index, or 0x00/0xFF for 1-bit masks) */
    uint32_t raw_byte_size;
} SBGdiBitmapSlot;

typedef struct SBGdiDcSlot {
    uint8_t in_use;
    SB_HBITMAP selected_bmp;
    SB_HPALETTE selected_pal;
    uint32_t bk_color;
} SBGdiDcSlot;

typedef struct SBGdiPaletteSlot {
    uint8_t in_use;
    uint16_t num_entries;
    SB_PALETTEENTRY entries[256];
} SBGdiPaletteSlot;

static SBGdiBitmapSlot g_bitmaps[SB_MAX_GDI_BITMAPS];
static SBGdiDcSlot g_dcs[SB_MAX_GDI_DCS];
static SBGdiPaletteSlot g_palettes[SB_MAX_GDI_PALS];
static SB_PALETTEENTRY g_active_palette[256];

void sb_win16_gdi_shim_reset(void)
{
    size_t i;
    for (i = 0; i < SB_MAX_GDI_BITMAPS; ++i) {
        if (g_bitmaps[i].pixels != NULL) {
            free(g_bitmaps[i].pixels);
        }
        memset(&g_bitmaps[i], 0, sizeof(g_bitmaps[i]));
    }
    memset(g_dcs, 0, sizeof(g_dcs));
    memset(g_palettes, 0, sizeof(g_palettes));
    memset(g_active_palette, 0, sizeof(g_active_palette));
}

/* ==========================================================================
 * USER Rectangle APIs
 * ========================================================================== */

void sb_win16_SetRect(SB_RECT16 *rc, int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    if (!rc) {
        return;
    }
    rc->left = left;
    rc->top = top;
    rc->right = right;
    rc->bottom = bottom;
}

int sb_win16_IsRectEmpty(const SB_RECT16 *rc)
{
    if (!rc) {
        return 1;
    }
    return (rc->left >= rc->right || rc->top >= rc->bottom) ? 1 : 0;
}

int sb_win16_EqualRect(const SB_RECT16 *a, const SB_RECT16 *b)
{
    if (!a || !b) {
        return 0;
    }
    return (a->left == b->left && a->top == b->top &&
            a->right == b->right && a->bottom == b->bottom)
               ? 1
               : 0;
}

void sb_win16_OffsetRect(SB_RECT16 *rc, int16_t dx, int16_t dy)
{
    if (!rc) {
        return;
    }
    rc->left = (int16_t)(rc->left + dx);
    rc->right = (int16_t)(rc->right + dx);
    rc->top = (int16_t)(rc->top + dy);
    rc->bottom = (int16_t)(rc->bottom + dy);
}

int sb_win16_IntersectRect(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b)
{
    int16_t l, t, r, bot;
    if (!dst || !a || !b) {
        return 0;
    }
    l = (a->left > b->left) ? a->left : b->left;
    t = (a->top > b->top) ? a->top : b->top;
    r = (a->right < b->right) ? a->right : b->right;
    bot = (a->bottom < b->bottom) ? a->bottom : b->bottom;
    if (l >= r || t >= bot) {
        sb_win16_SetRect(dst, 0, 0, 0, 0);
        return 0;
    }
    sb_win16_SetRect(dst, l, t, r, bot);
    return 1;
}

int sb_win16_UnionRect(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b)
{
    int a_empty = sb_win16_IsRectEmpty(a);
    int b_empty = sb_win16_IsRectEmpty(b);
    if (!dst) {
        return 0;
    }
    if (a_empty && b_empty) {
        sb_win16_SetRect(dst, 0, 0, 0, 0);
        return 0;
    }
    if (a_empty) {
        *dst = *b;
        return 1;
    }
    if (b_empty) {
        *dst = *a;
        return 1;
    }
    dst->left = (a->left < b->left) ? a->left : b->left;
    dst->top = (a->top < b->top) ? a->top : b->top;
    dst->right = (a->right > b->right) ? a->right : b->right;
    dst->bottom = (a->bottom > b->bottom) ? a->bottom : b->bottom;
    return 1;
}

/* ==========================================================================
 * GDI Palette APIs
 * ========================================================================== */

uint16_t sb_win16_GetSystemPaletteEntries(SB_HDC hdc,
                                          uint16_t iStartIndex,
                                          uint16_t nEntries,
                                          SB_PALETTEENTRY *lppe)
{
    uint16_t i;
    (void)hdc;
    if (!lppe) {
        return 0;
    }
    for (i = 0; i < nEntries; ++i) {
        uint16_t idx = (uint16_t)((iStartIndex + i) & 0xFFu);
        lppe[i] = g_active_palette[idx];
    }
    return nEntries;
}

SB_HPALETTE sb_win16_CreatePalette(const SB_LOGPALETTE *lplgpl)
{
    uint16_t i;
    if (!lplgpl) {
        return 0;
    }
    for (i = 1; i < SB_MAX_GDI_PALS; ++i) {
        if (!g_palettes[i].in_use) {
            uint16_t cnt = lplgpl->palNumEntries <= 256 ? lplgpl->palNumEntries : 256;
            g_palettes[i].in_use = 1;
            g_palettes[i].num_entries = cnt;
            memcpy(g_palettes[i].entries, lplgpl->palPalEntry, cnt * sizeof(SB_PALETTEENTRY));
            return (SB_HPALETTE)(SB_HANDLE_TAG_PALETTE | i);
        }
    }
    return 0;
}

SB_HPALETTE sb_win16_SelectPalette(SB_HDC hdc, SB_HPALETTE hpal, int bForceBackground)
{
    uint16_t pal_idx = (uint16_t)(hpal & 0x0FFFu);
    SB_HPALETTE prev = 0;
    (void)bForceBackground;
    if ((hdc & 0xF000u) == SB_HANDLE_TAG_DC) {
        uint16_t dc_idx = (uint16_t)(hdc & 0x0FFFu);
        if (dc_idx < SB_MAX_GDI_DCS && g_dcs[dc_idx].in_use) {
            prev = g_dcs[dc_idx].selected_pal;
            g_dcs[dc_idx].selected_pal = hpal;
        }
    }
    if ((hpal & 0xF000u) == SB_HANDLE_TAG_PALETTE && pal_idx < SB_MAX_GDI_PALS && g_palettes[pal_idx].in_use) {
        uint16_t cnt = g_palettes[pal_idx].num_entries;
        if (cnt == 0xECu) {
            memcpy(&g_active_palette[10], g_palettes[pal_idx].entries, cnt * sizeof(SB_PALETTEENTRY));
        } else {
            memcpy(g_active_palette, g_palettes[pal_idx].entries, cnt * sizeof(SB_PALETTEENTRY));
        }
    }
    return prev ? prev : hpal;
}

uint16_t sb_win16_RealizePalette(SB_HDC hdc)
{
    (void)hdc;
    return 236;
}

void sb_win16_AnimatePalette(SB_HPALETTE hpal,
                             uint16_t iStartIndex,
                             uint16_t nEntries,
                             const SB_PALETTEENTRY *lppe)
{
    uint16_t pal_idx = (uint16_t)(hpal & 0x0FFFu);
    uint16_t i;
    if (!lppe) {
        return;
    }
    if ((hpal & 0xF000u) == SB_HANDLE_TAG_PALETTE && pal_idx < SB_MAX_GDI_PALS && g_palettes[pal_idx].in_use) {
        for (i = 0; i < nEntries && (iStartIndex + i) < 256; ++i) {
            g_palettes[pal_idx].entries[iStartIndex + i] = lppe[i];
            g_active_palette[(10 + iStartIndex + i) & 0xFFu] = lppe[i];
        }
    }
}

const SB_PALETTEENTRY *sb_win16_gdi_getActivePaletteEntries(void)
{
    return g_active_palette;
}

/* ==========================================================================
 * GDI Device Context & Bitmap APIs
 * ========================================================================== */

static SBGdiBitmapSlot *get_bmp_slot(SB_HBITMAP hbmp)
{
    uint16_t idx = (uint16_t)(hbmp & 0x0FFFu);
    if ((hbmp & 0xF000u) == SB_HANDLE_TAG_BITMAP && idx < SB_MAX_GDI_BITMAPS && g_bitmaps[idx].in_use) {
        return &g_bitmaps[idx];
    }
    return NULL;
}

static SBGdiDcSlot *get_dc_slot(SB_HDC hdc)
{
    uint16_t idx = (uint16_t)(hdc & 0x0FFFu);
    if ((hdc & 0xF000u) == SB_HANDLE_TAG_DC && idx < SB_MAX_GDI_DCS && g_dcs[idx].in_use) {
        return &g_dcs[idx];
    }
    return NULL;
}

SB_HDC sb_win16_CreateCompatibleDC(SB_HDC hdc)
{
    uint16_t i;
    (void)hdc;
    for (i = 1; i < SB_MAX_GDI_DCS; ++i) {
        if (!g_dcs[i].in_use) {
            memset(&g_dcs[i], 0, sizeof(g_dcs[i]));
            g_dcs[i].in_use = 1;
            return (SB_HDC)(SB_HANDLE_TAG_DC | i);
        }
    }
    return 0;
}

int sb_win16_DeleteDC(SB_HDC hdc)
{
    SBGdiDcSlot *dc = get_dc_slot(hdc);
    if (!dc) {
        return 0;
    }
    memset(dc, 0, sizeof(*dc));
    return 1;
}

SB_HGDIOBJ sb_win16_SelectObject(SB_HDC hdc, SB_HGDIOBJ hgdiobj)
{
    SBGdiDcSlot *dc = get_dc_slot(hdc);
    if (!dc) {
        return 0;
    }
    if ((hgdiobj & 0xF000u) == SB_HANDLE_TAG_BITMAP) {
        SB_HBITMAP prev = dc->selected_bmp;
        dc->selected_bmp = (SB_HBITMAP)hgdiobj;
        return prev ? prev : hgdiobj;
    }
    return hgdiobj;
}

int sb_win16_DeleteObject(SB_HGDIOBJ hgdiobj)
{
    uint16_t idx = (uint16_t)(hgdiobj & 0x0FFFu);
    uint16_t tag = (uint16_t)(hgdiobj & 0xF000u);
    if (tag == SB_HANDLE_TAG_BITMAP && idx < SB_MAX_GDI_BITMAPS && g_bitmaps[idx].in_use) {
        free(g_bitmaps[idx].pixels);
        memset(&g_bitmaps[idx], 0, sizeof(g_bitmaps[idx]));
        return 1;
    }
    if (tag == SB_HANDLE_TAG_PALETTE && idx < SB_MAX_GDI_PALS && g_palettes[idx].in_use) {
        memset(&g_palettes[idx], 0, sizeof(g_palettes[idx]));
        return 1;
    }
    return 0;
}

int sb_win16_GetObjectBitmap(SB_HBITMAP hbm, int cbBuffer, SB_BITMAP16 *lpvObject)
{
    SBGdiBitmapSlot *bmp = get_bmp_slot(hbm);
    (void)cbBuffer;
    if (!bmp || !lpvObject) {
        return 0;
    }
    memset(lpvObject, 0, sizeof(*lpvObject));
    lpvObject->bmWidth = bmp->width;
    lpvObject->bmHeight = bmp->height;
    lpvObject->bmWidthBytes = bmp->stride;
    lpvObject->bmPlanes = 1;
    lpvObject->bmBitsPixel = bmp->bpp;
    lpvObject->bmBits = bmp->pixels;
    return sizeof(*lpvObject);
}

uint32_t sb_win16_SetBkColor(SB_HDC hdc, uint32_t crColor)
{
    SBGdiDcSlot *dc = get_dc_slot(hdc);
    uint32_t prev = 0;
    if (dc) {
        prev = dc->bk_color;
        dc->bk_color = crColor;
    }
    return prev;
}

SB_HBITMAP sb_win16_CreateBitmap(int16_t nWidth,
                                 int16_t nHeight,
                                 uint8_t cPlanes,
                                 uint8_t cBitsPerPel,
                                 const void *lpvBits)
{
    uint16_t i;
    int16_t w = nWidth > 0 ? nWidth : 1;
    int16_t h = nHeight > 0 ? nHeight : 1;
    int16_t stride = (int16_t)(w + (w & 1)); /* Win16 16-bit word-aligned scanline stride */
    uint32_t total_pixels = (uint32_t)stride * (uint32_t)h;
    (void)cPlanes;

    for (i = 1; i < SB_MAX_GDI_BITMAPS; ++i) {
        if (!g_bitmaps[i].in_use) {
            uint8_t *buf = (uint8_t *)calloc(1, total_pixels > 0 ? total_pixels : 1u);
            if (!buf) {
                return 0;
            }
            if (lpvBits != NULL && cBitsPerPel == 8) {
                memcpy(buf, lpvBits, total_pixels);
            }
            g_bitmaps[i].in_use = 1;
            g_bitmaps[i].width = w;
            g_bitmaps[i].height = h;
            g_bitmaps[i].stride = stride;
            g_bitmaps[i].bpp = cBitsPerPel ? cBitsPerPel : 8;
            g_bitmaps[i].pixels = buf;
            g_bitmaps[i].raw_byte_size = total_pixels;
            return (SB_HBITMAP)(SB_HANDLE_TAG_BITMAP | i);
        }
    }
    return 0;
}

int32_t sb_win16_GetBitmapBits(SB_HBITMAP hbmp, int32_t cbBuffer, void *lpvBits)
{
    SBGdiBitmapSlot *bmp = get_bmp_slot(hbmp);
    int32_t to_copy;
    if (!bmp || !lpvBits || cbBuffer <= 0) {
        return 0;
    }
    to_copy = (uint32_t)cbBuffer < bmp->raw_byte_size ? cbBuffer : (int32_t)bmp->raw_byte_size;
    memcpy(lpvBits, bmp->pixels, (size_t)to_copy);
    return to_copy;
}

int32_t sb_win16_SetBitmapBits(SB_HBITMAP hbmp, int32_t cBytes, const void *lpvBits)
{
    SBGdiBitmapSlot *bmp = get_bmp_slot(hbmp);
    int32_t to_copy;
    if (!bmp || !lpvBits || cBytes <= 0) {
        return 0;
    }
    to_copy = (uint32_t)cBytes < bmp->raw_byte_size ? cBytes : (int32_t)bmp->raw_byte_size;
    memcpy(bmp->pixels, lpvBits, (size_t)to_copy);
    return to_copy;
}

const uint8_t *sb_win16_gdi_getBitmapPixels(SB_HBITMAP hbmp, int16_t *out_w, int16_t *out_h)
{
    SBGdiBitmapSlot *bmp = get_bmp_slot(hbmp);
    if (!bmp) {
        return NULL;
    }
    if (out_w) *out_w = bmp->width;
    if (out_h) *out_h = bmp->height;
    return bmp->pixels;
}

int sb_win16_FillRectColor(SB_HDC hdc, const SB_RECT16 *lprc, uint8_t color_index)
{
    SBGdiDcSlot *dc = get_dc_slot(hdc);
    SBGdiBitmapSlot *bmp;
    int16_t x0, y0, x1, y1, y;
    if (!dc || !lprc) {
        return 0;
    }
    bmp = get_bmp_slot(dc->selected_bmp);
    if (!bmp) {
        return 0;
    }
    x0 = lprc->left < 0 ? 0 : lprc->left;
    y0 = lprc->top < 0 ? 0 : lprc->top;
    x1 = lprc->right > bmp->width ? bmp->width : lprc->right;
    y1 = lprc->bottom > bmp->height ? bmp->height : lprc->bottom;
    for (y = y0; y < y1; ++y) {
        memset(bmp->pixels + (size_t)y * bmp->stride + x0, color_index, (size_t)(x1 - x0));
    }
    return 1;
}

int sb_win16_BitBlt(SB_HDC hdcDest,
                    int16_t nXDest,
                    int16_t nYDest,
                    int16_t nWidth,
                    int16_t nHeight,
                    SB_HDC hdcSrc,
                    int16_t nXSrc,
                    int16_t nYSrc,
                    uint32_t dwRop)
{
    return sb_win16_StretchBlt(hdcDest, nXDest, nYDest, nWidth, nHeight,
                               hdcSrc, nXSrc, nYSrc, nWidth, nHeight, dwRop);
}

int sb_win16_StretchBlt(SB_HDC hdcDest,
                        int16_t nXOriginDest,
                        int16_t nYOriginDest,
                        int16_t nWidthDest,
                        int16_t nHeightDest,
                        SB_HDC hdcSrc,
                        int16_t nXOriginSrc,
                        int16_t nYOriginSrc,
                        int16_t nWidthSrc,
                        int16_t nHeightSrc,
                        uint32_t dwRop)
{
    SBGdiDcSlot *dst_dc = get_dc_slot(hdcDest);
    SBGdiDcSlot *src_dc = get_dc_slot(hdcSrc);
    SBGdiBitmapSlot *dst_bmp = dst_dc ? get_bmp_slot(dst_dc->selected_bmp) : NULL;
    SBGdiBitmapSlot *src_bmp = src_dc ? get_bmp_slot(src_dc->selected_bmp) : NULL;
    int16_t dy, dx;

    if (!dst_bmp || nWidthDest <= 0 || nHeightDest <= 0) {
        return 0;
    }
    if (dwRop == SB_BLACKNESS) {
        SB_RECT16 rc = { nXOriginDest, nYOriginDest,
                         (int16_t)(nXOriginDest + nWidthDest),
                         (int16_t)(nYOriginDest + nHeightDest) };
        return sb_win16_FillRectColor(hdcDest, &rc, 0);
    }
    if (!src_bmp || nWidthSrc <= 0 || nHeightSrc <= 0) {
        return 0;
    }

    /* Check if this is 1010:2d58's color-to-monochrome mask generation
     * (8-bit color source -> 1-bit monochrome destination where pixel == bk_color -> 0xFF, else 0x00)
     */
    for (dy = 0; dy < nHeightDest; ++dy) {
        int16_t dst_y = (int16_t)(nYOriginDest + dy);
        int16_t src_y = (int16_t)(nYOriginSrc + ((int32_t)dy * nHeightSrc) / nHeightDest);
        if (dst_y < 0 || dst_y >= dst_bmp->height || src_y < 0 || src_y >= src_bmp->height) {
            continue;
        }
        for (dx = 0; dx < nWidthDest; ++dx) {
            int16_t dst_x = (int16_t)(nXOriginDest + dx);
            int16_t src_x = (int16_t)(nXOriginSrc + ((int32_t)dx * nWidthSrc) / nWidthDest);
            uint8_t s_val;
            uint8_t *d_ptr;

            if (dst_x < 0 || dst_x >= dst_bmp->width || src_x < 0 || src_x >= src_bmp->width) {
                continue;
            }
            s_val = src_bmp->pixels[(size_t)src_y * src_bmp->stride + src_x];
            d_ptr = &dst_bmp->pixels[(size_t)dst_y * dst_bmp->stride + dst_x];

            if (dst_bmp->bpp == 1 && src_bmp->bpp == 8) {
                /* In Win16 GDI, blitting an 8-bit color DC to a 1-bit monochrome DC sets 1 (0xFF)
                 * where pixel == bk_color (0 = transparent background) and 0 (0x00) for foreground.
                 */
                *d_ptr = (s_val == (uint8_t)src_dc->bk_color) ? 0xFFu : 0x00u;
            } else if (dwRop == SB_SRCAND) {
                *d_ptr &= s_val;
            } else if (dwRop == SB_SRCPAINT) {
                *d_ptr |= s_val;
            } else {
                /* Default SB_SRCCOPY */
                *d_ptr = s_val;
            }
        }
    }
    return 1;
}
