/* Decompiled Bright Star Graphics, Palette (`clut`), & Offscreen `PICT` Surface Subsystem (Path 2)
 *
 * Reconstructs the 64 functions governing 256-color palette management,
 * PackBits RLE `PICT` (`0x03f3`) sprite decompression, transparent 1-bit mask
 * generation, and offscreen layered GDI rendering:
 *
 * 1. Geometry & Rectangle Helpers (`1010:0f1c..0fd0`):
 *    - `Rect_IsEmpty_1010_0f1c` (`1010:0f1c`)
 *    - `Rect_Equal_1010_0f2a` (`1010:0f2a`)
 *    - `Rect_Inset_1010_0f3c` (`1010:0f3c`)
 *    - `Rect_Offset_1010_0f74` (`1010:0f74`)
 *    - `Rect_Intersect_1010_0f88` (`1010:0f88`)
 *    - `Point_Set_1010_0f9e` (`1010:0f9e`)
 *    - `Rect_Set_1010_0fb6` (`1010:0fb6`)
 *    - `Rect_Union_1010_0fd0` (`1010:0fd0`)
 *
 * 2. Palette Manager (`CPaletteManager_1008_8278` at `DAT_1060_0198`):
 *    - `CPaletteManager_Init_1008_326e` (`1008:326e`)
 *    - `CPaletteManager_Dtor_1008_3464` (`1008:3464`)
 *    - `CPaletteManager_FadeToCurrent_1008_34bc` (`1008:34bc`)
 *    - `CPaletteManager_FadeOut_1008_34e6` (`1008:34e6`)
 *    - `CPaletteManager_ApplyTargetRange_1008_3596` (`1008:3596`)
 *    - `CPaletteManager_ApplyBlackRange_1008_3624` (`1008:3624`)
 *    - `CPaletteManager_LoadClutResource_1008_36b2` (`1008:36b2`)
 *    - `CPaletteManager_SaveAndRealize_1008_382e` (`1008:382e`)
 *    - `CPaletteManager_RestoreAndRealize_1008_3950` (`1008:3950`)
 *    - `CPaletteManager_ResetToInitial_1008_3a74` (`1008:3a74`)
 *    - `CPaletteManager_CycleRange_1008_3b44` (`1008:3b44`)
 *    - `CPaletteManager_FindColorIndex_1008_3c5c` (`1008:3c5c`)
 *    - `CPaletteManager_CrossfadeRange_1008_3cd2` (`1008:3cd2`)
 *    - `CPaletteManager_AnimateAll_1008_3e34` (`1008:3e34`)
 *    - `CPaletteManager_GetHPalette_1008_7cb4` (`1008:7cb4`)
 *
 * 3. PackBits RLE `PICT` Decoder & Transparent Mask Blitters (`1010:0614..0c94`, `1010:2ba8..2f6a`):
 *    - `Global_FreePtr_1010_0614` (`1010:0614`)
 *    - `Random_Byte_1010_07ba` (`1010:07ba`)
 *    - `Pict_DecodeToBitmap_1010_08e0` (`1010:08e0`)
 *    - `Pict_DecodeRleBanded_1010_0c94` (`1010:0c94`)
 *    - `Pict_LoadResourceHandle_1010_24ae` (`1010:24ae`)
 *    - `Gdi_StretchBitmapToBitmap_1010_2ba8` (`1010:2ba8`)
 *    - `Gdi_BlitBitmapToDc_1010_2c40` (`1010:2c40`)
 *    - `Gdi_StretchDcToDc_1010_2cc0` (`1010:2cc0`)
 *    - `SpriteMask_Create_1010_2d2e` (`1010:2d2e`)
 *    - `SpriteMask_InitFromBitmap_1010_2d58` (`1010:2d58`)
 *    - `SpriteMask_Destroy_1010_2e12` (`1010:2e12`)
 *    - `Gdi_TransparentBlit_1010_2e60` (`1010:2e60`)
 *    - `Gdi_TransparentBlitPrecomputed_1010_2f6a` (`1010:2f6a`)
 *
 * 4. Offscreen Surface Classes (`CPictSurface_1008_6448`, `CLayeredOffscreen_1008_b310`, `CMaskedPictSurface_1008_b384`, `CTiledPictSurface_1018_8476`):
 *    - `CPictSurface_InitBlank_1008_5646` (`1008:5646`)
 *    - `CPictSurface_Dtor_1008_56a8` (`1008:56a8`)
 *    - `CPictSurface_CopyToScreenIfMode_1008_56f8` (`1008:56f8`)
 *    - `CPictSurface_Noop14_1008_5724` (`1008:5724`)
 *    - `CPictSurface_BlitToSurfaceByMode_1008_5728` (`1008:5728`)
 *    - `CPictSurface_BlitToScreenByMode_1008_5786` (`1008:5786`)
 *    - `CPictSurface_CreateDc_1008_57ce` (`1008:57ce`)
 *    - `CPictSurface_LoadPictResource_1008_5804` (`1008:5804`)
 *    - `CPictSurface_FillBlack_1008_5a0a` (`1008:5a0a`)
 *    - `CPictSurface_ClipRects_1008_5a2a` (`1008:5a2a`)
 *    - `CPictSurface_FillColorRect_1008_5b78` (`1008:5b78`)
 *    - `CPictSurface_CopyFromScreen_1008_5c68` (`1008:5c68`)
 *    - `CPictSurface_CopyToSurface_1008_5c88` (`1008:5c88`)
 *    - `CPictSurface_CopyToScreen_1008_5caa` (`1008:5caa`)
 *    - `CPictSurface_ClearScreenRect_1008_5cca` (`1008:5cca`)
 *    - `CPictSurface_TransparentBlitToSurface_1008_5cea` (`1008:5cea`)
 *    - `CPictSurface_TransparentBlitToScreen_1008_5d36` (`1008:5d36`)
 *    - `CPictSurface_GetHBitmap_1008_5d7c` (`1008:5d7c`)
 *    - `CPictSurface_GetHDc_1008_5d8a` (`1008:5d8a`)
 *    - `CPictSurface_GetPixel_1008_5d98` (`1008:5d98`)
 *    - `CLayeredOffscreen_InitFromPict_1008_5e14` (`1008:5e14`)
 *    - `CLayeredOffscreen_Forward18_1008_5ed4` (`1008:5ed4`)
 *    - `CLayeredOffscreen_Forward1c_1008_5f02` (`1008:5f02`)
 *    - `CLayeredOffscreen_Forward38_1008_5f2c` (`1008:5f2c`)
 *    - `CLayeredOffscreen_Forward3c_1008_5f52` (`1008:5f52`)
 *    - `CMaskedPictSurface_InitFromPict_1008_5f6e` (`1008:5f6e`)
 *    - `CMaskedPictSurface_Dtor_1008_604a` (`1008:604a`)
 *    - `CMaskedPictSurface_Forward18_1008_60b2` (`1008:60b2`)
 *    - `CMaskedPictSurface_Forward1c_1008_60e0` (`1008:60e0`)
 *    - `CMaskedPictSurface_BlitToSurface_1008_610a` (`1008:610a`)
 *    - `CMaskedPictSurface_BlitToScreen_1008_612e` (`1008:612e`)
 *    - `CTiledPictSurface_InitFromPict_1008_614e` (`1008:614e`)
 *    - `CTiledPictSurface_CreateTiled_1008_6246` (`1008:6246`)
 *    - `CPictSurface_Noop0c_1008_6436` (`1008:6436`)
 *    - `CPictSurface_True58_1008_643a` (`1008:643a`)
 *    - `CPictSurface_Noop5c_1008_6440` (`1008:6440`)
 *    - `CPictSurface_Noop60_1008_6444` (`1008:6444`)
 *    - `WaterShimmer_AnimateBitmap_1008_64ac` (`1008:64ac`)
 */
#ifndef SB_RECOVERED_FRAMEWORK_GRAPHICS_GDI_H
#define SB_RECOVERED_FRAMEWORK_GRAPHICS_GDI_H

#include <stddef.h>
#include <stdint.h>
#include "../../shim/win16_gdi.h"
#include "audio_waveout.h"
#include "resource_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 14-byte (`0x0e`) Precomputed Transparent Sprite Mask (`1010:2d2e` / `1010:2d58`) */
typedef struct SBSpriteMask {
    uint16_t struct_size_00;  /* +0x00: 0x000e */
    SB_HDC hdc_color_02;      /* +0x02: DC with color sprite selected */
    SB_HBITMAP hbmp_color_04; /* +0x04: color sprite HBITMAP */
    SB_HGDIOBJ old_color_06;  /* +0x06: previous object in hdc_color_02 */
    SB_HDC hdc_mask_08;       /* +0x08: DC with 1-bit monochrome mask selected */
    SB_HBITMAP hbmp_mask_0a;  /* +0x0a: 1-bit monochrome mask HBITMAP */
    SB_HGDIOBJ old_mask_0c;   /* +0x0c: previous object in hdc_mask_08 */
} SBSpriteMask;

/* Global Palette Manager (`CPaletteManager_1008_8278`, `0x1806` bytes, at `DAT_1060_0198`) */
typedef struct SBPaletteManager {
    uint32_t vtable_tag_00;           /* +0x00: 0x10088278 */
    SB_LOGPALETTE *logpal_04;         /* +0x04: allocated LOGPALETTE (236 entries) */
    SB_HPALETTE hPalette_08;          /* +0x08: active Win16 HPALETTE */
    SB_HPALETTE hOldPalette_0a;       /* +0x0a: previous HPALETTE */
    SB_PALETTEENTRY initial_0c[236];  /* +0x0c..+0x3bb: initial system palette */
    SB_PALETTEENTRY current_3bc[236]; /* +0x3bc..+0x76b: current animated palette */
    SB_PALETTEENTRY target_76c[236];  /* +0x76c..+0xb1b: target CLUT palette */
    SB_PALETTEENTRY saved_b1c[236];   /* +0xb1c..+0xecb: saved palette backup */
    SB_PALETTEENTRY black_ecc[236];   /* +0xecc..+0x127b: all-black fade palette */
    uint16_t raw_rgb16_127c[236][3];  /* +0x127c..+0x1803: 16-bit Mac RGBwords from primary CLUT */
    uint8_t is_saved_1804;            /* +0x1804 (`puVar9 + 0x601`) */
} SBPaletteManager;

/* Offscreen Surface (`CPictSurface_1008_6448` / `CLayeredOffscreen_1008_b310` /
 * `CMaskedPictSurface_1008_b384` / `CTiledPictSurface_1018_8476`)
 */
typedef struct SBPictSurface {
    uint32_t vtable_tag_00;   /* +0x00: 0x10086448, 0x1008b310, 0x1008b384, or 0x10188476 */
    SB_HDC hdc_04;            /* +0x04: memory DC */
    SB_HBITMAP hbmp_06;       /* +0x06: 8-bit HBITMAP */
    SB_HGDIOBJ hOldBmp_08;    /* +0x08: previous HBITMAP in hdc_04 */
    uint16_t bpp_0a;          /* +0x0a: bits per pixel (8) */
    uint16_t planes_0c;       /* +0x0c: planes (1) */
    SB_RECT16 bounds_0e;      /* +0x0e..+0x15: surface rectangle (left, top, right, bottom) */
    uint16_t pict_res_id_16;  /* +0x16: loaded `'PICT'` (`0x03f3`) resource ID */
    SBSpriteMask *mask_18;    /* +0x18: precomputed transparent mask (`CMaskedPictSurface_1008_b384`) */
    SB_RECT16 tile_bounds_1c; /* +0x1c..+0x23 (`CTiledPictSurface_1018_8476`) */
    SB_RECT16 tile_step_24;   /* +0x24..+0x2b (`CTiledPictSurface_1018_8476`) */
} SBPictSurface;

/* Globals in Segment 1060 */
extern SBPaletteManager *g_DAT_1060_0198_paletteMgr;
extern SB_HDC g_DAT_1060_01d2_screenDC;

/* 1. Geometry & Rectangle Helpers (`1010:0f1c..0fd0`) */
int Rect_IsEmpty_1010_0f1c(const SB_RECT16 *rc);
int Rect_Equal_1010_0f2a(const SB_RECT16 *a, const SB_RECT16 *b);
void Rect_Inset_1010_0f3c(SB_RECT16 *rc, int16_t dx, int16_t dy);
void Rect_Offset_1010_0f74(SB_RECT16 *rc, int16_t dx, int16_t dy);
int Rect_Intersect_1010_0f88(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b);
void Point_Set_1010_0f9e(SB_POINT16 *pt, int16_t x, int16_t y);
void Rect_Set_1010_0fb6(SB_RECT16 *rc, int16_t left, int16_t top, int16_t right, int16_t bottom);
int Rect_Union_1010_0fd0(SB_RECT16 *dst, const SB_RECT16 *a, const SB_RECT16 *b);

/* 2. Palette Manager (`CPaletteManager_1008_8278`) */
SBPaletteManager *CPaletteManager_Init_1008_326e(SBPaletteManager *self);
void CPaletteManager_Dtor_1008_3464(SBPaletteManager *self);
void CPaletteManager_FadeToCurrent_1008_34bc(SBPaletteManager *self, int16_t start_1based, int16_t end_1based);
void CPaletteManager_FadeOut_1008_34e6(SBPaletteManager *self, int16_t start_1based, int16_t end_1based);
void CPaletteManager_ApplyTargetRange_1008_3596(SBPaletteManager *self, int16_t end_1based, int16_t start_1based);
void CPaletteManager_ApplyBlackRange_1008_3624(SBPaletteManager *self, int16_t end_1based, int16_t start_1based);
void CPaletteManager_LoadClutResource_1008_36b2(SBPaletteManager *self, uint16_t clut_res_id);
void CPaletteManager_SaveAndRealize_1008_382e(SBPaletteManager *self);
void CPaletteManager_RestoreAndRealize_1008_3950(SBPaletteManager *self);
void CPaletteManager_ResetToInitial_1008_3a74(SBPaletteManager *self);
void CPaletteManager_CycleRange_1008_3b44(SBPaletteManager *self, int16_t from_1based, int16_t to_1based);
int16_t CPaletteManager_FindColorIndex_1008_3c5c(const SBPaletteManager *self,
                                                 const uint16_t rgb16[3],
                                                 uint32_t *out_rgb);
void CPaletteManager_CrossfadeRange_1008_3cd2(SBPaletteManager *self,
                                              int16_t end_1based,
                                              int16_t start_1based,
                                              const SB_PALETTEENTRY *to_pal,
                                              const SB_PALETTEENTRY *from_pal);
void CPaletteManager_AnimateAll_1008_3e34(SBPaletteManager *self, const SB_PALETTEENTRY *entries_236);
SB_HPALETTE CPaletteManager_GetHPalette_1008_7cb4(const SBPaletteManager *self);

/* 3. PackBits RLE `PICT` Decoder & Transparent Mask Blitters */
void Global_FreePtr_1010_0614(void *ptr);
uint8_t Random_Byte_1010_07ba(void);
SB_HBITMAP Pict_DecodeToBitmap_1010_08e0(SBResourceHandle *pict_handle, const SB_RECT16 *target_rect);
SB_HBITMAP Pict_DecodeRleBanded_1010_0c94(const uint8_t *rle_data, int16_t width_bytes, int16_t height);
SBResourceHandle *Pict_LoadResourceHandle_1010_24ae(uint16_t pict_res_id);
void Gdi_StretchBitmapToBitmap_1010_2ba8(SB_HBITMAP dst_bmp,
                                         const SB_RECT16 *dst_rc,
                                         SB_HBITMAP src_bmp,
                                         const SB_RECT16 *src_rc);
void Gdi_BlitBitmapToDc_1010_2c40(SB_HDC dst_dc,
                                  const SB_RECT16 *dst_rc,
                                  SB_HBITMAP src_bmp,
                                  const SB_RECT16 *src_rc);
void Gdi_StretchDcToDc_1010_2cc0(SB_HDC dst_dc,
                                 const SB_RECT16 *dst_rc,
                                 SB_HDC src_dc,
                                 const SB_RECT16 *src_rc);
SBSpriteMask *SpriteMask_Create_1010_2d2e(SB_HBITMAP hbmp_color, SB_HDC hdc_color);
void SpriteMask_InitFromBitmap_1010_2d58(SB_HBITMAP hbmp_color, SB_HDC hdc_color, SBSpriteMask *out_mask);
void SpriteMask_Destroy_1010_2e12(SBSpriteMask *mask);
void Gdi_TransparentBlit_1010_2e60(SB_HDC dst_dc,
                                   const SB_RECT16 *dst_rc,
                                   SB_HBITMAP src_bmp,
                                   const SB_RECT16 *src_rc);
void Gdi_TransparentBlitPrecomputed_1010_2f6a(SB_HDC dst_dc,
                                              const SB_RECT16 *dst_rc,
                                              const SBSpriteMask *mask,
                                              const SB_RECT16 *src_rc);

/* 4. Offscreen Surface Classes (`CPictSurface_1008_6448`, `CLayeredOffscreen_1008_b310`, `CMaskedPictSurface_1008_b384`, `CTiledPictSurface_1018_8476`) */
void CPictSurface_InitBlank_1008_5646(SBPictSurface *self,
                                      uint16_t planes,
                                      uint16_t bpp,
                                      const SB_RECT16 *bounds);
void CPictSurface_Dtor_1008_56a8(SBPictSurface *self);
void CPictSurface_CopyToScreenIfMode_1008_56f8(SBPictSurface *self,
                                               int16_t mode,
                                               const SB_RECT16 *dst_rc,
                                               const SB_RECT16 *src_rc);
void CPictSurface_Noop14_1008_5724(void);
void CPictSurface_BlitToSurfaceByMode_1008_5728(SBPictSurface *self,
                                                int16_t mode,
                                                const SB_RECT16 *dst_rc,
                                                const SB_RECT16 *src_rc,
                                                SBPictSurface *dst_surf);
void CPictSurface_BlitToScreenByMode_1008_5786(SBPictSurface *self,
                                               int16_t mode,
                                               const SB_RECT16 *dst_rc,
                                               const SB_RECT16 *src_rc);
void CPictSurface_CreateDc_1008_57ce(SBPictSurface *self, const SB_RECT16 *bounds);
void CPictSurface_LoadPictResource_1008_5804(SBPictSurface *self,
                                             const SB_RECT16 *dst_rc,
                                             const SB_RECT16 *src_rc,
                                             uint16_t pict_res_id);
void CPictSurface_FillBlack_1008_5a0a(SBPictSurface *self, const SB_RECT16 *rc);
void CPictSurface_ClipRects_1008_5a2a(const SB_RECT16 *dst_clip,
                                      const SB_RECT16 *src_clip,
                                      SB_RECT16 *io_dst_rc,
                                      SB_RECT16 *io_src_rc);
void CPictSurface_FillColorRect_1008_5b78(SBPictSurface *self, int16_t color_idx, const SB_RECT16 *rc);
void CPictSurface_CopyFromScreen_1008_5c68(SBPictSurface *self,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc);
void CPictSurface_CopyToSurface_1008_5c88(SBPictSurface *self,
                                          const SB_RECT16 *dst_rc,
                                          const SB_RECT16 *src_rc,
                                          SBPictSurface *dst_surf);
void CPictSurface_CopyToScreen_1008_5caa(SBPictSurface *self,
                                         const SB_RECT16 *dst_rc,
                                         const SB_RECT16 *src_rc);
void CPictSurface_ClearScreenRect_1008_5cca(const SB_RECT16 *rc);
void CPictSurface_TransparentBlitToSurface_1008_5cea(SBPictSurface *self,
                                                     uint8_t use_precomputed_mask,
                                                     const SB_RECT16 *dst_rc,
                                                     const SB_RECT16 *src_rc,
                                                     SBPictSurface *dst_surf);
void CPictSurface_TransparentBlitToScreen_1008_5d36(SBPictSurface *self,
                                                    uint8_t use_precomputed_mask,
                                                    const SB_RECT16 *dst_rc,
                                                    const SB_RECT16 *src_rc);
SB_HBITMAP CPictSurface_GetHBitmap_1008_5d7c(const SBPictSurface *self);
SB_HDC CPictSurface_GetHDc_1008_5d8a(const SBPictSurface *self);
uint8_t CPictSurface_GetPixel_1008_5d98(const SBPictSurface *self, int16_t x, int16_t y);

void CLayeredOffscreen_InitFromPict_1008_5e14(SBPictSurface *self,
                                              uint16_t planes,
                                              uint16_t bpp,
                                              uint16_t pict_res_id);
void CLayeredOffscreen_Forward18_1008_5ed4(SBPictSurface *self,
                                           int16_t mode,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc,
                                           SBPictSurface *dst_surf);
void CLayeredOffscreen_Forward1c_1008_5f02(SBPictSurface *self,
                                           int16_t mode,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc);
void CLayeredOffscreen_Forward38_1008_5f2c(SBPictSurface *self,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc,
                                           SBPictSurface *dst_surf);
void CLayeredOffscreen_Forward3c_1008_5f52(SBPictSurface *self,
                                           const SB_RECT16 *dst_rc,
                                           const SB_RECT16 *src_rc);

void CMaskedPictSurface_InitFromPict_1008_5f6e(SBPictSurface *self,
                                               uint16_t planes,
                                               uint16_t bpp,
                                               uint16_t pict_res_id);
void CMaskedPictSurface_Dtor_1008_604a(SBPictSurface *self);
void CMaskedPictSurface_Forward18_1008_60b2(SBPictSurface *self,
                                            int16_t mode,
                                            const SB_RECT16 *dst_rc,
                                            const SB_RECT16 *src_rc,
                                            SBPictSurface *dst_surf);
void CMaskedPictSurface_Forward1c_1008_60e0(SBPictSurface *self,
                                            int16_t mode,
                                            const SB_RECT16 *dst_rc,
                                            const SB_RECT16 *src_rc);
void CMaskedPictSurface_BlitToSurface_1008_610a(SBPictSurface *self,
                                                const SB_RECT16 *dst_rc,
                                                const SB_RECT16 *src_rc,
                                                SBPictSurface *dst_surf);
void CMaskedPictSurface_BlitToScreen_1008_612e(SBPictSurface *self,
                                               const SB_RECT16 *dst_rc,
                                               const SB_RECT16 *src_rc);

void CTiledPictSurface_InitFromPict_1008_614e(SBPictSurface *self,
                                              uint16_t planes,
                                              uint16_t bpp,
                                              const SB_RECT16 *full_bounds,
                                              const SB_RECT16 *tile_step,
                                              uint16_t pict_res_id);
void CTiledPictSurface_CreateTiled_1008_6246(SBPictSurface *self, const SB_RECT16 *bounds);

void CPictSurface_Noop0c_1008_6436(void);
uint8_t CPictSurface_True58_1008_643a(void);
void CPictSurface_Noop5c_1008_6440(void);
void CPictSurface_Noop60_1008_6444(void);
void WaterShimmer_AnimateBitmap_1008_64ac(SBPictSurface *surface);

#ifdef __cplusplus
}
#endif

#endif /* SB_RECOVERED_FRAMEWORK_GRAPHICS_GDI_H */
