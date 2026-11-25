#include "resource_manager.h"

#include <stdlib.h>
#include <string.h>

SB_HMODULE g_DAT_1060_07d0_hInstance = SB_HMODULE_BLIZZARD_EXE;
SB_HMODULE g_DAT_1060_001c_hCdResModule = 0;
SBResourceManager *g_DAT_1060_0a14_resMgr = NULL;
uint8_t g_DAT_1060_07f5_preloadFlag = 0;

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* ==========================================================================
 * 1. Base Object & Allocators (`1008:eed8..ef9e`)
 * ========================================================================== */

/* Original: 1008:eed8 */
void *AllocZeroed_1008_eed8(size_t size)
{
    if (size == 0) {
        size = 1;
    }
    return calloc(1, size);
}

/* Original: 1008:ef1e */
void FreeMem_1008_ef1e(void *ptr)
{
    free(ptr);
}

/* Original: 1008:ef2c */
void *NewObject_1008_ef2c(size_t size)
{
    if (size == 0) {
        size = 1;
    }
    return calloc(1, size);
}

/* Original: 1008:ef80 */
void DeleteObject_1008_ef80(void *ptr)
{
    free(ptr);
}

/* Original: 1008:ef8e */
void CObject_Dtor_1008_ef8e(void *obj)
{
    free(obj);
}

/* Original: 1008:ef9e */
void CObject_Noop_1008_ef9e(void)
{
}

/* ==========================================================================
 * 2. Handle Memory & Resource Flags (`1010:0076..0608`)
 * ========================================================================== */

/* Original: 1010:0076 */
uint32_t Handle_GetSize_1010_0076(const SBResourceHandle *h)
{
    if (!h) {
        return 0;
    }
    return h->size_08;
}

/* Original: 1010:0096 */
uint8_t Handle_GetFlags_1010_0096(const SBResourceHandle *h)
{
    if (!h) {
        return 0;
    }
    return h->flags_0c;
}

/* Original: 1010:00b0 */
void Handle_SetFlags_1010_00b0(SBResourceHandle *h, uint8_t flags)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 && g_DAT_1060_0a14_resMgr != NULL) {
        CResourceManager_UpdateEntryFlags_1008_6602(g_DAT_1060_0a14_resMgr, flags, h);
        return;
    }
    h->flags_0c = flags;
}

/* Original: 1010:00e8 */
void Handle_Lock_1010_00e8(SBResourceHandle *h)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 && g_DAT_1060_0a14_resMgr != NULL) {
        CResourceManager_UpdateEntryFlags_1008_6602(
            g_DAT_1060_0a14_resMgr,
            (uint8_t)(h->flags_0c | SB_HANDLE_FLAG_LOCKED),
            h);
        return;
    }
    h->flags_0c |= SB_HANDLE_FLAG_LOCKED;
}

/* Original: 1010:0122 */
void Handle_Unlock_1010_0122(SBResourceHandle *h)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 && g_DAT_1060_0a14_resMgr != NULL) {
        CResourceManager_UpdateEntryFlags_1008_6602(
            g_DAT_1060_0a14_resMgr,
            (uint8_t)(h->flags_0c & ~SB_HANDLE_FLAG_LOCKED),
            h);
        return;
    }
    h->flags_0c &= (uint8_t)~SB_HANDLE_FLAG_LOCKED;
}

/* Original: 1010:015c */
void Handle_NoPurge_1010_015c(SBResourceHandle *h)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 && g_DAT_1060_0a14_resMgr != NULL) {
        CResourceManager_UpdateEntryFlags_1008_6602(
            g_DAT_1060_0a14_resMgr,
            (uint8_t)(h->flags_0c & ~SB_HANDLE_FLAG_PURGEABLE),
            h);
        return;
    }
    h->flags_0c &= (uint8_t)~SB_HANDLE_FLAG_PURGEABLE;
}

/* Original: 1010:0196 */
void Handle_AllowPurge_1010_0196(SBResourceHandle *h)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 && g_DAT_1060_0a14_resMgr != NULL) {
        CResourceManager_UpdateEntryFlags_1008_6602(
            g_DAT_1060_0a14_resMgr,
            (uint8_t)(h->flags_0c | SB_HANDLE_FLAG_PURGEABLE),
            h);
        return;
    }
    h->flags_0c |= SB_HANDLE_FLAG_PURGEABLE;
}

/* Original: 1010:01d0 */
void Handle_Dispose_1010_01d0(SBResourceHandle *h)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) == 0 && h->data_00 != NULL) {
        free((void *)h->data_00);
    }
    memset(h, 0, sizeof(*h));
    FreeMem_1008_ef1e(h);
}

/* Original: 1010:023e */
SBResourceHandle *Handle_Alloc_1010_023e(uint32_t size, int zero_init)
{
    uint32_t alloc_size = (size == 0) ? 1u : size;
    void *buf = zero_init ? calloc(1, alloc_size) : malloc(alloc_size);
    SBResourceHandle *h;

    if (!buf) {
        return NULL;
    }
    h = (SBResourceHandle *)AllocZeroed_1008_eed8(sizeof(SBResourceHandle));
    if (!h) {
        free(buf);
        return NULL;
    }
    h->data_00 = buf;
    h->self_04 = h;
    h->size_08 = size;
    h->flags_0c = SB_HANDLE_FLAG_HEAP_OR_LOCKED;
    return h;
}

/* Original: 1010:035c */
SBResourceHandle *Handle_AllocZeroed_1010_035c(uint32_t size)
{
    return Handle_Alloc_1010_023e(size, 1);
}

/* Original: 1010:03ee */
void Handle_Resize_1010_03ee(SBResourceHandle *h, uint32_t new_size)
{
    uint32_t alloc_size;
    void *new_buf;

    if (!h || h->size_08 == new_size) {
        return;
    }
    alloc_size = (new_size == 0) ? 1u : new_size;
    new_buf = realloc((void *)h->data_00, alloc_size);
    if (!new_buf) {
        return;
    }
    h->data_00 = new_buf;
    h->size_08 = new_size;
}

/* Original: 1010:057c */
void Handle_FreeIndirect_1010_057c(SBResourceHandle **h_ptr)
{
    if (h_ptr && *h_ptr) {
        Handle_Dispose_1010_01d0(*h_ptr);
        *h_ptr = NULL;
    }
}

/* Original: 1010:0608 (RETF no-op on Win16; Macintosh MoveHHi equivalent) */
void Handle_MoveNeck_1010_0608(SBResourceHandle *h)
{
    (void)h;
}

/* ==========================================================================
 * 3. Resource Loading Pipeline (`1008:2bbc..2bdc`, `1010:0fe6..1206`)
 * ========================================================================== */

/* Original: 1008:2bbc */
void ResMgr_LoadCdResLibrary_1008_2bbc(void)
{
    g_DAT_1060_001c_hCdResModule = sb_win16_LoadLibrary("SB_CDRES.DLL");
}

/* Original: 1008:2bcc */
void ResMgr_FreeCdResLibrary_1008_2bcc(void)
{
    sb_win16_FreeLibrary(g_DAT_1060_001c_hCdResModule);
    g_DAT_1060_001c_hCdResModule = 0;
}

/* Original: 1008:2bdc
 * Note: 'WIND' resources live inside BLIZZARD.EXE (`DAT_1060_07d0`);
 * all other resource types live inside SB_CDRES.DLL (`DAT_1060_001c`).
 */
SB_HMODULE ResMgr_GetResourceModule_1008_2bdc(uint16_t win16_type_id, const char mac_fourcc[4])
{
    (void)win16_type_id;
    if (mac_fourcc != NULL && memcmp(mac_fourcc, "WIND", 4) == 0) {
        return g_DAT_1060_07d0_hInstance;
    }
    return g_DAT_1060_001c_hCdResModule;
}

/* Original: 1010:0fe6 */
uint8_t ResHandle_LoadAndLock_1010_0fe6(SBResourceHandle *h)
{
    SB_HMODULE hModule;

    if (!h) {
        return 1;
    }
    hModule = ResMgr_GetResourceModule_1008_2bdc(h->type_14.win16_type_id, h->type_14.mac_fourcc);

    if (h->hRsrc_10 == 0) {
        h->hRsrc_10 = sb_win16_FindResource(hModule, h->res_id_12, h->type_14.win16_type_id);
        if (h->hRsrc_10 == 0) {
            return 1;
        }
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_RES_LOADED) == 0) {
        h->hGlobal_0e = sb_win16_LoadResource(hModule, h->hRsrc_10);
        if (h->hGlobal_0e == 0) {
            return 1;
        }
        h->flags_0c |= SB_HANDLE_FLAG_RES_LOADED;
        h->size_08 = sb_win16_SizeofResource(hModule, h->hRsrc_10);
        h->self_04 = h;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_HEAP_OR_LOCKED) == 0) {
        h->data_00 = sb_win16_LockResource(h->hGlobal_0e);
        if (h->data_00 == NULL) {
            h->flags_0c |= SB_HANDLE_FLAG_LOCKING_IN_PROGRESS;
            if (g_DAT_1060_0a14_resMgr != NULL) {
                CResourceMap_PurgeAllTypes_1010_2a54(g_DAT_1060_0a14_resMgr);
            }
            h->flags_0c &= (uint8_t)~SB_HANDLE_FLAG_LOCKING_IN_PROGRESS;
            h->data_00 = sb_win16_LockResource(h->hGlobal_0e);
            if (h->data_00 == NULL) {
                sb_win16_FreeResource(h->hGlobal_0e);
                return 1;
            }
        }
        h->flags_0c |= SB_HANDLE_FLAG_RES_LOADED;
    }
    return 0;
}

/* Original: 1010:10f2 */
SBResourceHandle *ResMgr_GetResource_1010_10f2(uint16_t win16_type_id,
                                               const char mac_fourcc[4],
                                               uint16_t res_id)
{
    SBResourceTypeList *type_list;
    SBResourceHandle *h;

    if (res_id == 0 || win16_type_id == 0 || g_DAT_1060_0a14_resMgr == NULL) {
        return NULL;
    }
    type_list = CResourceMap_FindTypeList_1010_2a2e(g_DAT_1060_0a14_resMgr, win16_type_id, mac_fourcc);
    if (!type_list) {
        return NULL;
    }
    h = CResourceTypeList_FindOrCreateById_1010_25e4(type_list, res_id);
    if (!h) {
        return NULL;
    }
    if (h->res_id_12 == res_id) {
        if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 &&
            (h->flags_0c & SB_HANDLE_FLAG_PURGED) != 0) {
            if (ResHandle_LoadAndLock_1010_0fe6(h) == 0) {
                h->flags_0c &= (uint8_t)~SB_HANDLE_FLAG_PURGED;
            }
        }
        return h->self_04;
    }

    h->type_14.win16_type_id = win16_type_id;
    memcpy(h->type_14.mac_fourcc, mac_fourcc, 4);
    h->res_id_12 = res_id;
    h->flags_0c = (uint8_t)(SB_HANDLE_FLAG_IS_RESOURCE | SB_HANDLE_FLAG_PURGEABLE);

    if (ResHandle_LoadAndLock_1010_0fe6(h) == 0) {
        return h->self_04;
    }
    memset(h, 0, sizeof(*h));
    return NULL;
}

/* Original: 1010:11d8 */
void ResHandle_EnsureLoaded_1010_11d8(SBResourceHandle *h)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 &&
        (h->flags_0c & SB_HANDLE_FLAG_PURGED) != 0) {
        if (ResHandle_LoadAndLock_1010_0fe6(h) == 0) {
            h->flags_0c &= (uint8_t)~SB_HANDLE_FLAG_PURGED;
        }
    }
}

/* Original: 1010:1206 */
const void *ResHandle_PurgeAndDeref_1010_1206(SBResourceHandle *h)
{
    if (g_DAT_1060_0a14_resMgr != NULL) {
        CResourceMap_PurgeAllTypes_1010_2a54(g_DAT_1060_0a14_resMgr);
    }
    return h ? h->data_00 : NULL;
}

/* ==========================================================================
 * 4. Resource Type Collection (`CResourceTypeList_1010_2b12`)
 * ========================================================================== */

SBResourceTypeList *CResourceTypeList_New(const SBResourceTypeDesc *desc, uint16_t expected_count)
{
    SBResourceTypeList *list = (SBResourceTypeList *)NewObject_1008_ef2c(sizeof(SBResourceTypeList));
    int32_t init_cap = 16;

    if (!list) {
        return NULL;
    }
    list->vtable_tag_00 = 0x10102b12u;
    list->count_04 = 0;
    list->grow_step_08 = (expected_count >> 4) > 3 ? (int16_t)(expected_count >> 4) : 3;
    list->capacity_0a = init_cap;
    list->items_0e = (SBResourceIdMapEntry *)calloc((size_t)init_cap, sizeof(SBResourceIdMapEntry));
    list->elem_size_12 = 6;
    if (desc) {
        list->type_18 = *desc;
    }
    list->total_expected_1e = expected_count;
    return list;
}

/* Original: 1010:25be */
void CResourceTypeList_Dtor_1010_25be(SBResourceTypeList *self)
{
    if (!self) {
        return;
    }
    CResourceTypeList_ReleaseAll_1010_2810(self);
    free(self->items_0e);
    self->items_0e = NULL;
    CObject_Dtor_1008_ef8e(self);
}

/* Original: 1010:2a94 */
int ResEntry_CompareId_1010_2a94(const uint16_t *target_id, const SBResourceIdMapEntry *entry)
{
    return (int)(*target_id) - (int)(entry->res_id_00);
}

/* Original: 1010:25e4 */
SBResourceHandle *CResourceTypeList_FindOrCreateById_1010_25e4(SBResourceTypeList *self, uint16_t res_id)
{
    int32_t i;

    if (!self) {
        return NULL;
    }
    /* Slot +0x30 (`1008:bcbe`) linear search with `1010:2a94` comparator */
    for (i = 0; i < self->count_04; ++i) {
        if (ResEntry_CompareId_1010_2a94(&res_id, &self->items_0e[i]) == 0) {
            return self->items_0e[i].handle_02;
        }
    }
    /* Not found: call slot +0x5c (`1010:266c`) to allocate and insert at front */
    return CResourceTypeList_CreateEntry_1010_266c(self, res_id);
}

/* Original: 1010:264c */
void CResourceTypeList_GetTypeDescriptor_1010_264c(const SBResourceTypeList *self, SBResourceTypeDesc *out_desc)
{
    if (self && out_desc) {
        *out_desc = self->type_18;
    }
}

/* Original: 1010:266c */
SBResourceHandle *CResourceTypeList_CreateEntry_1010_266c(SBResourceTypeList *self, uint16_t res_id)
{
    SBResourceHandle *h;

    if (!self) {
        return NULL;
    }
    h = (SBResourceHandle *)AllocZeroed_1008_eed8(sizeof(SBResourceHandle));
    if (!h) {
        return NULL;
    }
    if (self->count_04 >= self->capacity_0a) {
        int32_t new_cap = self->capacity_0a + (self->grow_step_08 > 0 ? self->grow_step_08 : 8);
        SBResourceIdMapEntry *new_items = (SBResourceIdMapEntry *)realloc(
            self->items_0e, (size_t)new_cap * sizeof(SBResourceIdMapEntry));
        if (!new_items) {
            FreeMem_1008_ef1e(h);
            return NULL;
        }
        self->items_0e = new_items;
        self->capacity_0a = new_cap;
    }
    /* Slot +0x18 (`1008:b9ee`) inserts at 1-based index 1 (front of array) */
    if (self->count_04 > 0) {
        memmove(&self->items_0e[1],
                &self->items_0e[0],
                (size_t)self->count_04 * sizeof(SBResourceIdMapEntry));
    }
    self->items_0e[0].res_id_00 = res_id;
    self->items_0e[0].handle_02 = h;
    self->count_04++;
    return h;
}

/* Original: 1010:26ba */
void CResourceTypeList_PurgeEntry_1010_26ba(SBResourceTypeList *self, SBResourceHandle *h)
{
    (void)self;
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_PURGEABLE) != 0 &&
        (h->flags_0c & (SB_HANDLE_FLAG_LOCKED | SB_HANDLE_FLAG_LOCKING_IN_PROGRESS)) == 0) {
        if ((h->flags_0c & SB_HANDLE_FLAG_HEAP_OR_LOCKED) != 0) {
            sb_win16_GlobalUnlock(h->hGlobal_0e);
        }
        if ((h->flags_0c & SB_HANDLE_FLAG_RES_LOADED) != 0) {
            sb_win16_FreeResource(h->hGlobal_0e);
        }
        h->hGlobal_0e = 0;
        h->data_00 = NULL;
        h->flags_0c |= SB_HANDLE_FLAG_PURGED;
        h->flags_0c &= (uint8_t)~(SB_HANDLE_FLAG_HEAP_OR_LOCKED | SB_HANDLE_FLAG_RES_LOADED);
    }
}

/* Original: 1010:2712 */
void CResourceTypeList_PurgeAll_1010_2712(SBResourceTypeList *self)
{
    int32_t i;
    if (!self) {
        return;
    }
    for (i = 0; i < self->count_04; ++i) {
        CResourceTypeList_PurgeEntry_1010_26ba(self, self->items_0e[i].handle_02);
    }
}

/* Original: 1010:2784 */
void CResourceTypeList_ReleaseEntry_1010_2784(SBResourceTypeList *self, int32_t index_1based, SBResourceHandle *h)
{
    int32_t idx0;

    if (!self) {
        return;
    }
    if (index_1based == -1) {
        int32_t i;
        for (i = 0; i < self->count_04; ++i) {
            if (self->items_0e[i].handle_02 == h) {
                index_1based = i + 1;
                break;
            }
        }
    }
    if (h != NULL) {
        if ((h->flags_0c & SB_HANDLE_FLAG_HEAP_OR_LOCKED) != 0) {
            sb_win16_GlobalUnlock(h->hGlobal_0e);
        }
        if ((h->flags_0c & SB_HANDLE_FLAG_RES_LOADED) != 0) {
            sb_win16_FreeResource(h->hGlobal_0e);
        }
        h->hGlobal_0e = 0;
        h->data_00 = NULL;
        FreeMem_1008_ef1e(h);
    }
    idx0 = index_1based - 1;
    if (idx0 >= 0 && idx0 < self->count_04) {
        if (idx0 + 1 < self->count_04) {
            memmove(&self->items_0e[idx0],
                    &self->items_0e[idx0 + 1],
                    (size_t)(self->count_04 - idx0 - 1) * sizeof(SBResourceIdMapEntry));
        }
        self->count_04--;
    }
}

/* Original: 1010:2810 */
void CResourceTypeList_ReleaseAll_1010_2810(SBResourceTypeList *self)
{
    int32_t i;
    if (!self) {
        return;
    }
    for (i = self->count_04; i > 0; --i) {
        CResourceTypeList_ReleaseEntry_1010_2784(self, i, self->items_0e[i - 1].handle_02);
    }
}

/* ==========================================================================
 * 5. Global Resource Manager (`CResourceMap_1008_2ece` / `CResourceManager_1008_2da6`)
 * ========================================================================== */

SBResourceManager *CResourceManager_CreateGlobal_1008_2bf4_part(void)
{
    SBResourceManager *mgr = (SBResourceManager *)NewObject_1008_ef2c(sizeof(SBResourceManager));
    if (!mgr) {
        return NULL;
    }
    mgr->vtable_tag_00 = 0x10082da6u;
    g_DAT_1060_0a14_resMgr = mgr;
    CResourceManager_Init_1008_65f2(mgr);
    return mgr;
}

/* Original: 1008:65f2 */
void CResourceManager_Init_1008_65f2(SBResourceManager *self)
{
    CResourceMap_InitFromDirectory_1010_286c(self);
}

/* Original: 1010:286c
 * Loads resource 0x03e8 #1001 (16-bit type count = 42) and #1002 (42 x 8-byte
 * type directory records) from BLIZZARD.EXE (`DAT_1060_07d0`), opens
 * `SB_CDRES.DLL` (`1008:2bbc`), and populates 42 `CResourceTypeList_1010_2b12`
 * collections.
 */
void CResourceMap_InitFromDirectory_1010_286c(SBResourceManager *self)
{
    SB_HRSRC hRsrcCount;
    SB_HGLOBAL hGlobCount;
    const uint8_t *pCountBytes;
    int16_t type_count;
    SB_HRSRC hRsrcDir;
    SB_HGLOBAL hGlobDir;
    const uint8_t *pDirBytes;
    int16_t i;

    if (!self) {
        return;
    }
    self->count_04 = 0;
    self->grow_step_08 = 3;
    self->capacity_0a = 64;
    self->types_0e = (SBResourceTypeList **)calloc((size_t)self->capacity_0a, sizeof(SBResourceTypeList *));

    ResMgr_LoadCdResLibrary_1008_2bbc();

    /* Note: in 1010:286c, FindResource is called with type=1000 (0x03e8) and id=1001 (0x03e9),
     * then type=1000 (0x03e8) and id=1002 (0x03ea).
     */
    hRsrcCount = sb_win16_FindResource(g_DAT_1060_07d0_hInstance, 1001, 0x03e8);
    hGlobCount = sb_win16_LoadResource(g_DAT_1060_07d0_hInstance, hRsrcCount);
    pCountBytes = (const uint8_t *)sb_win16_LockResource(hGlobCount);
    type_count = pCountBytes ? (int16_t)read_u16_le(pCountBytes) : 0;
    sb_win16_GlobalUnlock(hGlobCount);
    sb_win16_FreeResource(hGlobCount);

    hRsrcDir = sb_win16_FindResource(g_DAT_1060_07d0_hInstance, 1002, 0x03e8);
    hGlobDir = sb_win16_LoadResource(g_DAT_1060_07d0_hInstance, hRsrcDir);
    pDirBytes = (const uint8_t *)sb_win16_LockResource(hGlobDir);

    if (pDirBytes != NULL && type_count > 0) {
        for (i = 0; i < type_count; ++i) {
            const uint8_t *rec = pDirBytes + (i * 8);
            SBResourceTypeDesc desc;
            uint16_t res_count;
            SBResourceTypeList *type_list;

            /* Record layout in 0x03e8 #1002:
             * +0x00..+0x03: 4-char Mac FourCC (e.g. "RsLt", "snd ", "PICT")
             * +0x04..+0x05: uint16_t Win16 resource type ID (e.g. 0x03e9, 0x03ee)
             * +0x06..+0x07: uint16_t total resources of this type
             */
            memcpy(desc.mac_fourcc, rec, 4);
            desc.win16_type_id = read_u16_le(rec + 4);
            res_count = read_u16_le(rec + 6);

            type_list = CResourceTypeList_New(&desc, res_count);
            if (type_list != NULL && self->count_04 < self->capacity_0a) {
                self->types_0e[self->count_04++] = type_list;
            }
        }
    }

    sb_win16_GlobalUnlock(hGlobDir);
    sb_win16_FreeResource(hGlobDir);
}

/* Original: 1010:29ea */
void CResourceMap_Dtor_1010_29ea(SBResourceManager *self)
{
    int32_t i;
    if (!self) {
        return;
    }
    for (i = 0; i < self->count_04; ++i) {
        if (self->types_0e[i] != NULL) {
            CResourceTypeList_Dtor_1010_25be(self->types_0e[i]);
            self->types_0e[i] = NULL;
        }
    }
    self->count_04 = 0;
    free(self->types_0e);
    self->types_0e = NULL;
    ResMgr_FreeCdResLibrary_1008_2bcc();
    if (g_DAT_1060_0a14_resMgr == self) {
        g_DAT_1060_0a14_resMgr = NULL;
    }
    CObject_Dtor_1008_ef8e(self);
}

/* Original: 1010:2a14 */
void CResourceMap_UpdateEntryFlags_1010_2a14(SBResourceManager *self, uint8_t new_flags, SBResourceHandle *h)
{
    (void)self;
    if (!h) {
        return;
    }
    /* Updates only bits 0x60 (SB_HANDLE_FLAG_PURGEABLE | SB_HANDLE_FLAG_LOCKED) */
    h->flags_0c = (uint8_t)(h->flags_0c ^ ((h->flags_0c ^ new_flags) & 0x60u));
}

/* Original: 1008:6602
 * Override of slot +0x7c in `CResourceManager_1008_2da6`:
 * When a `PICT` (`0x03f3`) resource handle is unlocked and marked purgeable
 * (`(new_flags & 0x40) == 0 && (new_flags & 0x20) != 0`), it immediately purges
 * the entry from the `PICT` resource list before updating the flags!
 */
void CResourceManager_UpdateEntryFlags_1008_6602(SBResourceManager *self, uint8_t new_flags, SBResourceHandle *h)
{
    if (!h) {
        return;
    }
    if ((h->flags_0c & SB_HANDLE_FLAG_IS_RESOURCE) != 0 &&
        h->type_14.win16_type_id == 0x03f3u &&
        (new_flags & SB_HANDLE_FLAG_LOCKED) == 0 &&
        (new_flags & SB_HANDLE_FLAG_PURGEABLE) != 0 &&
        self != NULL) {
        SBResourceTypeList *type_list = CResourceMap_FindTypeList_1010_2a2e(
            self, h->type_14.win16_type_id, h->type_14.mac_fourcc);
        if (type_list != NULL) {
            /* Update purgeable bit first so PurgeEntry sees it is purgeable & unlocked */
            CResourceMap_UpdateEntryFlags_1010_2a14(self, new_flags, h);
            CResourceTypeList_PurgeEntry_1010_26ba(type_list, h);
            return;
        }
    }
    CResourceMap_UpdateEntryFlags_1010_2a14(self, new_flags, h);
}

/* Original: 1010:2aa6 */
int ResTypeList_MatchDescriptor_1010_2aa6(const SBResourceTypeList *list, const SBResourceTypeDesc *target)
{
    SBResourceTypeDesc list_desc;
    if (!list || !target) {
        return 0;
    }
    CResourceTypeList_GetTypeDescriptor_1010_264c(list, &list_desc);
    return (list_desc.win16_type_id == target->win16_type_id &&
            memcmp(list_desc.mac_fourcc, target->mac_fourcc, 4) == 0)
               ? 1
               : 0;
}

/* Original: 1010:2a2e */
SBResourceTypeList *CResourceMap_FindTypeList_1010_2a2e(SBResourceManager *self,
                                                        uint16_t win16_type_id,
                                                        const char mac_fourcc[4])
{
    SBResourceTypeDesc target;
    int32_t i;

    if (!self || !mac_fourcc) {
        return NULL;
    }
    target.win16_type_id = win16_type_id;
    memcpy(target.mac_fourcc, mac_fourcc, 4);

    for (i = 0; i < self->count_04; ++i) {
        if (ResTypeList_MatchDescriptor_1010_2aa6(self->types_0e[i], &target)) {
            return self->types_0e[i];
        }
    }
    return NULL;
}

/* Original: 1010:2a54 (and helper 1010:2ae2) */
void CResourceMap_PurgeAllTypes_1010_2a54(SBResourceManager *self)
{
    int32_t i;
    if (!self) {
        return;
    }
    for (i = 0; i < self->count_04; ++i) {
        CResourceTypeList_PurgeAll_1010_2712(self->types_0e[i]);
    }
}

/* Original: 1010:2a74 (and helper 1010:2afa) */
void CResourceMap_ReleaseAllTypes_1010_2a74(SBResourceManager *self)
{
    int32_t i;
    if (!self) {
        return;
    }
    for (i = 0; i < self->count_04; ++i) {
        CResourceTypeList_ReleaseAll_1010_2810(self->types_0e[i]);
    }
}

/* ==========================================================================
 * 6. Resource Batch Preloader (`CResourceList_1008_821c`, `1018:9140..9480`)
 * ========================================================================== */

/* Original: 1018:9140 */
void CResourceList_Init_1018_9140(SBResourceBatchLoader *self, uint16_t rslt_res_id)
{
    if (!self) {
        return;
    }
    self->vtable_tag_00 = 0x1008821cu;
    self->rslt_04 = ResMgr_GetResource_1010_10f2(0x03e9, "RsLt", rslt_res_id);
    Handle_NoPurge_1010_015c(self->rslt_04);
}

/* Original: 1018:9176 */
void CResourceList_Dtor_1018_9176(SBResourceBatchLoader *self)
{
    if (!self) {
        return;
    }
    Handle_AllowPurge_1010_0196(self->rslt_04);
    CObject_Dtor_1008_ef8e(self);
}

typedef void (*SBResourceBatchEntryVisitor)(SBResourceHandle *h);

static void visit_rslt_entries(SBResourceBatchLoader *self, SBResourceBatchEntryVisitor visitor)
{
    const uint8_t *bytes;
    int16_t count;
    int16_t i;

    if (!self || !self->rslt_04 || !self->rslt_04->data_00) {
        return;
    }
    Handle_MoveNeck_1010_0608(self->rslt_04);
    Handle_Lock_1010_00e8(self->rslt_04);

    bytes = (const uint8_t *)self->rslt_04->data_00;
    count = (int16_t)read_u16_le(bytes);
    for (i = 0; i < count; ++i) {
        const uint8_t *rec = bytes + 2 + (i * 8);
        uint16_t win16_type = read_u16_le(rec + 0);
        char fourcc[4];
        uint16_t res_id = read_u16_le(rec + 6);
        SBResourceHandle *h;

        memcpy(fourcc, rec + 2, 4);
        h = ResMgr_GetResource_1010_10f2(win16_type, fourcc, res_id);
        if (h != NULL && visitor != NULL) {
            visitor(h);
        }
    }

    Handle_Unlock_1010_0122(self->rslt_04);
}

/* Original: 1018:919e */
void CResourceList_MarkAllNoPurge_1018_919e(SBResourceBatchLoader *self)
{
    visit_rslt_entries(self, Handle_NoPurge_1010_015c);
}

/* Original: 1018:9242 */
void CResourceList_MarkAllPurgeable_1018_9242(SBResourceBatchLoader *self)
{
    visit_rslt_entries(self, Handle_AllowPurge_1010_0196);
}

static void load_and_lock_visitor(SBResourceHandle *h)
{
    uint8_t saved_flags = Handle_GetFlags_1010_0096(h);
    if (g_DAT_1060_07f5_preloadFlag != 0) {
        Handle_NoPurge_1010_015c(h);
        Handle_Unlock_1010_0122(h);
        Handle_MoveNeck_1010_0608(h);
    }
    Handle_SetFlags_1010_00b0(h, saved_flags);
    Handle_Lock_1010_00e8(h);
}

/* Original: 1018:92e6 */
void CResourceList_LoadAndLockAll_1018_92e6(SBResourceBatchLoader *self)
{
    visit_rslt_entries(self, load_and_lock_visitor);
}

/* Original: 1018:93dc */
void CResourceList_UnlockAll_1018_93dc(SBResourceBatchLoader *self)
{
    visit_rslt_entries(self, Handle_Unlock_1010_0122);
}

/* Original: 1018:9480 */
void CResourceList_PreloadAll_1018_9480(SBResourceBatchLoader *self)
{
    visit_rslt_entries(self, NULL);
}

