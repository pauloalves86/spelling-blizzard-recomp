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

    # Framework / 60 Hz Multimedia Timer, BSW Audio & WaveOut Streamer
    "1010:446a": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Constructs 60 Hz `TIMEGETTIME` timer at `DAT_1060_0a1e` (`CTimer60Hz_Ctor_1010_446a`)."),
    "1010:44d6": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "`CTimer60Hz_1010_456a` destructor (`CTimer60Hz_Dtor_1010_44d6`)."),
    "1010:44ec": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Advances 60 Hz Bresenham tick counter from `TIMEGETTIME` (`CTimer60Hz_GetTicks_1010_44ec`)."),
    "1010:07ae": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Global 60 Hz tick query (`Timer_GetTicks60Hz_1010_07ae`)."),
    "1010:24f8": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Inclusive bounded random integer helper (`Random_Range_1010_24f8`)."),
    "1010:312a": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Zero-initializes 10-byte `SBBswSample` (`BswSample_InitEmpty_1010_312a`)."),
    "1010:3152": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Frees PCM buffer in `SBBswSample` (`BswSample_FreeBuffer_1010_3152`)."),
    "1010:3172": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "`SBBswSample` constructor (`BswSample_Ctor_1010_3172`)."),
    "1010:3190": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Loads BSW audio sample directly from module resource (`BswSample_LoadFromExeResource_1010_3190`)."),
    "1010:31d8": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Parses 16-byte `BSW\\0` header and copies PCM payload (`BswSample_ParseHeaderAndCopyPcm_1010_31d8`)."),
    "1010:32bc": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Unprepares and frees chunked `WAVEHDR` stream buffers (`BswStream_UnprepareAndFree_1010_32bc`)."),
    "1010:3386": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Pumps next chunked `WAVEHDR` buffer to `waveOutWrite` (`BswStream_PumpNextChunk_1010_3386`)."),
    "1010:34ea": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Allocates and starts multi-chunk `waveOut` stream (`BswStream_StartChunked_1010_34ea`)."),
    "1010:36a6": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Constructs 50-byte `SBWavePlayer` at `DAT_1060_07cc` (`WavePlayer_Ctor_1010_36a6`)."),
    "1010:36d0": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Resets, unprepares, and closes `HWAVEOUT` (`WavePlayer_StopAndClose_1010_36d0`)."),
    "1010:37e6": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Opens `waveOut` at 11.025/22.05 kHz and writes `WAVEHDR` (`WavePlayer_OpenAndWrite_1010_37e6`)."),
    "1010:39d4": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Stops previous sound and starts new `SBBswSample` (`WavePlayer_PlaySample_1010_39d4`)."),
    "1010:3a10": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Sets 60 Hz playback completion tick (`WavePlayer_SetEndTick_1010_3a10`)."),
    "1010:3a24": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "No-op waveOut completion callback (`WavePlayer_NoopCallback_1010_3a24`)."),
    "1008:225a": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "`CSoundManager_1008_7edc` virtual destructor (`CSoundManager_Dtor_1008_225a`)."),
    "1008:22ac": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Checks if `waveOut` playback is still active (`CSoundManager_IsHardwareBusy_1008_22ac`)."),
    "1008:22ba": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Returns sound manager active flag `+0x16` (`CSoundManager_IsActive_1008_22ba`)."),
    "1008:22c8": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Returns sound interrupted flag `+0x0a` (`CSoundManager_WasInterrupted_1008_22c8`)."),
    "1008:22d6": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Loads `'snd '` (`0x03ee`) BSW resource and plays via `waveOut` (`CSoundManager_PlaySoundResource_1008_22d6`)."),
    "1008:23b2": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Waits for sound completion or user key/mouse interrupt (`CSoundManager_WaitOrInterrupt_1008_23b2`)."),
    "1008:2436": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Waits for sound playback to complete (`CSoundManager_WaitUntilDone_1008_2436`)."),
    "1008:2462": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Stops active sound and restores resource handle flags (`CSoundManager_Stop_1008_2462`)."),
    "1008:24ec": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Updates 10-minute (36000 tick) idle sound timer (`CSoundManager_IdleTimerCheck_1008_24ec`)."),
    "1008:2530": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Returns active `'snd '` resource ID (`CSoundManager_GetCurrentSoundId_1008_2530`)."),
    "1008:6a28": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Verifies Win 3.10+, 11.025 kHz 8-bit mono async `waveOut`, and display capabilities (`System_CheckRequirements_1008_6a28`)."),
    "1018:63b2": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "`CTimedCallback_1008_823c` flag `+0x0a` getter (`CTimedCallback_GetByte0a_1018_63b2`)."),
    "1018:63c0": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "`CTimedCallback_1008_823c` flag `+0x09` getter (`CTimedCallback_GetByte09_1018_63c0`)."),
    "1018:63ce": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "`CTimedCallback_1008_823c` paused flag `+0x08` getter (`CTimedCallback_IsPaused_1018_63ce`)."),
    "1018:63dc": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Resumes timed callback (`CTimedCallback_Resume_1018_63dc`)."),
    "1018:63ec": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Pauses timed callback (`CTimedCallback_Pause_1018_63ec`)."),
    "1018:63fc": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Timed callback virtual no-op (`CTimedCallback_OnStartNoop_1018_63fc`)."),
    "1018:6400": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Timed callback virtual no-op (`CTimedCallback_OnTickNoop_1018_6400`)."),
    "1018:6404": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Executes timed callback and schedules next interval (`CTimedCallback_Execute_1018_6404`)."),
    "1018:642c": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Checks if unpaused timed callback reached target 60 Hz tick (`CTimedCallback_IsReady_1018_642c`)."),
    "1018:646a": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Schedules next 60 Hz callback tick (`CTimedCallback_ScheduleNext_1018_646a`)."),
    "1018:648e": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Returns 0 (`CTimedCallback_False0_1018_648e`)."),
    "1018:6494": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Returns 0 (`CTimedCallback_False1_1018_6494`)."),
    "1020:27d8": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "`CSoundChannel_1010_bdd4` virtual destructor (`CSoundChannel_Dtor_1020_27d8`)."),
    "1020:2810": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Selects next sequential/random `'snd '` in channel and triggers playback (`CSoundChannel_ExecuteNext_1020_2810`)."),
    "1020:28e0": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Checks if sound channel is ready and sound manager is idle (`CSoundChannel_IsReady_1020_28e0`)."),
    "1020:2922": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Returns 1 (`CSoundChannel_True_1020_2922`)."),
    "1020:2928": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Appends `'snd '` resource ID to channel and locks handle (`CSoundChannel_AddSound_1020_2928`)."),
    "1020:29b6": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Sets primary `'snd '` resource ID on channel (`CSoundChannel_SetPrimarySound_1020_29b6`)."),
    "1020:2a96": ("decompiled", "framework", "src/recovered/framework/audio_waveout.c",
                  "Stops and unlocks all `'snd '` handles in channel (`CSoundChannel_ClearSounds_1020_2a96`)."),

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
