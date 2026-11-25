#!/usr/bin/env python3
"""Generate a per-function reconstruction ledger from Ghidra's function dump.

Usage: python3 tools/reconstruction_progress.py build/functions.json
The input is produced by ghidra/DumpCoverageData.java.
"""
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INPUT_SHA256 = "ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2"
CODE_SEGMENTS = {"1000", "1008", "1010", "1018", "1020"}

MAPPED = {
    # Framework / Message Pump
    "1008:e412": ("decompiled", "framework", "src/recovered/framework/message_pump.c",
                  "Message ranges/order decompiled (`framework_drain_message_ranges`)."),

    # Framework / Base Object & Memory Allocators
    "1008:eed8": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Zero-initialized memory allocator (`AllocZeroed_1008_eed8`)."),
    "1008:ef1e": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Memory release helper (`FreeMem_1008_ef1e`)."),
    "1008:ef2c": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Zero-initialized C++ object allocator (`NewObject_1008_ef2c`)."),
    "1008:ef80": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Object memory deallocator (`DeleteObject_1008_ef80`)."),
    "1008:ef8e": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Root `CObject_1008_0bc8` virtual destructor (`CObject_Dtor_1008_ef8e`)."),
    "1008:ef9e": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Root `CObject_1008_0bc8` virtual no-op (`CObject_Noop_1008_ef9e`)."),

    # Framework / Handle Memory & Resource Flags
    "1010:0076": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Returns 32-bit byte size at handle `+0x08` (`Handle_GetSize_1010_0076`)."),
    "1010:0096": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Returns flag byte at handle `+0x0c` (`Handle_GetFlags_1010_0096`)."),
    "1010:00b0": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Updates handle flags via Resource Manager slot `+0x7c` (`Handle_SetFlags_1010_00b0`)."),
    "1010:00e8": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Sets locked bit `0x40` on handle (`Handle_Lock_1010_00e8`)."),
    "1010:0122": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Clears locked bit `0x40` on handle (`Handle_Unlock_1010_0122`)."),
    "1010:015c": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Clears purgeable bit `0x20` on handle (`Handle_NoPurge_1010_015c`)."),
    "1010:0196": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Sets purgeable bit `0x20` on handle (`Handle_AllowPurge_1010_0196`)."),
    "1010:01d0": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Disposes heap/global buffer and frees handle descriptor (`Handle_Dispose_1010_01d0`)."),
    "1010:023e": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Allocates heap/global buffer and double-indirected handle (`Handle_Alloc_1010_023e`)."),
    "1010:035c": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Allocates zero-initialized handle (`Handle_AllocZeroed_1010_035c`)."),
    "1010:03ee": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Resizes handle payload buffer (`Handle_Resize_1010_03ee`)."),
    "1010:057c": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Disposes handle via pointer and clears pointer (`Handle_FreeIndirect_1010_057c`)."),
    "1010:0608": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "No-op handle compaction stub (`Handle_MoveNeck_1010_0608`)."),

    # Framework / Resource Loading Pipeline (`SB_CDRES.DLL` & `BLIZZARD.EXE`)
    "1008:2bbc": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Loads `SB_CDRES.DLL` into `DAT_1060_001c` (`ResMgr_LoadCdResLibrary_1008_2bbc`)."),
    "1008:2bcc": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Frees `SB_CDRES.DLL` module handle (`ResMgr_FreeCdResLibrary_1008_2bcc`)."),
    "1008:2bdc": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Routes `'WIND'` to `BLIZZARD.EXE` and all other FourCCs to `SB_CDRES.DLL` (`ResMgr_GetResourceModule_1008_2bdc`)."),
    "1010:0fe6": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Finds, loads, sizes, and locks NE resource into handle (`ResHandle_LoadAndLock_1010_0fe6`)."),
    "1010:10f2": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Fetches or loads resource by `(win16_type_id, mac_fourcc, res_id)` (`ResMgr_GetResource_1010_10f2`)."),
    "1010:11d8": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Reloads purged resource handle on demand (`ResHandle_EnsureLoaded_1010_11d8`)."),
    "1010:1206": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Purges unlocked resources and dereferences handle (`ResHandle_PurgeAndDeref_1010_1206`)."),

    # Framework / CResourceTypeList_1010_2b12 & CResourceManager_1008_2da6
    "1010:25be": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "`CResourceTypeList_1010_2b12` virtual destructor (`CResourceTypeList_Dtor_1010_25be`)."),
    "1010:25e4": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Finds or creates resource handle entry by ID (`CResourceTypeList_FindOrCreateById_1010_25e4`)."),
    "1010:264c": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Copies 6-byte type descriptor (`CResourceTypeList_GetTypeDescriptor_1010_264c`)."),
    "1010:266c": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Allocates 0x1a resource handle and inserts ID mapping (`CResourceTypeList_CreateEntry_1010_266c`)."),
    "1010:26ba": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Purges unlocked purgeable resource handle (`CResourceTypeList_PurgeEntry_1010_26ba`)."),
    "1010:2712": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Purges all purgeable entries in type list (`CResourceTypeList_PurgeAll_1010_2712`)."),
    "1010:2784": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Frees resource handle and removes from type list (`CResourceTypeList_ReleaseEntry_1010_2784`)."),
    "1010:2810": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Releases all entries in type list (`CResourceTypeList_ReleaseAll_1010_2810`)."),
    "1008:65f2": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Initializes `CResourceManager_1008_2da6` (`CResourceManager_Init_1008_65f2`)."),
    "1010:286c": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Reads `0x03e8` `#1001`/`#1002` from `BLIZZARD.EXE` and builds 42 type lists (`CResourceMap_InitFromDirectory_1010_286c`)."),
    "1010:29ea": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "`CResourceMap_1008_2ece` virtual destructor (`CResourceMap_Dtor_1010_29ea`)."),
    "1010:2a14": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Updates `0x60` lock/purge bits on resource handle (`CResourceMap_UpdateEntryFlags_1010_2a14`)."),
    "1008:6602": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Purges `PICT` (`0x03f3`) immediately on unlock when purgeable (`CResourceManager_UpdateEntryFlags_1008_6602`)."),
    "1010:2a2e": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Finds `CResourceTypeList_1010_2b12` matching `(win16_type_id, mac_fourcc)` (`CResourceMap_FindTypeList_1010_2a2e`)."),
    "1010:2a54": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Purges all purgeable handles across all 42 type lists (`CResourceMap_PurgeAllTypes_1010_2a54`)."),
    "1010:2a74": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Releases all handles across all 42 type lists (`CResourceMap_ReleaseAllTypes_1010_2a74`)."),
    "1010:2a94": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Resource ID comparator callback (`ResEntry_CompareId_1010_2a94`)."),
    "1010:2ae2": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Callback invoking type list slot `+0x64` (`PurgeAll`)."),
    "1010:2afa": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Callback invoking type list slot `+0x6c` (`ReleaseAll`)."),

    # Framework / CResourceList_1008_821c (`RsLt` Batch Preloader)
    "1018:9140": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Loads `RsLt` (`0x03e9`) resource list by ID (`CResourceList_Init_1018_9140`)."),
    "1018:9176": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "`CResourceList_1008_821c` virtual destructor (`CResourceList_Dtor_1018_9176`)."),
    "1018:919e": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Marks all resources in `RsLt` non-purgeable (`CResourceList_MarkAllNoPurge_1018_919e`)."),
    "1018:9242": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Marks all resources in `RsLt` purgeable (`CResourceList_MarkAllPurgeable_1018_9242`)."),
    "1018:92e6": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Loads and locks all resources in `RsLt` (`CResourceList_LoadAndLockAll_1018_92e6`)."),
    "1018:93dc": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Unlocks all resources in `RsLt` (`CResourceList_UnlockAll_1018_93dc`)."),
    "1018:9480": ("decompiled", "framework", "src/recovered/framework/resource_manager.c",
                  "Preloads all resources in `RsLt` (`CResourceList_PreloadAll_1018_9480`)."),

    # Game / Player & High Score Record Model
    "1018:a704": ("decompiled", "application", "src/recovered/application/object_model.c",
                  "Top-7 descending ordered record insertion (`RecordTable_InsertTop7_1018_a704`)."),
    "1018:adb8": ("decompiled", "application", "src/recovered/application/object_model.c",
                  "Indexed word adjustment and accumulated total update (`1018:adb8`)."),
    "1018:ae3a": ("decompiled", "application", "src/recovered/application/object_model.c",
                  "High-water mark update and top-7 table sync (`1018:ae3a`)."),
    "1018:aeaa": ("win16-abi-checked", "application", "src/recovered/win16/application_primitives.c",
                  "Direct far entry follows parent +0x250 to holder +0 and reads nested +0x06; RETF 4 verified."),
    "1018:aede": ("win16-abi-checked", "application", "src/recovered/win16/application_primitives.c",
                  "Direct far entry writes nested +0x1a8 then parent +0x254; RETF 6 verified."),
    "1018:af08": ("win16-abi-checked", "application", "src/recovered/win16/application_primitives.c",
                  "Direct far entry returns AL=1 for signed argument <= nested +0x1a8; RETF 6 verified."),
    "1008:ac80": ("partial", "application", "src/recovered/application/application_state.c",
                  "0x2af9 global-state writes represented; full command switch pending."),
}


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    source = Path(sys.argv[1])
    dump = json.loads(source.read_text())
    funcs = [f for f in dump["functions"]
             if f["entry"].split(":", 1)[0].lower() in CODE_SEGMENTS]
    by_entry = {f["entry"].lower(): f for f in funcs}
    missing = sorted(set(MAPPED) - by_entry.keys())
    if missing:
        raise SystemExit("Mapped address missing from Ghidra inventory: " + ", ".join(missing))

    csv_path = ROOT / "docs/reconstruction-status.csv"
    fields = ["entry", "ghidra_name", "body_bytes", "status", "scope", "source", "notes"]
    counts = {"win16-abi-checked": 0, "decompiled": 0, "partial": 0, "not-started": 0}
    byte_counts = dict.fromkeys(counts, 0)
    with csv_path.open("w", newline="") as stream:
        stream.write(f"# input_sha256={INPUT_SHA256}; ghidra=12.2; code_segments={','.join(sorted(CODE_SEGMENTS))}\n")
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for f in sorted(funcs, key=lambda x: x["entry"].lower()):
            entry = f["entry"].lower()
            status, scope, src, notes = MAPPED.get(
                entry, ("not-started", "untriaged", "", "Classified in subsystem-map.md."))
            counts[status] += 1
            byte_counts[status] += f["body_size"]
            writer.writerow({"entry": entry, "ghidra_name": f["name"], "body_bytes": f["body_size"],
                             "status": status, "scope": scope, "source": src, "notes": notes})

    total = len(funcs)
    total_bytes = sum(f["body_size"] for f in funcs)
    done_funcs = counts["win16-abi-checked"] + counts["decompiled"]
    done_bytes = byte_counts["win16-abi-checked"] + byte_counts["decompiled"]
    pct_funcs = done_funcs * 100 / total if total else 0
    pct_bytes = done_bytes * 100 / total_bytes if total_bytes else 0

    report = [
        "# Reconstruction progress",
        "",
        f"Generated from Ghidra 12.2 function inventory for `original/BLIZZARD.EXE` (SHA-256 `{INPUT_SHA256}`).",
        "All functions in the five initialized code segments are listed in [reconstruction-status.csv](reconstruction-status.csv) and classified by subsystem in [subsystem-map.md](subsystem-map.md).",
        "",
        "## Function inventory",
        "",
        f"- Code-segment functions: **{total}** ({total_bytes:,} bytes)",
        f"- Decompiled & tested functions (Path 2 C + Win16 ABI): **{done_funcs} / {total} ({pct_funcs:.2f}%)**",
        f"- Decompiled & tested code bytes: **{done_bytes:,} / {total_bytes:,} ({pct_bytes:.2f}%)**",
        f"  - Clean C decompiled against Win16 shim (`decompiled`): **{counts['decompiled']}** ({byte_counts['decompiled']:,} bytes)",
        f"  - Win16 OMF ABI-checked entrypoints (`win16-abi-checked`): **{counts['win16-abi-checked']}** ({byte_counts['win16-abi-checked']:,} bytes)",
        f"- Partial original functions: **{counts['partial']}** ({byte_counts['partial']:,} bytes)",
        f"- Remaining functions (mapped in `docs/subsystem-map.md`): **{counts['not-started']}**",
        "",
        "## Decompiled Functions (Path 2 Clean C + Thin Win16 Shim)",
        "",
        "| Original entry | C source | Recovered behavior |",
        "|---|---|---|",
    ]
    for entry, (status, _, src, note) in sorted(MAPPED.items()):
        if status in ("decompiled", "win16-abi-checked"):
            report.append(f"| `{entry}` | `{src}` | {note} |")
    report += [
        "",
        "## Partial work",
        "",
        "`1008:ac80` has a C fragment for the `0x2af9` state writes. It is not a reconstruction of the full original function.",
        "",
        "Regenerate with `python3 tools/reconstruction_progress.py build/functions.json && python3 tools/map_subsystems.py`.",
        "",
    ]
    (ROOT / "docs/reconstruction-progress.md").write_text("\n".join(report))


if __name__ == "__main__":
    main()
