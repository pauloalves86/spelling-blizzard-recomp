#include "../src/recovered/framework/graphics_gdi.h"
#include "../src/recovered/framework/resource_manager.h"
#include "../src/shim/win16_gdi.h"
#include "../src/shim/win16_resource.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    SBResourceManager *res_mgr;
    SBPaletteManager *pal_mgr;
    SBPictSurface *bg_surf;
    SBPictSurface *sprite_surf;
    SB_RECT16 rc1, rc2, rc_inter;
    int16_t bmp_w = 0, bmp_h = 0;
    const uint8_t *pixels;
    int non_zero_pixels = 0;
    int i;

    /* 1. Initialize Win16 Resource and GDI Shims */
    assert(sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL") == 0);
    sb_win16_gdi_shim_reset();

    /* 2. Test Geometry & Rectangle helpers (`1010:0f1c..0fd0`) */
    Rect_Set_1010_0fb6(&rc1, 0, 0, 640, 480);
    assert(Rect_IsEmpty_1010_0f1c(&rc1) == 0);
    Rect_Set_1010_0fb6(&rc2, 100, 100, 300, 250);
    assert(Rect_Intersect_1010_0f88(&rc_inter, &rc1, &rc2) == 1);
    assert(Rect_Equal_1010_0f2a(&rc_inter, &rc2) == 1);
    Rect_Inset_1010_0f3c(&rc_inter, 10, 20);
    assert(rc_inter.left == 110 && rc_inter.top == 120 && rc_inter.right == 290 && rc_inter.bottom == 230);

    /* 3. Initialize Resource Manager & Palette Manager (`1008:326e`, `1008:36b2`) */
    res_mgr = CResourceManager_CreateGlobal_1008_2bf4_part();
    assert(res_mgr != NULL);

    pal_mgr = (SBPaletteManager *)NewObject_1008_ef2c(sizeof(SBPaletteManager));
    assert(pal_mgr != NULL);
    CPaletteManager_Init_1008_326e(pal_mgr);
    assert(g_DAT_1060_0198_paletteMgr == pal_mgr);
    assert(CPaletteManager_GetHPalette_1008_7cb4(pal_mgr) != 0);

    /* Load real `'clut'` #1049 from `SB_CDRES.DLL` (which pairs with companion `'clut'` #999) */
    CPaletteManager_LoadClutResource_1008_36b2(pal_mgr, 1049);
    {
        int non_black_colors = 0;
        for (i = 0; i < 236; ++i) {
            if (pal_mgr->current_3bc[i].peRed != 0 ||
                pal_mgr->current_3bc[i].peGreen != 0 ||
                pal_mgr->current_3bc[i].peBlue != 0) {
                non_black_colors++;
            }
        }
        assert(non_black_colors > 200);
    }

    /* Test palette save/restore (`1008:382e`, `1008:3950`) and cycling (`1008:3b44`) */
    CPaletteManager_SaveAndRealize_1008_382e(pal_mgr);
    CPaletteManager_CycleRange_1008_3b44(pal_mgr, 10, 20);
    CPaletteManager_RestoreAndRealize_1008_3950(pal_mgr);

    /* 4. Decode real PackBits RLE `'PICT'` (`0x03f3`) #129 (640x480 full screen background)
     * via `CLayeredOffscreen_InitFromPict_1008_5e14` -> `CPictSurface_LoadPictResource_1008_5804` -> `1010:08e0`
     */
    bg_surf = (SBPictSurface *)NewObject_1008_ef2c(sizeof(SBPictSurface));
    assert(bg_surf != NULL);
    CLayeredOffscreen_InitFromPict_1008_5e14(bg_surf, 1, 8, 129);
    assert(bg_surf->bounds_0e.right == 640);
    assert(bg_surf->bounds_0e.bottom == 480);
    assert(CPictSurface_GetHBitmap_1008_5d7c(bg_surf) != 0);

    pixels = sb_win16_gdi_getBitmapPixels(bg_surf->hbmp_06, &bmp_w, &bmp_h);
    assert(pixels != NULL);
    assert(bmp_w == 640 && bmp_h == 480);
    for (i = 0; i < 640 * 480; ++i) {
        if (pixels[i] != 0) {
            non_zero_pixels++;
        }
    }
    assert(non_zero_pixels > 200000);

    /* 5. Test `CMaskedPictSurface_1008_b384` (`1008:5f6e`) with `'PICT'` #1049 (356x16 UI banner)
     * and transparent mask blitting (`1008:610a` -> `1010:2f6a`) onto `bg_surf`
     */
    sprite_surf = (SBPictSurface *)NewObject_1008_ef2c(sizeof(SBPictSurface));
    assert(sprite_surf != NULL);
    CMaskedPictSurface_InitFromPict_1008_5f6e(sprite_surf, 1, 8, 1049);
    assert(sprite_surf->bounds_0e.right == 356);
    assert(sprite_surf->bounds_0e.bottom == 16);
    assert(sprite_surf->mask_18 != NULL);
    assert(sprite_surf->mask_18->hbmp_mask_0a != 0);

    {
        SB_RECT16 src_rc = { 0, 0, 356, 16 };
        SB_RECT16 dst_rc = { 50, 50, 406, 66 };
        CMaskedPictSurface_BlitToSurface_1008_610a(sprite_surf, &dst_rc, &src_rc, bg_surf);
    }

    /* 6. Test procedural water/ice shimmer animation (`1008:64ac`) */
    {
        SB_RECT16 water_rc = { 0, 0, 64, 64 };
        /* Fill a 64x64 patch with color index 160 (160 + 9 = 169 = 0xA9, in the 0xA8..0xAF water range) */
        CPictSurface_FillColorRect_1008_5b78(bg_surf, 160, &water_rc);
        assert(CPictSurface_GetPixel_1008_5d98(bg_surf, 10, 10) == 0xA9);
        WaterShimmer_AnimateBitmap_1008_64ac(bg_surf);
        {
            uint8_t p = CPictSurface_GetPixel_1008_5d98(bg_surf, 10, 10);
            assert(p >= 0xA8 && p <= 0xAF);
        }
    }

    /* Cleanup */
    CMaskedPictSurface_Dtor_1008_604a(sprite_surf);
    CPictSurface_Dtor_1008_56a8(bg_surf);
    CPaletteManager_Dtor_1008_3464(pal_mgr);
    CResourceMap_Dtor_1010_29ea(res_mgr);
    sb_win16_gdi_shim_reset();
    sb_win16_resource_shim_shutdown();

    printf("test_graphics_gdi: clut #1049/#999 palette, PackBits RLE PICT #129 (640x480) & #1049 (356x16), transparent mask blitting, and water shimmer verified!\n");
    return 0;
}
