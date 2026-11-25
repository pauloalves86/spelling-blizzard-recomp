#include "../src/recovered/framework/resource_manager.h"
#include "../src/shim/win16_resource.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

int main(void)
{
    SBResourceManager *mgr;
    SBResourceTypeList *snd_list;
    SBResourceHandle *h_rslt;
    SBResourceHandle *h_snd;
    SBResourceHandle *h_pict;
    SBResourceHandle *h_clut;
    SBResourceHandle *h_wind;
    SBResourceHandle *h_gmlt;
    SBResourceHandle *h_heap;
    const uint8_t *snd_bytes;

    /* 1. Initialize the NE resource shim against the immutable original binaries */
    assert(sb_win16_resource_shim_init("original/BLIZZARD.EXE", "original/SB_CDRES.DLL") == 0);

    /* 2. Construct and initialize the global Resource Manager (`1008:2bf4` -> `1008:65f2` -> `1010:286c`) */
    mgr = CResourceManager_CreateGlobal_1008_2bf4_part();
    assert(mgr != NULL);
    assert(g_DAT_1060_0a14_resMgr == mgr);
    assert(g_DAT_1060_001c_hCdResModule == SB_HMODULE_SB_CDRES_DLL);
    /* BLIZZARD.EXE 0x03e8 #1001 declares exactly 42 resource types */
    assert(mgr->count_04 == 42);

    /* Verify 'snd ' (`0x03ee`) collection metadata from directory 0x03e8 #1002 */
    snd_list = CResourceMap_FindTypeList_1010_2a2e(mgr, 0x03ee, "snd ");
    assert(snd_list != NULL);
    assert(snd_list->total_expected_1e == 430);

    /* 3. Test loading 'RsLt' (`0x03e9`) #501 via `1010:10f2` (as `1018:9140` does at startup) */
    h_rslt = ResMgr_GetResource_1010_10f2(0x03e9, "RsLt", 501);
    assert(h_rslt != NULL);
    assert(h_rslt->data_00 != NULL);
    assert(Handle_GetSize_1010_0076(h_rslt) == 512);

    /* 4. Test loading 'snd ' (`0x03ee`) #129 BSW audio stream (as `1008:22d6` does) */
    h_snd = ResMgr_GetResource_1010_10f2(0x03ee, "snd ", 129);
    assert(h_snd != NULL);
    assert(h_snd->data_00 != NULL);
    assert(Handle_GetSize_1010_0076(h_snd) == 304896);
    snd_bytes = (const uint8_t *)h_snd->data_00;
    assert(memcmp(snd_bytes, "BSW\0", 4) == 0);
    assert(read_u16_le(snd_bytes + 8) == 11025);

    /* Fetching the same resource again returns the cached handle */
    assert(ResMgr_GetResource_1010_10f2(0x03ee, "snd ", 129) == h_snd);

    /* 5. Test loading 'WIND' (`0x03f0`) #1 from BLIZZARD.EXE (`1008:2bdc` module routing) */
    h_wind = ResMgr_GetResource_1010_10f2(0x03f0, "WIND", 1);
    assert(h_wind != NULL);
    assert(h_wind->data_00 != NULL);
    assert(Handle_GetSize_1010_0076(h_wind) == 32);

    /* 6. Test loading 'clut' (`0x03ef`) #999 palette (as `1008:36b2` does) */
    h_clut = ResMgr_GetResource_1010_10f2(0x03ef, "clut", 999);
    assert(h_clut != NULL);
    assert(h_clut->data_00 != NULL);
    assert(Handle_GetSize_1010_0076(h_clut) == 2304);

    /* 7. Test loading 'GmLt' (`0x0407`) #1001 spelling level word list (as `1020:1292` does) */
    h_gmlt = ResMgr_GetResource_1010_10f2(0x0407, "GmLt", 1001);
    assert(h_gmlt != NULL);
    assert(h_gmlt->data_00 != NULL);
    assert(Handle_GetSize_1010_0076(h_gmlt) == 1792);

    /* 8. Test 'PICT' (`0x03f3`) #128 special immediate-purge behavior in `1008:6602` */
    h_pict = ResMgr_GetResource_1010_10f2(0x03f3, "PICT", 128);
    assert(h_pict != NULL);
    assert(h_pict->data_00 != NULL);
    assert(Handle_GetSize_1010_0076(h_pict) == 39936);

    /* Lock the PICT handle (`1010:00e8`), then unlock it (`1010:0122`).
     * Because `h_pict` is purgeable (`0x20`) and type `0x03f3`, unlocking it triggers
     * `CResourceManager_UpdateEntryFlags_1008_6602` to immediately purge it!
     */
    Handle_Lock_1010_00e8(h_pict);
    assert((Handle_GetFlags_1010_0096(h_pict) & SB_HANDLE_FLAG_LOCKED) != 0);
    Handle_Unlock_1010_0122(h_pict);
    assert((Handle_GetFlags_1010_0096(h_pict) & SB_HANDLE_FLAG_PURGED) != 0);
    assert(h_pict->data_00 == NULL);

    /* Calling `ResHandle_EnsureLoaded_1010_11d8` or `ResMgr_GetResource_1010_10f2` reloads it seamlessly */
    ResHandle_EnsureLoaded_1010_11d8(h_pict);
    assert((Handle_GetFlags_1010_0096(h_pict) & SB_HANDLE_FLAG_PURGED) == 0);
    assert(h_pict->data_00 != NULL);

    /* 9. Test `CResourceList_1008_821c` batch preloader (`1018:9140..9480`) with RsLt #501 (0x1f5)
     * Exact startup sequence from `1008:72b6`: allocates 8-byte `CResourceList_1008_821c`,
     * calls `1018:9140(loader, 0x1f5)`, and invokes slot `+0x14` (`1018:92e6`) to load & lock all 43 assets!
     */
    {
        SBResourceBatchLoader *batch = (SBResourceBatchLoader *)NewObject_1008_ef2c(sizeof(SBResourceBatchLoader));
        SBResourceTypeList *rlsy_list;
        assert(batch != NULL);
        CResourceList_Init_1018_9140(batch, 501);
        assert(batch->rslt_04 == h_rslt);
        CResourceList_LoadAndLockAll_1018_92e6(batch);

        /* RsLt #501 loads 23 `RlSy` (`0x03ec`) DIB bitmaps among its 43 entries */
        rlsy_list = CResourceMap_FindTypeList_1010_2a2e(mgr, 0x03ec, "RlSy");
        assert(rlsy_list != NULL);
        assert(rlsy_list->count_04 == 23);

        CResourceList_UnlockAll_1018_93dc(batch);
        CResourceList_MarkAllPurgeable_1018_9242(batch);
        CResourceList_Dtor_1018_9176(batch);
    }

    /* 10. Test heap handle allocation, resize, and disposal (`1010:023e`, `1010:035c`, `1010:03ee`, `1010:057c`) */
    h_heap = Handle_AllocZeroed_1010_035c(0x1aa);
    assert(h_heap != NULL);
    assert(h_heap->data_00 != NULL);
    assert(Handle_GetSize_1010_0076(h_heap) == 0x1aa);
    Handle_Resize_1010_03ee(h_heap, 0x300);
    assert(Handle_GetSize_1010_0076(h_heap) == 0x300);
    Handle_FreeIndirect_1010_057c(&h_heap);
    assert(h_heap == NULL);

    /* 10. Clean shutdown via `CResourceMap_Dtor_1010_29ea` */
    CResourceMap_Dtor_1010_29ea(mgr);
    assert(g_DAT_1060_0a14_resMgr == NULL);
    assert(g_DAT_1060_001c_hCdResModule == 0);
    sb_win16_resource_shim_shutdown();

    printf("test_resource_manager: all 42 resource types and real SB_CDRES.DLL asset loads verified!\n");
    return 0;
}
