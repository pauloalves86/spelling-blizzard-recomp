#include "graphics_gdi.h"

#include <stdlib.h>
#include <string.h>

SBPaletteManager *g_DAT_1060_0198_paletteMgr = NULL;
SB_HDC g_DAT_1060_01d2_screenDC = 0;

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* ==========================================================================
 * 1. Geometry & Rectangle Helpers (`1010:0f1c..0fd0`)
 * ========================================================================== */

/* Original: 1010:0f1c */
int Rect_IsEmpty_1010_0f1c(const SB_RECT16 *rc)
{
    return sb_win16_IsRectEmpty(rc);
}

/* Original: 1010:0f2a */
int Rect_Equal_1010_0f2a(const SB_RECT16 *a, const SB_RECT16 *b)
{
    return sb_win16_EqualRect(a, b);
}

/* Original: 1010:0f3c */
void Rect_Inset_1010_0f3c(SB_RECT16 *rc, int16_t dx, int16_t dy)
{
    if (!rc) {
        return;
    }
    sb_win16_SetRect(rc,
                     (int16_t)(rc->left + dx),
                     (int16_t)(rc->top + dy),
                     (int16_t)(rc->right - dx),
                     (int16_t)(rc->bottom - dy));
}

/* Original: 1010:0f74 */
void Rect_Offset_1010_0f74(SB_RECT16 *rc, int16_t dx, int16_t dy)
{
    sb_win16_OffsetRect(rc, dx, dy);
}

/* Original: 1010:0f88 */
int Rect_Intersect_1010_0f88(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b)
{
    return sb_win16_IntersectRect(dst, a, b);
}

/* Original: 1010:0f9e */
void Point_Set_1010_0f9e(SB_POINT16 *pt, int16_t x, int16_t y)
{
    if (pt) {
        pt->x = x;
        pt->y = y;
    }
}

/* Original: 1010:0fb6 */
void Rect_Set_1010_0fb6(SB_RECT16 *rc, int16_t left, int16_t top, int16_t right, int16_t bottom)
{
    sb_win16_SetRect(rc, left, top, right, bottom);
}

/* Original: 1010:0fd0 */
int Rect_Union_1010_0fd0(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b)
{
    return sb_win16_UnionRect(dst, a, b);
}

/* ==========================================================================
 * 2. Palette Manager (`CPaletteManager_1008_8278` at `DAT_1060_0198`)
 * ========================================================================== */

static int16_t clamp_pal_idx(int16_t idx_1based)
{
    int16_t idx = (int16_t)(idx_1based - 1);
    if (idx < 0) {
        idx = 0;
    }
    if (idx > 0xEB) {
        idx = 0xEB;
    }
    return idx;
}

/* Original: 1008:326e */
SBPaletteManager *CPaletteManager_Init_1008_326e(SBPaletteManager *self)
{
    int i;
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x10088278u;
    sb_win16_GetSystemPaletteEntries(g_DAT_1060_01d2_screenDC, 10, 0xEC, self->current_3bc);
    memcpy(self->initial_0c, self->current_3bc, sizeof(self->initial_0c));
    memcpy(self->target_76c, self->current_3bc, sizeof(self->target_76c));
    memset(self->black_ecc, 0, sizeof(self->black_ecc));

    for (i = 0; i < 0xEC; ++i) {
        self->initial_0c[i].peFlags = 1; /* PC_RESERVED */
        self->current_3bc[i].peFlags = 1;
        self->target_76c[i].peFlags = 1;
        self->black_ecc[i].peFlags = 1;
    }

    self->logpal_04 = (SB_LOGPALETTE *)AllocZeroed_1008_eed8(sizeof(SB_LOGPALETTE));
    if (self->logpal_04 != NULL) {
        self->logpal_04->palVersion = 0x0300;
        self->logpal_04->palNumEntries = 0xEC;
        memcpy(self->logpal_04->palPalEntry, self->initial_0c, 0xEC * sizeof(SB_PALETTEENTRY));
        self->hPalette_08 = sb_win16_CreatePalette(self->logpal_04);
        self->hOldPalette_0a = sb_win16_SelectPalette(g_DAT_1060_01d2_screenDC, self->hPalette_08, 0);
        sb_win16_RealizePalette(g_DAT_1060_01d2_screenDC);
    }
    g_DAT_1060_0198_paletteMgr = self;
    return self;
}

/* Original: 1008:3464 */
void CPaletteManager_Dtor_1008_3464(SBPaletteManager *self)
{
    if (!self) {
        return;
    }
    CPaletteManager_ResetToInitial_1008_3a74(self);
    sb_win16_SelectPalette(g_DAT_1060_01d2_screenDC, self->hOldPalette_0a, 0);
    sb_win16_DeleteObject(self->hPalette_08);
    FreeMem_1008_ef1e(self->logpal_04);
    if (g_DAT_1060_0198_paletteMgr == self) {
        g_DAT_1060_0198_paletteMgr = NULL;
    }
    CObject_Dtor_1008_ef8e(self);
}

/* Original: 1008:34bc */
void CPaletteManager_FadeToCurrent_1008_34bc(SBPaletteManager *self, int16_t start_1based, int16_t end_1based)
{
    if (!self) {
        return;
    }
    CPaletteManager_CrossfadeRange_1008_3cd2(
        self, end_1based, start_1based, self->target_76c, self->current_3bc);
}

/* Original: 1008:34e6 */
void CPaletteManager_FadeOut_1008_34e6(SBPaletteManager *self, int16_t start_1based, int16_t end_1based)
{
    SB_PALETTEENTRY backup[236];
    if (!self) {
        return;
    }
    memcpy(backup, self->target_76c, sizeof(backup));
    memcpy(self->target_76c, self->current_3bc, sizeof(self->target_76c));
    CPaletteManager_CrossfadeRange_1008_3cd2(
        self, end_1based, start_1based, self->black_ecc, self->target_76c);
    memcpy(self->target_76c, backup, sizeof(self->target_76c));
}

/* Original: 1008:3596 */
void CPaletteManager_ApplyTargetRange_1008_3596(SBPaletteManager *self, int16_t end_1based, int16_t start_1based)
{
    int16_t s = clamp_pal_idx(start_1based);
    int16_t e = clamp_pal_idx(end_1based);
    int16_t i;
    if (!self) {
        return;
    }
    for (i = s; i <= e; ++i) {
        self->current_3bc[i] = self->target_76c[i];
    }
    sb_win16_AnimatePalette(self->hPalette_08, 0, 0xEC, self->current_3bc);
}

/* Original: 1008:3624 */
void CPaletteManager_ApplyBlackRange_1008_3624(SBPaletteManager *self, int16_t end_1based, int16_t start_1based)
{
    int16_t s = clamp_pal_idx(start_1based);
    int16_t e = clamp_pal_idx(end_1based);
    int16_t i;
    if (!self) {
        return;
    }
    for (i = s; i <= e; ++i) {
        self->current_3bc[i] = self->black_ecc[i];
    }
    sb_win16_AnimatePalette(self->hPalette_08, 0, 0xEC, self->current_3bc);
}

/* Original: 1008:36b2
 * Loads primary `'clut'` (`0x03ef`) resource (e.g. #1049), extracts 236 x 6-byte
 * 16-bit RGB entries into `raw_rgb16_127c` and reads companion `'clut'` ID at `+0x04`
 * (e.g. #999), extracts 236 x 8-bit R,G,B entries into `target_76c`, and animates
 * the hardware palette via slot `+0x3c` (`1008:3e34`).
 */
void CPaletteManager_LoadClutResource_1008_36b2(SBPaletteManager *self, uint16_t clut_res_id)
{
    SBResourceHandle *h1;
    SBResourceHandle *h2;
    const uint8_t *raw1;
    const uint8_t *raw2;
    uint16_t companion_id;
    int i;

    if (!self) {
        return;
    }
    h1 = ResMgr_GetResource_1010_10f2(0x03ef, "clut", clut_res_id);
    if (!h1 || !h1->data_00) {
        return;
    }
    Handle_Lock_1010_00e8(h1);
    raw1 = (const uint8_t *)h1->data_00;
    companion_id = read_u16_le(raw1 + 4);

    for (i = 0; i < 0xEC; ++i) {
        const uint8_t *entry = raw1 + 0x10 + (i * 8);
        self->raw_rgb16_127c[i][0] = read_u16_le(entry + 2);
        self->raw_rgb16_127c[i][1] = read_u16_le(entry + 4);
        self->raw_rgb16_127c[i][2] = read_u16_le(entry + 6);
    }
    Handle_Unlock_1010_0122(h1);
    Handle_AllowPurge_1010_0196(h1);

    h2 = ResMgr_GetResource_1010_10f2(0x03ef, "clut", companion_id);
    if (h2 != NULL && h2->data_00 != NULL) {
        Handle_Lock_1010_00e8(h2);
        raw2 = (const uint8_t *)h2->data_00;
        /* Companion clut entries start at +0x18 (skipping first 2 entries) */
        for (i = 0; i < 0xEC; ++i) {
            const uint8_t *entry = raw2 + 0x18 + (i * 8);
            self->target_76c[i].peRed = entry[2];
            self->target_76c[i].peGreen = entry[4];
            self->target_76c[i].peBlue = entry[6];
            self->target_76c[i].peFlags = 1;
        }
        Handle_Unlock_1010_0122(h2);
        Handle_AllowPurge_1010_0196(h2);
    }
    CPaletteManager_AnimateAll_1008_3e34(self, self->target_76c);
}

/* Original: 1008:382e */
void CPaletteManager_SaveAndRealize_1008_382e(SBPaletteManager *self)
{
    if (!self) {
        return;
    }
    if (self->is_saved_1804 == 0) {
        self->is_saved_1804 = 1;
        memcpy(self->saved_b1c, self->current_3bc, sizeof(self->saved_b1c));
        CPaletteManager_AnimateAll_1008_3e34(self, self->current_3bc);
    }
}

/* Original: 1008:3950 */
void CPaletteManager_RestoreAndRealize_1008_3950(SBPaletteManager *self)
{
    if (!self) {
        return;
    }
    if (self->is_saved_1804 != 0) {
        self->is_saved_1804 = 0;
        memcpy(self->current_3bc, self->saved_b1c, sizeof(self->current_3bc));
        CPaletteManager_AnimateAll_1008_3e34(self, self->current_3bc);
    }
}

/* Original: 1008:3a74 */
void CPaletteManager_ResetToInitial_1008_3a74(SBPaletteManager *self)
{
    if (!self) {
        return;
    }
    CPaletteManager_AnimateAll_1008_3e34(self, self->initial_0c);
}

/* Original: 1008:3b44 */
void CPaletteManager_CycleRange_1008_3b44(SBPaletteManager *self, int16_t from_1based, int16_t to_1based)
{
    int16_t from_idx = clamp_pal_idx(from_1based);
    int16_t to_idx = clamp_pal_idx(to_1based);
    int16_t min_idx;
    int16_t span;
    SB_PALETTEENTRY saved;

    if (!self || from_idx == to_idx) {
        return;
    }
    min_idx = (from_idx < to_idx) ? from_idx : to_idx;
    span = (int16_t)((from_idx < to_idx) ? (to_idx - from_idx) : (from_idx - to_idx));
    saved = self->current_3bc[from_idx];

    if (from_idx < to_idx) {
        memmove(&self->current_3bc[from_idx],
                &self->current_3bc[from_idx + 1],
                (size_t)span * sizeof(SB_PALETTEENTRY));
        self->current_3bc[to_idx] = saved;
    } else {
        memmove(&self->current_3bc[to_idx + 1],
                &self->current_3bc[to_idx],
                (size_t)span * sizeof(SB_PALETTEENTRY));
        self->current_3bc[to_idx] = saved;
    }
    sb_win16_AnimatePalette(self->hPalette_08, (uint16_t)min_idx, (uint16_t)(span + 1), &self->current_3bc[min_idx]);
}

/* Original: 1008:3c5c */
int16_t CPaletteManager_FindColorIndex_1008_3c5c(const SBPaletteManager *self,
                                                 const uint16_t rgb16[3],
                                                 uint32_t *out_rgb)
{
    int16_t i;
    if (!self || !rgb16) {
        return 0xEC;
    }
    for (i = 1; i <= 0xEB; ++i) {
        if (self->raw_rgb16_127c[i][0] == rgb16[0] &&
            self->raw_rgb16_127c[i][1] == rgb16[1] &&
            self->raw_rgb16_127c[i][2] == rgb16[2]) {
            if (out_rgb != NULL) {
                const SB_PALETTEENTRY *pe = &self->target_76c[i];
                *out_rgb = (uint32_t)pe->peRed |
                           ((uint32_t)pe->peGreen << 8) |
                           ((uint32_t)pe->peBlue << 16);
            }
            return i;
        }
    }
    return 0xEC;
}

/* Original: 1008:3cd2 */
void CPaletteManager_CrossfadeRange_1008_3cd2(SBPaletteManager *self,
                                              int16_t end_1based,
                                              int16_t start_1based,
                                              const SB_PALETTEENTRY *to_pal,
                                              const SB_PALETTEENTRY *from_pal)
{
    int16_t s = clamp_pal_idx(start_1based);
    int16_t e = clamp_pal_idx(end_1based);
    int step;

    if (!self || !to_pal || !from_pal || s > e) {
        return;
    }
    for (step = 0; step < 32; ++step) {
        int w_from = 31 - step;
        int w_to = step + 1;
        int16_t i;
        for (i = s; i <= e; ++i) {
            self->current_3bc[i].peRed = (uint8_t)(((int)from_pal[i].peRed * w_from + (int)to_pal[i].peRed * w_to) >> 5);
            self->current_3bc[i].peGreen = (uint8_t)(((int)from_pal[i].peGreen * w_from + (int)to_pal[i].peGreen * w_to) >> 5);
            self->current_3bc[i].peBlue = (uint8_t)(((int)from_pal[i].peBlue * w_from + (int)to_pal[i].peBlue * w_to) >> 5);
            self->current_3bc[i].peFlags = 1;
        }
        sb_win16_AnimatePalette(self->hPalette_08, (uint16_t)s, (uint16_t)(e - s + 1), &self->current_3bc[s]);
    }
}

/* Original: 1008:3e34 */
void CPaletteManager_AnimateAll_1008_3e34(SBPaletteManager *self, const SB_PALETTEENTRY *entries_236)
{
    if (!self || !entries_236) {
        return;
    }
    sb_win16_AnimatePalette(self->hPalette_08, 0, 0xEC, entries_236);
    memcpy(self->current_3bc, entries_236, 0xEC * sizeof(SB_PALETTEENTRY));
}

/* Original: 1008:7cb4 */
SB_HPALETTE CPaletteManager_GetHPalette_1008_7cb4(const SBPaletteManager *self)
{
    return self ? self->hPalette_08 : 0;
}

/* ==========================================================================
 * 3. PackBits RLE `PICT` Decoder & Transparent Mask Blitters
 * ========================================================================== */

/* Original: 1010:0614 */
void Global_FreePtr_1010_0614(void *ptr)
{
    free(ptr);
}

/* Original: 1010:07ba */
uint8_t Random_Byte_1010_07ba(void)
{
    return (uint8_t)(rand() & 0xFF);
}

static void decode_packbits_stream(const uint8_t *src, size_t src_max, uint8_t *dst, size_t dst_size)
{
    size_t sp = 0;
    size_t dp = 0;
    while (dp < dst_size && sp < src_max) {
        uint8_t ctrl = src[sp++];
        if ((ctrl & 0x80u) == 0) {
            size_t count = (size_t)ctrl + 1u;
            size_t i;
            for (i = 0; i < count && dp < dst_size && sp < src_max; ++i) {
                dst[dp++] = src[sp++];
            }
        } else {
            size_t count = 0x101u - (size_t)ctrl;
            uint8_t val = (sp < src_max) ? src[sp++] : 0;
            size_t i;
            for (i = 0; i < count && dp < dst_size; ++i) {
                dst[dp++] = val;
            }
        }
    }
}

/* Original: 1010:0c94 */
SB_HBITMAP Pict_DecodeRleBanded_1010_0c94(const uint8_t *rle_data, int16_t width_bytes, int16_t height)
{
    uint32_t total_size = (uint32_t)width_bytes * (uint32_t)height;
    uint8_t *pixels = (uint8_t *)calloc(1, total_size > 0 ? total_size : 1u);
    SB_HBITMAP hbmp;

    if (!pixels) {
        return 0;
    }
    decode_packbits_stream(rle_data, total_size * 2u, pixels, total_size);
    hbmp = sb_win16_CreateBitmap(width_bytes, height, 1, 8, pixels);
    free(pixels);
    return hbmp;
}

/* Original: 1010:08e0
 * Decodes a `'PICT'` (`0x03f3`) resource handle into an 8-bit `HBITMAP`.
 * Header layout:
 *   +0x00: uint16_t format_magic (0x0200 = PackBits RLE, 0x0100/0x0028 = raw uncompressed)
 *   +0x02: int16_t width
 *   +0x04: int16_t height
 *   +0x06: int16_t row_bytes (width aligned to even bytes)
 *   +0x08: pixel payload (PackBits RLE or raw 8-bit indices)
 */
SB_HBITMAP Pict_DecodeToBitmap_1010_08e0(SBResourceHandle *pict_handle, const SB_RECT16 *target_rect)
{
    const uint8_t *raw;
    uint16_t magic;
    int16_t width;
    int16_t height;
    int16_t row_bytes;
    uint32_t total_bytes;
    uint8_t *decoded = NULL;
    SB_HBITMAP hbmp = 0;

    if (!pict_handle || !pict_handle->data_00) {
        return 0;
    }
    raw = (const uint8_t *)pict_handle->data_00;
    magic = (uint16_t)(read_u16_le(raw + 0) & 0x7FFFu);
    width = (int16_t)read_u16_le(raw + 2);
    height = (int16_t)read_u16_le(raw + 4);
    row_bytes = (int16_t)read_u16_le(raw + 6);
    if (row_bytes <= 0) {
        row_bytes = (int16_t)(width + (width & 1));
    }
    total_bytes = (uint32_t)row_bytes * (uint32_t)height;

    if (magic == 0x0200u) {
        decoded = (uint8_t *)calloc(1, total_bytes + 256u);
        if (decoded != NULL) {
            size_t max_in = (pict_handle->size_08 > 8) ? (pict_handle->size_08 - 8) : total_bytes;
            decode_packbits_stream(raw + 8, max_in, decoded, total_bytes);
            hbmp = sb_win16_CreateBitmap(width, height, 1, 8, decoded);
            free(decoded);
        } else {
            hbmp = Pict_DecodeRleBanded_1010_0c94(raw + 8, row_bytes, height);
        }
    } else {
        hbmp = sb_win16_CreateBitmap(width, height, 1, 8, raw + 8);
    }

    if (target_rect != NULL && hbmp != 0) {
        SB_RECT16 native_rc;
        sb_win16_SetRect(&native_rc, 0, 0, width, height);
        if (!sb_win16_EqualRect(&native_rc, target_rect)) {
            int16_t dst_w = (int16_t)(target_rect->right - target_rect->left);
            int16_t dst_h = (int16_t)(target_rect->bottom - target_rect->top);
            if (dst_w > 0 && dst_h > 0) {
                SB_RECT16 dst_rc;
                SB_HBITMAP scaled_bmp;
                sb_win16_SetRect(&dst_rc, 0, 0, dst_w, dst_h);
                scaled_bmp = sb_win16_CreateBitmap(dst_w, dst_h, 1, 8, NULL);
                if (scaled_bmp != 0) {
                    Gdi_StretchBitmapToBitmap_1010_2ba8(scaled_bmp, &dst_rc, hbmp, &native_rc);
                    sb_win16_DeleteObject(hbmp);
                    return scaled_bmp;
                }
            }
        }
    }
    return hbmp;
}

/* Original: 1010:24ae */
SBResourceHandle *Pict_LoadResourceHandle_1010_24ae(uint16_t pict_res_id)
{
    return ResMgr_GetResource_1010_10f2(0x03f3, "PICT", pict_res_id);
}

/* Original: 1010:2ba8 */
void Gdi_StretchBitmapToBitmap_1010_2ba8(SB_HBITMAP dst_bmp,
                                         const SB_RECT16 *dst_rc,
                                         SB_HBITMAP src_bmp,
                                         const SB_RECT16 *src_rc)
{
    SB_HDC hdc_dst = sb_win16_CreateCompatibleDC(0);
    SB_HDC hdc_src = sb_win16_CreateCompatibleDC(0);
    SB_HGDIOBJ old_dst = sb_win16_SelectObject(hdc_dst, dst_bmp);
    SB_HGDIOBJ old_src = sb_win16_SelectObject(hdc_src, src_bmp);

    if (dst_rc && src_rc) {
        sb_win16_StretchBlt(hdc_dst,
                            dst_rc->left,
                            dst_rc->top,
                            (int16_t)(dst_rc->right - dst_rc->left),
                            (int16_t)(dst_rc->bottom - dst_rc->top),
                            hdc_src,
                            src_rc->left,
                            src_rc->top,
                            (int16_t)(src_rc->right - src_rc->left),
                            (int16_t)(src_rc->bottom - src_rc->top),
                            SB_SRCCOPY);
    }
    sb_win16_SelectObject(hdc_dst, old_dst);
    sb_win16_SelectObject(hdc_src, old_src);
    sb_win16_DeleteDC(hdc_dst);
    sb_win16_DeleteDC(hdc_src);
}

/* Original: 1010:2c40 */
void Gdi_BlitBitmapToDc_1010_2c40(SB_HDC dst_dc,
                                  const SB_RECT16 *dst_rc,
                                  SB_HBITMAP src_bmp,
                                  const SB_RECT16 *src_rc)
{
    SB_HDC hdc_src = sb_win16_CreateCompatibleDC(dst_dc);
    SB_HGDIOBJ old_src = sb_win16_SelectObject(hdc_src, src_bmp);

    if (dst_rc && src_rc) {
        sb_win16_StretchBlt(dst_dc,
                            dst_rc->left,
                            dst_rc->top,
                            (int16_t)(dst_rc->right - dst_rc->left),
                            (int16_t)(dst_rc->bottom - dst_rc->top),
                            hdc_src,
                            src_rc->left,
                            src_rc->top,
                            (int16_t)(src_rc->right - src_rc->left),
                            (int16_t)(src_rc->bottom - src_rc->top),
                            SB_SRCCOPY);
    }
    sb_win16_SelectObject(hdc_src, old_src);
    sb_win16_DeleteDC(hdc_src);
}

/* Original: 1010:2cc0 */
void Gdi_StretchDcToDc_1010_2cc0(SB_HDC dst_dc,
                                 const SB_RECT16 *dst_rc,
                                 SB_HDC src_dc,
                                 const SB_RECT16 *src_rc)
{
    if (!dst_rc || !src_rc) {
        return;
    }
    sb_win16_StretchBlt(dst_dc,
                        dst_rc->left,
                        dst_rc->top,
                        (int16_t)(dst_rc->right - dst_rc->left),
                        (int16_t)(dst_rc->bottom - dst_rc->top),
                        src_dc,
                        src_rc->left,
                        src_rc->top,
                        (int16_t)(src_rc->right - src_rc->left),
                        (int16_t)(src_rc->bottom - src_rc->top),
                        SB_SRCCOPY);
}

/* Original: 1010:2d2e */
SBSpriteMask *SpriteMask_Create_1010_2d2e(SB_HBITMAP hbmp_color, SB_HDC hdc_color)
{
    SBSpriteMask *mask = (SBSpriteMask *)AllocZeroed_1008_eed8(sizeof(SBSpriteMask));
    if (mask != NULL) {
        SpriteMask_InitFromBitmap_1010_2d58(hbmp_color, hdc_color, mask);
    }
    return mask;
}

/* Original: 1010:2d58 */
void SpriteMask_InitFromBitmap_1010_2d58(SB_HBITMAP hbmp_color, SB_HDC hdc_color, SBSpriteMask *out_mask)
{
    SB_BITMAP16 bm;
    if (!out_mask) {
        return;
    }
    memset(out_mask, 0, sizeof(*out_mask));
    out_mask->struct_size_00 = 0x000E;
    out_mask->hdc_color_02 = hdc_color;
    out_mask->hbmp_color_04 = hbmp_color;
    out_mask->hdc_mask_08 = sb_win16_CreateCompatibleDC(hdc_color);

    if (sb_win16_GetObjectBitmap(hbmp_color, sizeof(bm), &bm) != 0) {
        out_mask->hbmp_mask_0a = sb_win16_CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);
        out_mask->old_color_06 = sb_win16_SelectObject(out_mask->hdc_color_02, out_mask->hbmp_color_04);
        out_mask->old_mask_0c = sb_win16_SelectObject(out_mask->hdc_mask_08, out_mask->hbmp_mask_0a);
        sb_win16_SetBkColor(out_mask->hdc_color_02, 0);
        sb_win16_BitBlt(out_mask->hdc_mask_08, 0, 0, bm.bmWidth, bm.bmHeight,
                        out_mask->hdc_color_02, 0, 0, SB_SRCCOPY);
    }
}

/* Original: 1010:2e12 */
void SpriteMask_Destroy_1010_2e12(SBSpriteMask *mask)
{
    if (!mask) {
        return;
    }
    sb_win16_SelectObject(mask->hdc_color_02, mask->old_color_06);
    sb_win16_SelectObject(mask->hdc_mask_08, mask->old_mask_0c);
    sb_win16_DeleteDC(mask->hdc_mask_08);
    sb_win16_DeleteObject(mask->hbmp_mask_0a);
    FreeMem_1008_ef1e(mask);
}

/* Original: 1010:2e60 */
void Gdi_TransparentBlit_1010_2e60(SB_HDC dst_dc,
                                   const SB_RECT16 *dst_rc,
                                   SB_HBITMAP src_bmp,
                                   const SB_RECT16 *src_rc)
{
    SB_HDC hdc_src = sb_win16_CreateCompatibleDC(dst_dc);
    SB_HGDIOBJ old_src = sb_win16_SelectObject(hdc_src, src_bmp);
    SBSpriteMask mask;

    SpriteMask_InitFromBitmap_1010_2d58(src_bmp, hdc_src, &mask);
    Gdi_TransparentBlitPrecomputed_1010_2f6a(dst_dc, dst_rc, &mask, src_rc);

    sb_win16_SelectObject(mask.hdc_color_02, mask.old_color_06);
    sb_win16_SelectObject(mask.hdc_mask_08, mask.old_mask_0c);
    sb_win16_DeleteDC(mask.hdc_mask_08);
    sb_win16_DeleteObject(mask.hbmp_mask_0a);
    sb_win16_SelectObject(hdc_src, old_src);
    sb_win16_DeleteDC(hdc_src);
}

/* Original: 1010:2f6a */
void Gdi_TransparentBlitPrecomputed_1010_2f6a(SB_HDC dst_dc,
                                              const SB_RECT16 *dst_rc,
                                              const SBSpriteMask *mask,
                                              const SB_RECT16 *src_rc)
{
    if (!dst_rc || !src_rc || !mask) {
        return;
    }
    /* Step 1: SRCAND with 1-bit mask (0xFF where transparent, 0x00 where sprite foreground) */
    sb_win16_StretchBlt(dst_dc,
                        dst_rc->left,
                        dst_rc->top,
                        (int16_t)(dst_rc->right - dst_rc->left),
                        (int16_t)(dst_rc->bottom - dst_rc->top),
                        mask->hdc_mask_08,
                        src_rc->left,
                        src_rc->top,
                        (int16_t)(src_rc->right - src_rc->left),
                        (int16_t)(src_rc->bottom - src_rc->top),
                        SB_SRCAND);
    /* Step 2: SRCPAINT with color sprite (0x00 where transparent, color index where foreground) */
    sb_win16_StretchBlt(dst_dc,
                        dst_rc->left,
                        dst_rc->top,
                        (int16_t)(dst_rc->right - dst_rc->left),
                        (int16_t)(dst_rc->bottom - dst_rc->top),
                        mask->hdc_color_02,
                        src_rc->left,
                        src_rc->top,
                        (int16_t)(src_rc->right - src_rc->left),
                        (int16_t)(src_rc->bottom - src_rc->top),
                        SB_SRCPAINT);
}

/* ==========================================================================
 * 4. Offscreen Surface Classes (`CPictSurface_1008_6448`, `CLayeredOffscreen_1008_b310`, etc.)
 * ========================================================================== */

/* Original: 1008:5646 */
void CPictSurface_InitBlank_1008_5646(SBPictSurface *self,
                                      uint16_t planes,
                                      uint16_t bpp,
                                      const SB_RECT16 *bounds)
{
    if (!self) {
        return;
    }
    if (self->vtable_tag_00 == 0) {
        self->vtable_tag_00 = 0x10086448u;
    }
    self->bpp_0a = bpp;
    self->planes_0c = planes;
    CPictSurface_CreateDc_1008_57ce(self, bounds);
    if (self->hbmp_06 == 0) {
        int16_t w = (int16_t)(self->bounds_0e.right - self->bounds_0e.left);
        int16_t h = (int16_t)(self->bounds_0e.bottom - self->bounds_0e.top);
        self->hbmp_06 = sb_win16_CreateBitmap(w, h, (uint8_t)planes, (uint8_t)bpp, NULL);
        self->hOldBmp_08 = sb_win16_SelectObject(self->hdc_04, self->hbmp_06);
    }
}

/* Original: 1008:56a8 */
void CPictSurface_Dtor_1008_56a8(SBPictSurface *self)
{
    if (!self) {
        return;
    }
    if (self->hdc_04 != 0) {
        if (self->hOldBmp_08 != 0) {
            sb_win16_SelectObject(self->hdc_04, self->hOldBmp_08);
        }
        sb_win16_DeleteDC(self->hdc_04);
        self->hdc_04 = 0;
    }
    if (self->hbmp_06 != 0) {
        sb_win16_DeleteObject(self->hbmp_06);
        self->hbmp_06 = 0;
    }
    CObject_Dtor_1008_ef8e(self);
}

/* Original: 1008:56f8 */
void CPictSurface_CopyToScreenIfMode_1008_56f8(SBPictSurface *self,
                                               int16_t mode,
                                               const SB_RECT16 *dst_rc,
                                               const SB_RECT16 *src_rc)
{
    if (!self) {
        return;
    }
    if (mode == 0 || mode == 0x24) {
        Gdi_StretchDcToDc_1010_2cc0(self->hdc_04, dst_rc, g_DAT_1060_01d2_screenDC, src_rc);
    }
}

/* Original: 1008:5724 */
void CPictSurface_Noop14_1008_5724(void)
{
}

/* Original: 1008:5728 */
void CPictSurface_BlitToSurfaceByMode_1008_5728(SBPictSurface *self,
                                                int16_t mode,
                                                const SB_RECT16 *dst_rc,
                                                const SB_RECT16 *src_rc,
                                                SBPictSurface *dst_surf)
{
    if (!self) {
        return;
    }
    if (mode == 0) {
        CPictSurface_CopyToSurface_1008_5c88(self, dst_rc, src_rc, dst_surf);
    } else if (mode == 0x24) {
        CPictSurface_TransparentBlitToSurface_1008_5cea(self, 0, dst_rc, src_rc, dst_surf);
    }
}

/* Original: 1008:5786 */
void CPictSurface_BlitToScreenByMode_1008_5786(SBPictSurface *self,
                                               int16_t mode,
                                               const SB_RECT16 *dst_rc,
                                               const SB_RECT16 *src_rc)
{
    if (!self) {
        return;
    }
    if (mode == 0) {
        CPictSurface_CopyToScreen_1008_5caa(self, dst_rc, src_rc);
    } else if (mode == 0x24) {
        CPictSurface_TransparentBlitToScreen_1008_5d36(self, 0, dst_rc, src_rc);
    }
}

/* Original: 1008:57ce */
void CPictSurface_CreateDc_1008_57ce(SBPictSurface *self, const SB_RECT16 *bounds)
{
    if (!self) {
        return;
    }
    if (bounds != NULL) {
        self->bounds_0e = *bounds;
    }
    self->hdc_04 = sb_win16_CreateCompatibleDC(g_DAT_1060_01d2_screenDC);
    self->hbmp_06 = 0;
}

/* Original: 1008:5804 */
void CPictSurface_LoadPictResource_1008_5804(SBPictSurface *self,
                                             const SB_RECT16 *dst_rc,
                                             const SB_RECT16 *src_rc,
                                             uint16_t pict_res_id)
{
    SBResourceHandle *h;
    const uint8_t *raw;
    uint8_t is_transparent;
    uint8_t had_no_bmp;
    uint8_t eq_bounds;
    SB_HBITMAP decoded_bmp;

    if (!self) {
        return;
    }
    h = ResMgr_GetResource_1010_10f2(0x03f3, "PICT", pict_res_id);
    if (!h || !h->data_00) {
        return;
    }
    raw = (const uint8_t *)h->data_00;
    if (Rect_IsEmpty_1010_0f1c(&self->bounds_0e)) {
        int16_t w = (int16_t)read_u16_le(raw + 2);
        int16_t h_px = (int16_t)read_u16_le(raw + 4);
        Rect_Set_1010_0fb6(&self->bounds_0e, 0, 0, w, h_px);
    }
    is_transparent = ((raw[1] & 0x80u) != 0) ? 1u : 0u;
    had_no_bmp = (self->hbmp_06 == 0) ? 1u : 0u;
    eq_bounds = dst_rc ? (uint8_t)Rect_Equal_1010_0f2a(&self->bounds_0e, dst_rc) : 1u;

    if (!had_no_bmp && eq_bounds && !is_transparent) {
        sb_win16_SelectObject(self->hdc_04, self->hOldBmp_08);
        sb_win16_DeleteObject(self->hbmp_06);
        self->hbmp_06 = 0;
    }

    decoded_bmp = Pict_DecodeToBitmap_1010_08e0(h, src_rc);
    Handle_AllowPurge_1010_0196(h);

    if (eq_bounds && (!is_transparent || had_no_bmp)) {
        self->hbmp_06 = decoded_bmp;
        self->hOldBmp_08 = sb_win16_SelectObject(self->hdc_04, decoded_bmp);
        return;
    }

    if (had_no_bmp) {
        int16_t w = (int16_t)(self->bounds_0e.right + (self->bounds_0e.right & 1));
        int16_t h_px = self->bounds_0e.bottom;
        self->hbmp_06 = sb_win16_CreateBitmap(w, h_px, 1, 8, NULL);
        self->hOldBmp_08 = sb_win16_SelectObject(self->hdc_04, self->hbmp_06);
    }
    if (src_rc && dst_rc) {
        SB_RECT16 norm_src;
        Rect_Set_1010_0fb6(&norm_src,
                           0,
                           0,
                           (int16_t)(src_rc->right - src_rc->left),
                           (int16_t)(src_rc->bottom - src_rc->top));
        if (is_transparent && !had_no_bmp) {
            Gdi_TransparentBlit_1010_2e60(self->hdc_04, dst_rc, decoded_bmp, &norm_src);
        } else {
            Gdi_BlitBitmapToDc_1010_2c40(self->hdc_04, dst_rc, decoded_bmp, &norm_src);
        }
    }
    sb_win16_DeleteObject(decoded_bmp);
}

/* Original: 1008:5a0a */
void CPictSurface_FillBlack_1008_5a0a(SBPictSurface *self, const SB_RECT16 *rc)
{
    if (self && rc) {
        sb_win16_FillRectColor(self->hdc_04, rc, 0);
    }
}

/* Original: 1008:5a2a */
void CPictSurface_ClipRects_1008_5a2a(const SB_RECT16 *dst_clip,
                                      const SB_RECT16 *src_clip,
                                      SB_RECT16 *io_dst_rc,
                                      SB_RECT16 *io_src_rc)
{
    int16_t d;
    if (!dst_clip || !src_clip || !io_dst_rc || !io_src_rc) {
        return;
    }
    if (io_src_rc->left < src_clip->left) {
        d = (int16_t)(src_clip->left - io_src_rc->left);
        io_src_rc->left = (int16_t)(io_src_rc->left + d);
        io_dst_rc->left = (int16_t)(io_dst_rc->left + d);
    }
    if (src_clip->right < io_src_rc->right) {
        d = (int16_t)(io_src_rc->right - src_clip->right);
        io_src_rc->right = (int16_t)(io_src_rc->right - d);
        io_dst_rc->right = (int16_t)(io_dst_rc->right - d);
    }
    if (io_src_rc->top < src_clip->top) {
        d = (int16_t)(src_clip->top - io_src_rc->top);
        io_src_rc->top = (int16_t)(io_src_rc->top + d);
        io_dst_rc->top = (int16_t)(io_dst_rc->top + d);
    }
    if (src_clip->bottom < io_src_rc->bottom) {
        d = (int16_t)(io_src_rc->bottom - src_clip->bottom);
        io_src_rc->bottom = (int16_t)(io_src_rc->bottom - d);
        io_dst_rc->bottom = (int16_t)(io_dst_rc->bottom - d);
    }
    if (io_dst_rc->left < dst_clip->left) {
        d = (int16_t)(dst_clip->left - io_dst_rc->left);
        io_dst_rc->left = (int16_t)(io_dst_rc->left + d);
        io_src_rc->left = (int16_t)(io_src_rc->left + d);
    }
    if (dst_clip->right < io_dst_rc->right) {
        d = (int16_t)(io_dst_rc->right - dst_clip->right);
        io_dst_rc->right = (int16_t)(io_dst_rc->right - d);
        io_src_rc->right = (int16_t)(io_src_rc->right - d);
    }
    if (io_dst_rc->top < dst_clip->top) {
        d = (int16_t)(dst_clip->top - io_dst_rc->top);
        io_dst_rc->top = (int16_t)(io_dst_rc->top + d);
        io_src_rc->top = (int16_t)(io_src_rc->top + d);
    }
    if (dst_clip->bottom < io_dst_rc->bottom) {
        d = (int16_t)(io_dst_rc->bottom - dst_clip->bottom);
        io_dst_rc->bottom = (int16_t)(io_dst_rc->bottom - d);
        io_src_rc->bottom = (int16_t)(io_src_rc->bottom - d);
    }
}

/* Original: 1008:5b78 */
void CPictSurface_FillColorRect_1008_5b78(SBPictSurface *self, int16_t color_idx, const SB_RECT16 *rc)
{
    uint8_t actual_color;
    if (!self || !rc) {
        return;
    }
    if (color_idx == 0 || color_idx == 0xFF) {
        actual_color = (uint8_t)(color_idx ^ 0xFF);
    } else {
        actual_color = (uint8_t)(color_idx + 9);
    }
    sb_win16_FillRectColor(self->hdc_04, rc, actual_color);
}

/* Original: 1008:5c68 */
void CPictSurface_CopyFromScreen_1008_5c68(SBPictSurface *self,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc)
{
    if (self) {
        Gdi_StretchDcToDc_1010_2cc0(self->hdc_04, dst_rc, g_DAT_1060_01d2_screenDC, src_rc);
    }
}

/* Original: 1008:5c88 */
void CPictSurface_CopyToSurface_1008_5c88(SBPictSurface *self,
                                          const SB_RECT16 *dst_rc,
                                          const SB_RECT16 *src_rc,
                                          SBPictSurface *dst_surf)
{
    if (self && dst_surf) {
        Gdi_StretchDcToDc_1010_2cc0(dst_surf->hdc_04, dst_rc, self->hdc_04, src_rc);
    }
}

/* Original: 1008:5caa */
void CPictSurface_CopyToScreen_1008_5caa(SBPictSurface *self,
                                         const SB_RECT16 *dst_rc,
                                         const SB_RECT16 *src_rc)
{
    if (self) {
        Gdi_StretchDcToDc_1010_2cc0(g_DAT_1060_01d2_screenDC, dst_rc, self->hdc_04, src_rc);
    }
}

/* Original: 1008:5cca */
void CPictSurface_ClearScreenRect_1008_5cca(const SB_RECT16 *rc)
{
    if (rc) {
        sb_win16_FillRectColor(g_DAT_1060_01d2_screenDC, rc, 0);
    }
}

/* Original: 1008:5cea */
void CPictSurface_TransparentBlitToSurface_1008_5cea(SBPictSurface *self,
                                                     uint8_t use_precomputed_mask,
                                                     const SB_RECT16 *dst_rc,
                                                     const SB_RECT16 *src_rc,
                                                     SBPictSurface *dst_surf)
{
    if (!self || !dst_surf) {
        return;
    }
    if (use_precomputed_mask == 0 || self->mask_18 == NULL) {
        Gdi_TransparentBlit_1010_2e60(dst_surf->hdc_04, dst_rc, self->hbmp_06, src_rc);
    } else {
        Gdi_TransparentBlitPrecomputed_1010_2f6a(dst_surf->hdc_04, dst_rc, self->mask_18, src_rc);
    }
}

/* Original: 1008:5d36 */
void CPictSurface_TransparentBlitToScreen_1008_5d36(SBPictSurface *self,
                                                    uint8_t use_precomputed_mask,
                                                    const SB_RECT16 *dst_rc,
                                                    const SB_RECT16 *src_rc)
{
    if (!self) {
        return;
    }
    if (use_precomputed_mask == 0 || self->mask_18 == NULL) {
        Gdi_TransparentBlit_1010_2e60(g_DAT_1060_01d2_screenDC, dst_rc, self->hbmp_06, src_rc);
    } else {
        Gdi_TransparentBlitPrecomputed_1010_2f6a(g_DAT_1060_01d2_screenDC, dst_rc, self->mask_18, src_rc);
    }
}

/* Original: 1008:5d7c */
SB_HBITMAP CPictSurface_GetHBitmap_1008_5d7c(const SBPictSurface *self)
{
    return self ? self->hbmp_06 : 0;
}

/* Original: 1008:5d8a */
SB_HDC CPictSurface_GetHDc_1008_5d8a(const SBPictSurface *self)
{
    return self ? self->hdc_04 : 0;
}

/* Original: 1008:5d98 */
uint8_t CPictSurface_GetPixel_1008_5d98(const SBPictSurface *self, int16_t x, int16_t y)
{
    int16_t w = 0, h = 0;
    const uint8_t *px;
    if (!self) {
        return 0;
    }
    px = sb_win16_gdi_getBitmapPixels(self->hbmp_06, &w, &h);
    if (!px || x < 0 || x >= w || y < 0 || y >= h) {
        return 0;
    }
    return px[(size_t)y * (w + (w & 1)) + x];
}

/* Original: 1008:5e14 */
void CLayeredOffscreen_InitFromPict_1008_5e14(SBPictSurface *self,
                                              uint16_t planes,
                                              uint16_t bpp,
                                              uint16_t pict_res_id)
{
    SBResourceHandle *h;
    SB_RECT16 rc = {0, 0, 0, 0};

    if (!self) {
        return;
    }
    if (self->vtable_tag_00 == 0) {
        self->vtable_tag_00 = 0x1008b310u;
    }
    self->pict_res_id_16 = pict_res_id;
    h = ResMgr_GetResource_1010_10f2(0x03f3, "PICT", pict_res_id);
    if (h != NULL && h->data_00 != NULL) {
        const uint8_t *raw = (const uint8_t *)h->data_00;
        Rect_Set_1010_0fb6(&rc, 0, 0, (int16_t)read_u16_le(raw + 2), (int16_t)read_u16_le(raw + 4));
    }
    self->bpp_0a = bpp;
    self->planes_0c = planes;
    CPictSurface_CreateDc_1008_57ce(self, &rc);
    if (self->hbmp_06 == 0) {
        self->hbmp_06 = sb_win16_CreateBitmap(rc.right, rc.bottom, (uint8_t)planes, (uint8_t)bpp, NULL);
        self->hOldBmp_08 = sb_win16_SelectObject(self->hdc_04, self->hbmp_06);
    }
    CPictSurface_LoadPictResource_1008_5804(self, &rc, &rc, pict_res_id);
}

/* Original: 1008:5ed4 */
void CLayeredOffscreen_Forward18_1008_5ed4(SBPictSurface *self,
                                           int16_t mode,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc,
                                           SBPictSurface *dst_surf)
{
    CPictSurface_BlitToSurfaceByMode_1008_5728(self, mode, dst_rc, src_rc, dst_surf);
}

/* Original: 1008:5f02 */
void CLayeredOffscreen_Forward1c_1008_5f02(SBPictSurface *self,
                                           int16_t mode,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc)
{
    CPictSurface_BlitToScreenByMode_1008_5786(self, mode, dst_rc, src_rc);
}

/* Original: 1008:5f2c */
void CLayeredOffscreen_Forward38_1008_5f2c(SBPictSurface *self,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc,
                                           SBPictSurface *dst_surf)
{
    CPictSurface_CopyToSurface_1008_5c88(self, dst_rc, src_rc, dst_surf);
}

/* Original: 1008:5f52 */
void CLayeredOffscreen_Forward3c_1008_5f52(SBPictSurface *self,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc)
{
    CPictSurface_CopyToScreen_1008_5caa(self, dst_rc, src_rc);
}

/* Original: 1008:5f6e */
void CMaskedPictSurface_InitFromPict_1008_5f6e(SBPictSurface *self,
                                               uint16_t planes,
                                               uint16_t bpp,
                                               uint16_t pict_res_id)
{
    if (!self) {
        return;
    }
    self->vtable_tag_00 = 0x1008b384u;
    CLayeredOffscreen_InitFromPict_1008_5e14(self, planes, bpp, pict_res_id);
    self->mask_18 = SpriteMask_Create_1010_2d2e(self->hbmp_06, self->hdc_04);
}

/* Original: 1008:604a */
void CMaskedPictSurface_Dtor_1008_604a(SBPictSurface *self)
{
    if (!self) {
        return;
    }
    if (self->mask_18 != NULL) {
        SpriteMask_Destroy_1010_2e12(self->mask_18);
        self->mask_18 = NULL;
    }
    CPictSurface_Dtor_1008_56a8(self);
}

/* Original: 1008:60b2 */
void CMaskedPictSurface_Forward18_1008_60b2(SBPictSurface *self,
                                            int16_t mode,
                                            const SB_RECT16 *dst_rc,
                                            const SB_RECT16 *src_rc,
                                            SBPictSurface *dst_surf)
{
    CPictSurface_BlitToSurfaceByMode_1008_5728(self, mode, dst_rc, src_rc, dst_surf);
}

/* Original: 1008:60e0 */
void CMaskedPictSurface_Forward1c_1008_60e0(SBPictSurface *self,
                                            int16_t mode,
                                            const SB_RECT16 *dst_rc,
                                            const SB_RECT16 *src_rc)
{
    CPictSurface_BlitToScreenByMode_1008_5786(self, mode, dst_rc, src_rc);
}

/* Original: 1008:610a */
void CMaskedPictSurface_BlitToSurface_1008_610a(SBPictSurface *self,
                                                const SB_RECT16 *dst_rc,
                                                const SB_RECT16 *src_rc,
                                                SBPictSurface *dst_surf)
{
    if (self && dst_surf) {
        Gdi_TransparentBlitPrecomputed_1010_2f6a(dst_surf->hdc_04, dst_rc, self->mask_18, src_rc);
    }
}

/* Original: 1008:612e */
void CMaskedPictSurface_BlitToScreen_1008_612e(SBPictSurface *self,
                                               const SB_RECT16 *dst_rc,
                                               const SB_RECT16 *src_rc)
{
    if (self) {
        Gdi_TransparentBlitPrecomputed_1010_2f6a(g_DAT_1060_01d2_screenDC, dst_rc, self->mask_18, src_rc);
    }
}

/* Original: 1008:614e */
void CTiledPictSurface_InitFromPict_1008_614e(SBPictSurface *self,
                                              uint16_t planes,
                                              uint16_t bpp,
                                              const SB_RECT16 *full_bounds,
                                              const SB_RECT16 *tile_step,
                                              uint16_t pict_res_id)
{
    if (!self) {
        return;
    }
    self->vtable_tag_00 = 0x10188476u;
    if (full_bounds) self->tile_bounds_1c = *full_bounds;
    if (tile_step) self->tile_step_24 = *tile_step;
    self->pict_res_id_16 = pict_res_id;
    self->bpp_0a = bpp;
    self->planes_0c = planes;
    CTiledPictSurface_CreateTiled_1008_6246(self, full_bounds);
}

/* Original: 1008:6246 */
void CTiledPictSurface_CreateTiled_1008_6246(SBPictSurface *self, const SB_RECT16 *bounds)
{
    int16_t tile_w, tile_h;
    int16_t cols, rows, r, c;
    SB_RECT16 tile_rc;

    if (!self || !bounds) {
        return;
    }
    self->bounds_0e = *bounds;
    self->hdc_04 = sb_win16_CreateCompatibleDC(g_DAT_1060_01d2_screenDC);
    self->hbmp_06 = sb_win16_CreateBitmap(
        (int16_t)(bounds->right - bounds->left),
        (int16_t)(bounds->bottom - bounds->top),
        (uint8_t)self->planes_0c,
        (uint8_t)self->bpp_0a,
        NULL);
    self->hOldBmp_08 = sb_win16_SelectObject(self->hdc_04, self->hbmp_06);

    tile_w = (int16_t)(self->tile_step_24.right - self->tile_step_24.left);
    tile_h = (int16_t)(self->tile_step_24.bottom - self->tile_step_24.top);
    if (tile_w <= 0 || tile_h <= 0) {
        CPictSurface_LoadPictResource_1008_5804(self, bounds, bounds, self->pict_res_id_16);
        return;
    }
    Rect_Set_1010_0fb6(&tile_rc, 0, 0, tile_w, tile_h);
    CPictSurface_LoadPictResource_1008_5804(self, &tile_rc, &tile_rc, self->pict_res_id_16);

    cols = (int16_t)((bounds->right - bounds->left) / tile_w);
    rows = (int16_t)((bounds->bottom - bounds->top) / tile_h);
    for (r = 0; r < rows; ++r) {
        for (c = 0; c < cols; ++c) {
            if (r != 0 || c != 0) {
                SB_RECT16 dst_tile = tile_rc;
                Rect_Offset_1010_0f74(&dst_tile, (int16_t)(c * tile_w), (int16_t)(r * tile_h));
                CPictSurface_CopyToSurface_1008_5c88(self, &dst_tile, &tile_rc, self);
            }
        }
    }
}

/* Original: 1008:6436 */
void CPictSurface_Noop0c_1008_6436(void)
{
}

/* Original: 1008:643a */
uint8_t CPictSurface_True58_1008_643a(void)
{
    return 1;
}

/* Original: 1008:6440 */
void CPictSurface_Noop5c_1008_6440(void)
{
}

/* Original: 1008:6444 */
void CPictSurface_Noop60_1008_6444(void)
{
}

/* Original: 1008:64ac
 * Generates a 32x32 procedural water/ice shimmer noise tile across palette
 * indices `0xa8..0xaf` (168..175) and replaces all pixels in `surface` whose
 * color index is in `[0xa8, 0xaf]` with the animated shimmer value!
 */
void WaterShimmer_AnimateBitmap_1008_64ac(SBPictSurface *surface)
{
    uint8_t noise[32 * 32];
    uint8_t cur = 0xA7;
    int i, y, x;
    int16_t w, h, stride;
    uint32_t total_bytes;
    uint8_t *pixels;

    if (!surface || surface->hbmp_06 == 0) {
        return;
    }
    for (i = 0; i < 32 * 32; ++i) {
        uint8_t r = Random_Byte_1010_07ba();
        uint8_t next = (uint8_t)(cur + (r & 1u) + 1u);
        if (next > 0xAFu) {
            next = (uint8_t)(cur + (r & 1u) - 7u);
        }
        cur = next;
        noise[i] = cur;
    }

    w = (int16_t)(surface->bounds_0e.right - surface->bounds_0e.left);
    h = (int16_t)(surface->bounds_0e.bottom - surface->bounds_0e.top);
    stride = (int16_t)(w + (w & 1));
    total_bytes = (uint32_t)stride * (uint32_t)h;
    pixels = (uint8_t *)AllocZeroed_1008_eed8(total_bytes);
    if (!pixels) {
        return;
    }
    sb_win16_GetBitmapBits(surface->hbmp_06, (int32_t)total_bytes, pixels);
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            uint8_t p = pixels[y * stride + x];
            if (p > 0xA7u && p < 0xB0u) {
                pixels[y * stride + x] = noise[(y % 32) + (x % 32) * 32];
            }
        }
    }
    sb_win16_SetBitmapBits(surface->hbmp_06, (int32_t)total_bytes, pixels);
    FreeMem_1008_ef1e(pixels);
}
