/* Decompiled Bright Star Resource Manager & Handle Subsystem (Path 2)
 *
 * Reconstructs the 31 functions governing memory handles, dynamic lists,
 * resource collections, and the global Resource Manager (`DAT_1060_0a14`):
 *
 * 1. Base Object & Allocators (`CObject_1008_0bc8`):
 *    - `AllocZeroed_1008_eed8` (`1008:eed8`)
 *    - `FreeMem_1008_ef1e` (`1008:ef1e`)
 *    - `NewObject_1008_ef2c` (`1008:ef2c`)
 *    - `DeleteObject_1008_ef80` (`1008:ef80`)
 *    - `CObject_Dtor_1008_ef8e` (`1008:ef8e`)
 *    - `CObject_Noop_1008_ef9e` (`1008:ef9e`)
 *
 * 2. Handle Memory & Resource Flags (`1010:0076..0608`):
 *    - `Handle_GetSize_1010_0076` (`1010:0076`)
 *    - `Handle_GetFlags_1010_0096` (`1010:0096`)
 *    - `Handle_SetFlags_1010_00b0` (`1010:00b0`)
 *    - `Handle_Lock_1010_00e8` (`1010:00e8`)
 *    - `Handle_Unlock_1010_0122` (`1010:0122`)
 *    - `Handle_NoPurge_1010_015c` (`1010:015c`)
 *    - `Handle_AllowPurge_1010_0196` (`1010:0196`)
 *    - `Handle_Dispose_1010_01d0` (`1010:01d0`)
 *    - `Handle_Alloc_1010_023e` (`1010:023e`)
 *    - `Handle_AllocZeroed_1010_035c` (`1010:035c`)
 *    - `Handle_Resize_1010_03ee` (`1010:03ee`)
 *    - `Handle_FreeIndirect_1010_057c` (`1010:057c`)
 *    - `Handle_MoveNeck_1010_0608` (`1010:0608`)
 *
 * 3. Resource Loading Pipeline (`1008:2bbc..2bdc`, `1010:0fe6..1206`):
 *    - `ResMgr_LoadCdResLibrary_1008_2bbc` (`1008:2bbc`)
 *    - `ResMgr_FreeCdResLibrary_1008_2bcc` (`1008:2bcc`)
 *    - `ResMgr_GetResourceModule_1008_2bdc` (`1008:2bdc`)
 *    - `ResHandle_LoadAndLock_1010_0fe6` (`1010:0fe6`)
 *    - `ResMgr_GetResource_1010_10f2` (`1010:10f2`)
 *    - `ResHandle_EnsureLoaded_1010_11d8` (`1010:11d8`)
 *    - `ResHandle_PurgeAndDeref_1010_1206` (`1010:1206`)
 *
 * 4. Resource Type Collection (`CResourceTypeList_1010_2b12`):
 *    - `CResourceTypeList_Dtor_1010_25be` (`1010:25be`)
 *    - `CResourceTypeList_FindOrCreateById_1010_25e4` (`1010:25e4`)
 *    - `CResourceTypeList_GetTypeDescriptor_1010_264c` (`1010:264c`)
 *    - `CResourceTypeList_CreateEntry_1010_266c` (`1010:266c`)
 *    - `CResourceTypeList_PurgeEntry_1010_26ba` (`1010:26ba`)
 *    - `CResourceTypeList_PurgeAll_1010_2712` (`1010:2712`)
 *    - `CResourceTypeList_ReleaseEntry_1010_2784` (`1010:2784`)
 *    - `CResourceTypeList_ReleaseAll_1010_2810` (`1010:2810`)
 *
 * 5. Global Resource Manager (`CResourceMap_1008_2ece` / `CResourceManager_1008_2da6`):
 *    - `CResourceManager_Init_1008_65f2` (`1008:65f2`)
 *    - `CResourceMap_InitFromDirectory_1010_286c` (`1010:286c`)
 *    - `CResourceMap_Dtor_1010_29ea` (`1010:29ea`)
 *    - `CResourceMap_UpdateEntryFlags_1010_2a14` (`1010:2a14`)
 *    - `CResourceManager_UpdateEntryFlags_1008_6602` (`1008:6602`)
 *    - `CResourceMap_FindTypeList_1010_2a2e` (`1010:2a2e`)
 *    - `CResourceMap_PurgeAllTypes_1010_2a54` (`1010:2a54`)
 *    - `CResourceMap_ReleaseAllTypes_1010_2a74` (`1010:2a74`)
 *    - `ResEntry_CompareId_1010_2a94` (`1010:2a94`)
 *    - `ResTypeList_MatchDescriptor_1010_2aa6` (`1010:2aa6`)
 */
#ifndef SB_RECOVERED_FRAMEWORK_RESOURCE_MANAGER_H
#define SB_RECOVERED_FRAMEWORK_RESOURCE_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include "../../shim/win16_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Flag bit definitions at SBResourceHandle+0x0c (`flags_0c`) */
#define SB_HANDLE_FLAG_LOCKING_IN_PROGRESS 0x01u
#define SB_HANDLE_FLAG_HEAP_OR_LOCKED      0x02u
#define SB_HANDLE_FLAG_RES_LOADED          0x04u
#define SB_HANDLE_FLAG_PURGED              0x08u
#define SB_HANDLE_FLAG_IS_RESOURCE         0x10u
#define SB_HANDLE_FLAG_PURGEABLE           0x20u
#define SB_HANDLE_FLAG_LOCKED              0x40u

/* 6-byte resource type descriptor (matches Segment 7 entries at 0x36c80) */
typedef struct SBResourceTypeDesc {
    uint16_t win16_type_id; /* +0x00: e.g. 0x03ee (1006) for 'snd ' */
    char mac_fourcc[4];     /* +0x02: e.g. {'s','n','d',' '} */
} SBResourceTypeDesc;

/* 26-byte (`0x1a`) Resource Handle Block (and 14-byte `0x0e` Heap Handle prefix).
 * In original 16-bit code, `1010:10f2` returns `self_04` (which points back to
 * `+0x00`, the `data_00` pointer), emulating a Macintosh double-indirected `Handle`
 * (`**h` is the payload bytes, and container fields are at fixed offsets from `*h`).
 */
typedef struct SBResourceHandle {
    const void *data_00;          /* +0x00: locked payload pointer */
    struct SBResourceHandle *self_04; /* +0x04: self pointer (`*handle == handle`) */
    uint32_t size_08;             /* +0x08: resource byte size */
    uint8_t flags_0c;             /* +0x0c: SB_HANDLE_FLAG_* bitmask */
    uint8_t pad_0d;               /* +0x0d */
    SB_HGLOBAL hGlobal_0e;        /* +0x0e: Win16 HGLOBAL from LoadResource */
    SB_HRSRC hRsrc_10;            /* +0x10: Win16 HRSRC from FindResource */
    uint16_t res_id_12;           /* +0x12: integer resource ID */
    SBResourceTypeDesc type_14;   /* +0x14..+0x19: 6-byte resource type descriptor */
} SBResourceHandle;

/* 6-byte entry stored inside `CResourceTypeList_1010_2b12` array */
typedef struct SBResourceIdMapEntry {
    uint16_t res_id_00;           /* +0x00: resource ID */
    SBResourceHandle *handle_02;  /* +0x02: pointer to 0x1a SBResourceHandle */
} SBResourceIdMapEntry;

/* Per-type resource list (`CResourceTypeList_1010_2b12`, inherits `CList_1008_0904`) */
typedef struct SBResourceTypeList {
    uint32_t vtable_tag_00;       /* +0x00: 0x10102b12 */
    int32_t count_04;             /* +0x04: active entry count */
    int16_t grow_step_08;         /* +0x08: capacity growth increment */
    int32_t capacity_0a;          /* +0x0a: allocated element capacity */
    SBResourceIdMapEntry *items_0e; /* +0x0e: array of (res_id, handle) pairs */
    uint32_t elem_size_12;        /* +0x12: 6 bytes */
    uint8_t lock_flag_16;         /* +0x16 */
    uint8_t swap_flag_17;         /* +0x17 */
    SBResourceTypeDesc type_18;   /* +0x18..+0x1d: 6-byte type descriptor */
    uint16_t total_expected_1e;   /* +0x1e: expected resource count from directory */
} SBResourceTypeList;

/* Global Resource Manager (`CResourceManager_1008_2da6` at `DAT_1060_0a14`) */
typedef struct SBResourceManager {
    uint32_t vtable_tag_00;       /* +0x00: 0x10082da6 */
    int32_t count_04;             /* +0x04: number of registered resource types (42) */
    int16_t grow_step_08;         /* +0x08 */
    int32_t capacity_0a;          /* +0x0a */
    SBResourceTypeList **types_0e;/* +0x0e: array of pointers to SBResourceTypeList */
} SBResourceManager;

/* Resource Batch Preloader (`CResourceList_1008_821c`, size 8 bytes, loads `RsLt` `0x03e9`) */
typedef struct SBResourceBatchLoader {
    uint32_t vtable_tag_00;       /* +0x00: 0x1008821c */
    SBResourceHandle *rslt_04;    /* +0x04: handle to `RsLt` (`0x03e9`) resource */
} SBResourceBatchLoader;

/* Global state mirroring Segment 1060 (`DAT_1060_*`) */
extern SB_HMODULE g_DAT_1060_07d0_hInstance;   /* BLIZZARD.EXE module handle */
extern SB_HMODULE g_DAT_1060_001c_hCdResModule;/* SB_CDRES.DLL module handle */
extern SBResourceManager *g_DAT_1060_0a14_resMgr; /* Global Resource Manager */
extern uint8_t g_DAT_1060_07f5_preloadFlag;    /* Preload lock/no-purge mode flag */

/* 1. Allocator & Base Object */
void *AllocZeroed_1008_eed8(size_t size);
void FreeMem_1008_ef1e(void *ptr);
void *NewObject_1008_ef2c(size_t size);
void DeleteObject_1008_ef80(void *ptr);
void CObject_Dtor_1008_ef8e(void *obj);
void CObject_Noop_1008_ef9e(void);

/* 2. Handle Memory & Resource Flags (`1010:0076..0608`) */
uint32_t Handle_GetSize_1010_0076(const SBResourceHandle *h);
uint8_t Handle_GetFlags_1010_0096(const SBResourceHandle *h);
void Handle_SetFlags_1010_00b0(SBResourceHandle *h, uint8_t flags);
void Handle_Lock_1010_00e8(SBResourceHandle *h);
void Handle_Unlock_1010_0122(SBResourceHandle *h);
void Handle_NoPurge_1010_015c(SBResourceHandle *h);
void Handle_AllowPurge_1010_0196(SBResourceHandle *h);
void Handle_Dispose_1010_01d0(SBResourceHandle *h);
SBResourceHandle *Handle_Alloc_1010_023e(uint32_t size, int zero_init);
SBResourceHandle *Handle_AllocZeroed_1010_035c(uint32_t size);
void Handle_Resize_1010_03ee(SBResourceHandle *h, uint32_t new_size);
void Handle_FreeIndirect_1010_057c(SBResourceHandle **h_ptr);
void Handle_MoveNeck_1010_0608(SBResourceHandle *h);

/* 3. Resource Loading Pipeline (`1008:2bbc..2bdc`, `1010:0fe6..1206`) */
void ResMgr_LoadCdResLibrary_1008_2bbc(void);
void ResMgr_FreeCdResLibrary_1008_2bcc(void);
SB_HMODULE ResMgr_GetResourceModule_1008_2bdc(uint16_t win16_type_id, const char mac_fourcc[4]);
uint8_t ResHandle_LoadAndLock_1010_0fe6(SBResourceHandle *h);
SBResourceHandle *ResMgr_GetResource_1010_10f2(uint16_t win16_type_id,
                                               const char mac_fourcc[4],
                                               uint16_t res_id);
void ResHandle_EnsureLoaded_1010_11d8(SBResourceHandle *h);
const void *ResHandle_PurgeAndDeref_1010_1206(SBResourceHandle *h);

/* 4. Resource Type Collection (`CResourceTypeList_1010_2b12`) */
SBResourceTypeList *CResourceTypeList_New(const SBResourceTypeDesc *desc, uint16_t expected_count);
void CResourceTypeList_Dtor_1010_25be(SBResourceTypeList *self);
SBResourceHandle *CResourceTypeList_FindOrCreateById_1010_25e4(SBResourceTypeList *self, uint16_t res_id);
void CResourceTypeList_GetTypeDescriptor_1010_264c(const SBResourceTypeList *self, SBResourceTypeDesc *out_desc);
SBResourceHandle *CResourceTypeList_CreateEntry_1010_266c(SBResourceTypeList *self, uint16_t res_id);
void CResourceTypeList_PurgeEntry_1010_26ba(SBResourceTypeList *self, SBResourceHandle *h);
void CResourceTypeList_PurgeAll_1010_2712(SBResourceTypeList *self);
void CResourceTypeList_ReleaseEntry_1010_2784(SBResourceTypeList *self, int32_t index_1based, SBResourceHandle *h);
void CResourceTypeList_ReleaseAll_1010_2810(SBResourceTypeList *self);

/* 5. Global Resource Manager (`CResourceMap_1008_2ece` / `CResourceManager_1008_2da6`) */
SBResourceManager *CResourceManager_CreateGlobal_1008_2bf4_part(void);
void CResourceManager_Init_1008_65f2(SBResourceManager *self);
void CResourceMap_InitFromDirectory_1010_286c(SBResourceManager *self);
void CResourceMap_Dtor_1010_29ea(SBResourceManager *self);
void CResourceMap_UpdateEntryFlags_1010_2a14(SBResourceManager *self, uint8_t new_flags, SBResourceHandle *h);
void CResourceManager_UpdateEntryFlags_1008_6602(SBResourceManager *self, uint8_t new_flags, SBResourceHandle *h);
SBResourceTypeList *CResourceMap_FindTypeList_1010_2a2e(SBResourceManager *self,
                                                        uint16_t win16_type_id,
                                                        const char mac_fourcc[4]);
void CResourceMap_PurgeAllTypes_1010_2a54(SBResourceManager *self);
void CResourceMap_ReleaseAllTypes_1010_2a74(SBResourceManager *self);
int ResEntry_CompareId_1010_2a94(const uint16_t *target_id, const SBResourceIdMapEntry *entry);
int ResTypeList_MatchDescriptor_1010_2aa6(const SBResourceTypeList *list, const SBResourceTypeDesc *target);

/* 6. Resource Batch Preloader (`CResourceList_1008_821c`, `1018:9140..9480`) */
void CResourceList_Init_1018_9140(SBResourceBatchLoader *self, uint16_t rslt_res_id);
void CResourceList_Dtor_1018_9176(SBResourceBatchLoader *self);
void CResourceList_MarkAllNoPurge_1018_919e(SBResourceBatchLoader *self);
void CResourceList_MarkAllPurgeable_1018_9242(SBResourceBatchLoader *self);
void CResourceList_LoadAndLockAll_1018_92e6(SBResourceBatchLoader *self);
void CResourceList_UnlockAll_1018_93dc(SBResourceBatchLoader *self);
void CResourceList_PreloadAll_1018_9480(SBResourceBatchLoader *self);

#ifdef __cplusplus
}
#endif

#endif /* SB_RECOVERED_FRAMEWORK_RESOURCE_MANAGER_H */
