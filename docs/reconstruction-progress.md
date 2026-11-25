# Reconstruction progress

Generated from Ghidra 12.2 function inventory for `original/BLIZZARD.EXE` (SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`).
All functions in the five initialized code segments are listed in [reconstruction-status.csv](reconstruction-status.csv) and classified by subsystem in [subsystem-map.md](subsystem-map.md).

## Function inventory

- Code-segment functions: **1422** (180,530 bytes)
- Decompiled & tested functions (Path 2 C + Win16 ABI): **59 / 1422 (4.15%)**
- Decompiled & tested code bytes: **6,436 / 180,530 (3.57%)**
  - Clean C decompiled against Win16 shim (`decompiled`): **56** (6,337 bytes)
  - Win16 OMF ABI-checked entrypoints (`win16-abi-checked`): **3** (99 bytes)
- Partial original functions: **1** (340 bytes)
- Remaining functions (mapped in `docs/subsystem-map.md`): **1362**

## Decompiled Functions (Path 2 Clean C + Thin Win16 Shim)

| Original entry | C source | Recovered behavior |
|---|---|---|
| `1008:2bbc` | `src/recovered/framework/resource_manager.c` | Loads `SB_CDRES.DLL` into `DAT_1060_001c` (`ResMgr_LoadCdResLibrary_1008_2bbc`). |
| `1008:2bcc` | `src/recovered/framework/resource_manager.c` | Frees `SB_CDRES.DLL` module handle (`ResMgr_FreeCdResLibrary_1008_2bcc`). |
| `1008:2bdc` | `src/recovered/framework/resource_manager.c` | Routes `'WIND'` to `BLIZZARD.EXE` and all other FourCCs to `SB_CDRES.DLL` (`ResMgr_GetResourceModule_1008_2bdc`). |
| `1008:65f2` | `src/recovered/framework/resource_manager.c` | Initializes `CResourceManager_1008_2da6` (`CResourceManager_Init_1008_65f2`). |
| `1008:6602` | `src/recovered/framework/resource_manager.c` | Purges `PICT` (`0x03f3`) immediately on unlock when purgeable (`CResourceManager_UpdateEntryFlags_1008_6602`). |
| `1008:e412` | `src/recovered/framework/message_pump.c` | Message ranges/order decompiled (`framework_drain_message_ranges`). |
| `1008:eed8` | `src/recovered/framework/resource_manager.c` | Zero-initialized memory allocator (`AllocZeroed_1008_eed8`). |
| `1008:ef1e` | `src/recovered/framework/resource_manager.c` | Memory release helper (`FreeMem_1008_ef1e`). |
| `1008:ef2c` | `src/recovered/framework/resource_manager.c` | Zero-initialized C++ object allocator (`NewObject_1008_ef2c`). |
| `1008:ef80` | `src/recovered/framework/resource_manager.c` | Object memory deallocator (`DeleteObject_1008_ef80`). |
| `1008:ef8e` | `src/recovered/framework/resource_manager.c` | Root `CObject_1008_0bc8` virtual destructor (`CObject_Dtor_1008_ef8e`). |
| `1008:ef9e` | `src/recovered/framework/resource_manager.c` | Root `CObject_1008_0bc8` virtual no-op (`CObject_Noop_1008_ef9e`). |
| `1010:0076` | `src/recovered/framework/resource_manager.c` | Returns 32-bit byte size at handle `+0x08` (`Handle_GetSize_1010_0076`). |
| `1010:0096` | `src/recovered/framework/resource_manager.c` | Returns flag byte at handle `+0x0c` (`Handle_GetFlags_1010_0096`). |
| `1010:00b0` | `src/recovered/framework/resource_manager.c` | Updates handle flags via Resource Manager slot `+0x7c` (`Handle_SetFlags_1010_00b0`). |
| `1010:00e8` | `src/recovered/framework/resource_manager.c` | Sets locked bit `0x40` on handle (`Handle_Lock_1010_00e8`). |
| `1010:0122` | `src/recovered/framework/resource_manager.c` | Clears locked bit `0x40` on handle (`Handle_Unlock_1010_0122`). |
| `1010:015c` | `src/recovered/framework/resource_manager.c` | Clears purgeable bit `0x20` on handle (`Handle_NoPurge_1010_015c`). |
| `1010:0196` | `src/recovered/framework/resource_manager.c` | Sets purgeable bit `0x20` on handle (`Handle_AllowPurge_1010_0196`). |
| `1010:01d0` | `src/recovered/framework/resource_manager.c` | Disposes heap/global buffer and frees handle descriptor (`Handle_Dispose_1010_01d0`). |
| `1010:023e` | `src/recovered/framework/resource_manager.c` | Allocates heap/global buffer and double-indirected handle (`Handle_Alloc_1010_023e`). |
| `1010:035c` | `src/recovered/framework/resource_manager.c` | Allocates zero-initialized handle (`Handle_AllocZeroed_1010_035c`). |
| `1010:03ee` | `src/recovered/framework/resource_manager.c` | Resizes handle payload buffer (`Handle_Resize_1010_03ee`). |
| `1010:057c` | `src/recovered/framework/resource_manager.c` | Disposes handle via pointer and clears pointer (`Handle_FreeIndirect_1010_057c`). |
| `1010:0608` | `src/recovered/framework/resource_manager.c` | No-op handle compaction stub (`Handle_MoveNeck_1010_0608`). |
| `1010:0fe6` | `src/recovered/framework/resource_manager.c` | Finds, loads, sizes, and locks NE resource into handle (`ResHandle_LoadAndLock_1010_0fe6`). |
| `1010:10f2` | `src/recovered/framework/resource_manager.c` | Fetches or loads resource by `(win16_type_id, mac_fourcc, res_id)` (`ResMgr_GetResource_1010_10f2`). |
| `1010:11d8` | `src/recovered/framework/resource_manager.c` | Reloads purged resource handle on demand (`ResHandle_EnsureLoaded_1010_11d8`). |
| `1010:1206` | `src/recovered/framework/resource_manager.c` | Purges unlocked resources and dereferences handle (`ResHandle_PurgeAndDeref_1010_1206`). |
| `1010:25be` | `src/recovered/framework/resource_manager.c` | `CResourceTypeList_1010_2b12` virtual destructor (`CResourceTypeList_Dtor_1010_25be`). |
| `1010:25e4` | `src/recovered/framework/resource_manager.c` | Finds or creates resource handle entry by ID (`CResourceTypeList_FindOrCreateById_1010_25e4`). |
| `1010:264c` | `src/recovered/framework/resource_manager.c` | Copies 6-byte type descriptor (`CResourceTypeList_GetTypeDescriptor_1010_264c`). |
| `1010:266c` | `src/recovered/framework/resource_manager.c` | Allocates 0x1a resource handle and inserts ID mapping (`CResourceTypeList_CreateEntry_1010_266c`). |
| `1010:26ba` | `src/recovered/framework/resource_manager.c` | Purges unlocked purgeable resource handle (`CResourceTypeList_PurgeEntry_1010_26ba`). |
| `1010:2712` | `src/recovered/framework/resource_manager.c` | Purges all purgeable entries in type list (`CResourceTypeList_PurgeAll_1010_2712`). |
| `1010:2784` | `src/recovered/framework/resource_manager.c` | Frees resource handle and removes from type list (`CResourceTypeList_ReleaseEntry_1010_2784`). |
| `1010:2810` | `src/recovered/framework/resource_manager.c` | Releases all entries in type list (`CResourceTypeList_ReleaseAll_1010_2810`). |
| `1010:286c` | `src/recovered/framework/resource_manager.c` | Reads `0x03e8` `#1001`/`#1002` from `BLIZZARD.EXE` and builds 42 type lists (`CResourceMap_InitFromDirectory_1010_286c`). |
| `1010:29ea` | `src/recovered/framework/resource_manager.c` | `CResourceMap_1008_2ece` virtual destructor (`CResourceMap_Dtor_1010_29ea`). |
| `1010:2a14` | `src/recovered/framework/resource_manager.c` | Updates `0x60` lock/purge bits on resource handle (`CResourceMap_UpdateEntryFlags_1010_2a14`). |
| `1010:2a2e` | `src/recovered/framework/resource_manager.c` | Finds `CResourceTypeList_1010_2b12` matching `(win16_type_id, mac_fourcc)` (`CResourceMap_FindTypeList_1010_2a2e`). |
| `1010:2a54` | `src/recovered/framework/resource_manager.c` | Purges all purgeable handles across all 42 type lists (`CResourceMap_PurgeAllTypes_1010_2a54`). |
| `1010:2a74` | `src/recovered/framework/resource_manager.c` | Releases all handles across all 42 type lists (`CResourceMap_ReleaseAllTypes_1010_2a74`). |
| `1010:2a94` | `src/recovered/framework/resource_manager.c` | Resource ID comparator callback (`ResEntry_CompareId_1010_2a94`). |
| `1010:2ae2` | `src/recovered/framework/resource_manager.c` | Callback invoking type list slot `+0x64` (`PurgeAll`). |
| `1010:2afa` | `src/recovered/framework/resource_manager.c` | Callback invoking type list slot `+0x6c` (`ReleaseAll`). |
| `1018:9140` | `src/recovered/framework/resource_manager.c` | Loads `RsLt` (`0x03e9`) resource list by ID (`CResourceList_Init_1018_9140`). |
| `1018:9176` | `src/recovered/framework/resource_manager.c` | `CResourceList_1008_821c` virtual destructor (`CResourceList_Dtor_1018_9176`). |
| `1018:919e` | `src/recovered/framework/resource_manager.c` | Marks all resources in `RsLt` non-purgeable (`CResourceList_MarkAllNoPurge_1018_919e`). |
| `1018:9242` | `src/recovered/framework/resource_manager.c` | Marks all resources in `RsLt` purgeable (`CResourceList_MarkAllPurgeable_1018_9242`). |
| `1018:92e6` | `src/recovered/framework/resource_manager.c` | Loads and locks all resources in `RsLt` (`CResourceList_LoadAndLockAll_1018_92e6`). |
| `1018:93dc` | `src/recovered/framework/resource_manager.c` | Unlocks all resources in `RsLt` (`CResourceList_UnlockAll_1018_93dc`). |
| `1018:9480` | `src/recovered/framework/resource_manager.c` | Preloads all resources in `RsLt` (`CResourceList_PreloadAll_1018_9480`). |
| `1018:a704` | `src/recovered/application/object_model.c` | Top-7 descending ordered record insertion (`RecordTable_InsertTop7_1018_a704`). |
| `1018:adb8` | `src/recovered/application/object_model.c` | Indexed word adjustment and accumulated total update (`1018:adb8`). |
| `1018:ae3a` | `src/recovered/application/object_model.c` | High-water mark update and top-7 table sync (`1018:ae3a`). |
| `1018:aeaa` | `src/recovered/win16/application_primitives.c` | Direct far entry follows parent +0x250 to holder +0 and reads nested +0x06; RETF 4 verified. |
| `1018:aede` | `src/recovered/win16/application_primitives.c` | Direct far entry writes nested +0x1a8 then parent +0x254; RETF 6 verified. |
| `1018:af08` | `src/recovered/win16/application_primitives.c` | Direct far entry returns AL=1 for signed argument <= nested +0x1a8; RETF 6 verified. |

## Partial work

`1008:ac80` has a C fragment for the `0x2af9` state writes. It is not a reconstruction of the full original function.

Regenerate with `python3 tools/reconstruction_progress.py build/functions.json && python3 tools/map_subsystems.py`.
