#!/usr/bin/env python3
"""Map all 1,422 code-segment functions and 142 virtual method tables of
original/BLIZZARD.EXE into subsystems, using:
1. Segment 7 Resource Type Descriptor Table (Mac FourCC -> Win16 Resource ID)
2. Direct and transitive callers of the resource loader (1010:0fe6 / 1010:10f2 / 1018:908c)
3. Vtable hierarchy and method ownership (build/method_tables.json)
4. Win16 imported APIs (KERNEL, GDI, USER, MMSYSTEM, SHELL, KEYBOARD)
5. Address ranges and Segment 1 CRT classification

Outputs:
- build/subsystem_map.json
- docs/subsystem-map.md
- docs/reconstruction-status.csv (enriched with subsystem and class/vtable ownership)
"""

import csv
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import ne_farptr as N

# 1. Decode Segment 7 Resource Type Descriptor Table (0x36c80, 0xde bytes = 37 entries of 6 bytes)
SEG7_FILE_OFF = 0x36C80
SEG7_LEN = 0xDE

RES_TYPE_DESCRIPTIONS = {
    "AnRv": ("Animation Reverse / Sequence Data", "Graphics/Animation"),
    "AnSq": ("Animation Sequence (`TYPE_0x840b`)", "Graphics/Animation"),
    "AtMt": ("Actor / Attack Matrix (`TYPE_0x83fe`)", "Game/Actors"),
    "ClTx": ("Color Text Style (`ColorText`, `TYPE_0x83f6`)", "UI/Controls"),
    "clut": ("Color Lookup Table / Palette (`TYPE_0x83ef`)", "Graphics/Palette"),
    "CnPn": ("Control Panel (`TYPE_0x8409`)", "UI/Controls"),
    "CPic": ("Control Picture (`TYPE_0x83f2`)", "UI/Controls"),
    "CpLt": ("Control Panel List (`TYPE_0x83f8`)", "UI/Controls"),
    "FrSq": ("Frame Sequence (`itsFrameSequenceList`, `TYPE_0x83ff`)", "Graphics/Animation"),
    "FrTr": ("Frame Tree / Tile Map (`TYPE_0x840c`)", "Game/Board & Level"),
    "GmBn": ("Game Button (`TYPE_0x83f7`)", "UI/Controls"),
    "GmLt": ("Game Level / Word List (`TYPE_0x8407`)", "Game/Spelling & Words"),
    "GmTr": ("Game Word Tree / Table (`TYPE_0x8406`)", "Game/Spelling & Words"),
    "KeyE": ("Keyboard / Entry Control (`TYPE_0x83fb`)", "UI/Controls"),
    "KeyI": ("Keyboard / Input Control (`TYPE_0x83fc`)", "UI/Controls"),
    "KeyO": ("Keyboard / Option Control (`TYPE_0x83fa`)", "UI/Controls"),
    "MpRv": ("Map / View Resource (`TYPE_0x83fd`)", "Game/Board & Level"),
    "MvAc": ("Move Action / Plan (`itsPlanList`, `TYPE_0x8400`)", "Game/Actors"),
    "OvSt": ("Overlay / State Data (`TYPE_0x8401`)", "Game/State"),
    "Pane": ("UI Pane Layout (`CView`/`Pane`, `TYPE_0x83f1`)", "UI/Views & Windows"),
    "PcCl": ("Picture Collection (`TYPE_0x83ea`)", "Graphics/Sprites"),
    "PICT": ("Sprite / Picture Asset (`TYPE_0x83f3`, 2.20 MB)", "Graphics/Sprites"),
    "RlCn": ("Rule / Scene Controller (`TYPE_0x840a`)", "Game/State"),
    "RlPn": ("Rule / Panel Metadata (`TYPE_0x83f5`)", "UI/Controls"),
    "RlQt": ("Rule / Quote / Text Data (`TYPE_0x83eb`)", "Game/Spelling & Words"),
    "RlSy": ("DIB Bitmap / Symbol (`BITMAPINFOHEADER`, `TYPE_0x83ec`)", "Graphics/Bitmaps"),
    "RsLt": ("Resource List Directory (`TYPE_0x83e9`)", "Resource Manager"),
    "SAIL": ("Script / AI / Actor Logic (`TYPE_0x83f9`)", "Game/Actors"),
    "SdEf": ("Sound Effect List (`itsSoundEffectsList`, `TYPE_0x8402`)", "Audio/WaveOut"),
    "SdGd": ("Spelling Demon Object Grid (`itsSpellingDemonObjectGrid`, `TYPE_0x8405`)", "Game/Board & Level"),
    "SdOb": ("Spelling Demon Game Object / Word Record (`TYPE_0x8403`)", "Game/Actors & Words"),
    "SdTr": ("Spelling Demon Tree / Directory (`0x0000`)", "Game/Board & Level"),
    "snd ": ("BSW Audio Stream (`BSW\\0` 11,025 Hz 8-bit mono, `TYPE_0x83ee`, 7.10 MB)", "Audio/WaveOut"),
    "SpEf": ("Special Effects List (`itsSpecialEffectsList`, `TYPE_0x8404`)", "Graphics/Animation"),
    "STR#": ("String List (`TYPE_0x83ed`)", "UI/Views & Windows"),
    "Yobi": ("Auxiliary Scene / Object Descriptor (`TYPE_0x83f4`)", "Game/State"),
    "WIND": ("Window Template (`BLIZZARD.EXE` resource `TYPE_0x83f0`)", "UI/Views & Windows"),
}


def decode_seg7_table():
    seg7 = N.data[SEG7_FILE_OFF : SEG7_FILE_OFF + SEG7_LEN]
    table = {}
    for off in range(0, len(seg7), 6):
        tid = struct.unpack_from("<H", seg7, off)[0]
        fcc = seg7[off + 2 : off + 6].decode("ascii", errors="replace")
        desc, subsys = RES_TYPE_DESCRIPTIONS.get(fcc, ("Custom Resource", "Resource Manager"))
        table[off] = {
            "seg7_offset": f"0x{off:02x}",
            "win16_type_id": tid,
            "win16_rt_hex": f"0x{0x8000 | tid:04x}" if tid else "0x0000",
            "mac_fourcc": fcc,
            "description": desc,
            "subsystem": subsys,
        }
    return table


def scan_function_resource_types(funcs, seg7_table):
    seg_map = {"1000": 1, "1008": 2, "1010": 3, "1018": 4, "1020": 5}
    func_res = defaultdict(list)
    for f in funcs:
        s_num = seg_map.get(f["segment"])
        if not s_num:
            continue
        fo, ln, _ = N.segs[s_num]
        f_off = int(f["entry"].split(":")[1], 16)
        f_bytes = N.data[fo + f_off : fo + f_off + f["size"]]
        imms = []
        for i in range(len(f_bytes) - 3):
            b0, b1 = f_bytes[i], f_bytes[i + 1]
            if b0 == 0xA1:
                imms.append((i, struct.unpack_from("<H", f_bytes, i + 1)[0]))
            elif (b0 == 0xFF and b1 == 0x36) or (b0 == 0x8B and (b1 & 0xC7) == 0x06):
                imms.append((i, struct.unpack_from("<H", f_bytes, i + 2)[0]))
        found_bases = set()
        for idx, (pos1, imm1) in enumerate(imms):
            base = (imm1 // 6) * 6
            if base in seg7_table and (imm1 - base) in (0, 2, 4):
                for pos2, imm2 in imms[max(0, idx - 4) : min(len(imms), idx + 5)]:
                    if (
                        pos1 != pos2
                        and abs(pos1 - pos2) <= 20
                        and (imm2 // 6) * 6 == base
                        and imm1 != imm2
                    ):
                        found_bases.add(base)
        # Also check literal 'TEXT' (0x54584554) in 1008:4f40 or 'WIND' (0x444e4957) in 1008:2bdc or 0x3e9 in 1010:286c
        if f["entry"] == "1010:286c":
            found_bases.add(0x9C)  # RsLt / 0x03e9
        for b in sorted(found_bases):
            func_res[f["entry"]].append(seg7_table[b])
    return func_res


def classify_all():
    decomp = json.load(open(ROOT / "build/decomp/decomp_index.json"))
    mt = json.load(open(ROOT / "build/method_tables.json"))
    seg7_table = decode_seg7_table()

    funcs = decomp["functions"]
    code_funcs = [f for f in funcs if f["segment"] in ("1000", "1008", "1010", "1018", "1020")]
    func_by_entry = {f["entry"]: f for f in code_funcs}

    func_res = scan_function_resource_types(code_funcs, seg7_table)

    # Map function -> vtables that introduce/override it, and all vtables containing it
    func_defining_tables = defaultdict(list)
    func_all_tables = defaultdict(list)
    table_by_addr = {t["table"]: t for t in mt["tables"]}

    for t in mt["tables"]:
        t_addr = t["table"]
        for slot_hex, fn, prev in t["overrides"]:
            func_defining_tables[fn].append((t_addr, slot_hex))
        for idx, fn in enumerate(t["cells"]):
            func_all_tables[fn].append((t_addr, f"0x{idx*4:x}"))

    # Determine working class name and subsystem for each of the 142 vtables
    KNOWN_CLASS_INFO = {
        "1008:0bc8": ("CObject_1008_0bc8", "Framework/Core Object & Collections", "Root abstract object class (`Subclass Responsibility` base)"),
        "1008:07f4": ("CEventTarget_1008_07f4", "Framework/Event & Command Dispatch", "Base event/command receiver (`+0x1c` event handler)"),
        "1008:0824": ("CCollection_1008_0824", "Framework/Core Object & Collections", "Base collection/array container"),
        "1008:0904": ("CList_1008_0904", "Framework/Core Object & Collections", "Dynamic pointer/element list"),
        "1008:0b08": ("COrderedList_1008_0b08", "Framework/Core Object & Collections", "Ordered/indexed collection"),
        "1008:0a48": ("CObList_1008_0a48", "Framework/Core Object & Collections", "Object pointer collection"),
        "1008:2ece": ("CResourceMap_1008_2ece", "Framework/Resource Manager", "Resource type map base"),
        "1008:2da6": ("CResourceManager_1008_2da6", "Framework/Resource Manager", "Global Resource Manager (`DAT_1060:0a14`) loading from `SB_CDRES.DLL`"),
        "1010:2b12": ("CResourceTypeList_1010_2b12", "Framework/Resource Manager", "Per-FourCC resource collection (`RsLt` entries)"),
        "1008:2e32": ("CApplication_1008_2e32", "Framework/Application & Startup", "Root application controller (`DAT_1060:07c4`)"),
        "1008:0b84": ("CEventDispatcherBase_1008_0b84", "Framework/Event & Command Dispatch", "Base Win16 message/key event translator (`1010:3d56`)"),
        "1008:7f84": ("CEventDispatcher_1008_7f84", "Framework/Event & Command Dispatch", "Application event dispatcher (`root+0x0e`)"),
        "1008:7edc": ("CSoundManager_1008_7edc", "Framework/Audio & WaveOut", "Global sound/speech manager (`DAT_1060:07c8`, loads `snd ` BSW resources)"),
        "1010:bdd4": ("CSoundChannel_1010_bdd4", "Framework/Audio & WaveOut", "Audio channel/sequence player (`snd ` BSW loader at `1020:2928`)"),
        "1008:8278": ("CPaletteManager_1008_8278", "Framework/Graphics & Palette", "Global palette/CLUT manager (`DAT_1060:0198`, loads `clut` `0x03ef`)"),
        "1008:6448": ("CPictSurface_1008_6448", "Framework/Graphics & Offscreen", "Offscreen bitmap/PICT surface (`1008:5804` loads `PICT` `0x03f3`)"),
        "1008:b310": ("CLayeredOffscreen_1008_b310", "Framework/Graphics & Offscreen", "Layered offscreen renderer buffer (`itsLayeredOffscreen`)"),
        "1008:1aac": ("CView_1008_1aac", "Framework/UI Views & Windows", "Base hierarchical UI View (`CView`: `Pane` resource init, child list, mouse hit-test, paint)"),
        "1008:7d10": ("CDesktop_1008_7d10", "Framework/UI Views & Windows", "Root desktop view (`CDesktop` / `SpellingDemonsDesktop` at `DAT_1060:0810`)"),
        "1010:5942": ("CWindow_1010_5942", "Framework/UI Views & Windows", "Native Win16 window wrapper (`CWindow`, `REGISTERCLASS`/`CREATEWINDOW`, `WndProc` `1010:5db2`)"),
        "1008:6f82": ("CSpellingDemonsWindow_1008_6f82", "Framework/UI Views & Windows", "Main game window wrapper (`SpellingDemonsWindow`)"),
        "1008:97e6": ("CGameControllerView_1008_97e6", "Game/Controller & HUD", "Main game HUD/controller view (`1018:267e` event switch, `1018:284c` Pane thresholds)"),
        "1018:3228": ("CMainMenuOrMatchView_1018_3228", "Game/Menus & Flow", "Game screen/menu view (`1018:509c` `CHEAT` input, loads `CPic`, `GmBn`, `MpRv`, `Pane`, `Yobi`)"),
        "1008:2b04": ("CPlayerRecord_1008_2b04", "Game/Player & High Scores", "Player profile & top-7 high score record object (`DAT_1060:01e4` / `DAT_1060:0a46`)"),
        "1010:cc76": ("CSpellingMatch_1010_cc76", "Game/Spelling & Words", "Spelling word/level loader (`theMatch`, loads `GmTr` `0x0406`, `GmLt` `0x0407`, `SdOb` `0x0403`)"),
        "1010:be64": ("CBoardScene_1010_be64", "Game/Board & Level", "Game board/scene controller (`1010:6ef4`, loads `FrTr` `0x040c` & `SdTr`)"),
        "1020:52ee": ("CDemonGridController_1020_52ee", "Game/Board & Level", "Spelling Demon object grid owner (`1010:cb02`, loads `SdGd` `0x0405`)"),
        "1010:92ae": ("CDemonActor_1010_92ae", "Game/Actors & AI", "Board actor/demon object (`1010:8558`, `1010:8e06`, loads `SdOb` `0x0403`)"),
        "1010:90be": ("CActorBehavior_1010_90be", "Game/Actors & AI", "Actor behavior/movement controller (`1018:d0fc`, loads `SdOb` `0x0403`)"),
        "1010:bc7c": ("CDialogOrRollView_1010_bc7c", "Game/Menus & Flow", "Quote/Symbol roll view (`1020:62fe` loads `RlQt`, `1020:6494` loads `RlSy`)"),
    }

    # Build vtable classification
    vtable_info = {}
    for t in mt["tables"]:
        t_addr = t["table"]
        if t_addr in KNOWN_CLASS_INFO:
            cname, subsys, role = KNOWN_CLASS_INFO[t_addr]
        else:
            # Infer from resource usage in its overrides or from parent hierarchy
            res_used = []
            for _, fn, _ in t["overrides"]:
                for r in func_res.get(fn, []):
                    res_used.append(r["mac_fourcc"])
            parents = t["parents"]
            seg = t_addr.split(":")[0]
            addr_tag = t_addr.replace(":", "_")
            if "1008:1aac" in parents:
                cname = f"CViewSub_{addr_tag}"
                subsys = "Game/UI Views & Dialogs" if seg in ("1018", "1020") else "Framework/UI Views & Controls"
                role = f"`CView` subclass ({t['slots']} slots)"
            elif "1010:92ae" in parents or "1010:6262" in parents:
                cname = f"CGameEntity_{addr_tag}"
                subsys = "Game/Actors & AI"
                role = f"Game entity/actor class ({t['slots']} slots)"
            elif "1008:0824" in parents:
                cname = f"CListSub_{addr_tag}"
                subsys = "Framework/Core Object & Collections"
                role = f"Collection subclass ({t['slots']} slots)"
            elif "1008:07f4" in parents:
                cname = f"CControllerSub_{addr_tag}"
                subsys = "Game/Controller & HUD" if seg in ("1018", "1020") else "Framework/Event & Command Dispatch"
                role = f"Event-target subclass ({t['slots']} slots)"
            else:
                cname = f"CClass_{addr_tag}"
                subsys = "Game/Logic & State" if seg in ("1018", "1020") else "Framework/Core Object & Collections"
                role = f"Class with {t['slots']} virtual slots"
            if res_used:
                role += f"; loads resources: {', '.join(sorted(set(res_used)))}"
        vtable_info[t_addr] = {
            "table": t_addr,
            "class_name": cname,
            "subsystem": subsys,
            "role": role,
            "slots": t["slots"],
            "parents": t["parents"],
        }

    # Classify all 1,422 code functions into Bucket (A: CRT, B: Framework, C: Game) & Subsystem
    AUDIO_APIS = {"WAVEOUTGETNUMDEVS", "WAVEOUTGETDEVCAPS", "WAVEOUTOPEN", "WAVEOUTPREPAREHEADER", "WAVEOUTWRITE", "WAVEOUTRESET", "WAVEOUTUNPREPAREHEADER", "WAVEOUTCLOSE", "TIMEGETTIME"}
    GDI_APIS = {"BITBLT", "STRETCHBLT", "TEXTOUT", "EXTTEXTOUT", "CREATEBITMAP", "CREATECOMPATIBLEBITMAP", "CREATECOMPATIBLEDC", "GETBITMAPBITS", "SETBITMAPBITS", "CREATEPALETTE", "SELECTPALETTE", "REALIZEPALETTE", "ANIMATEPALETTE", "GETSYSTEMPALETTEENTRIES", "SETPALETTEENTRIES", "SELECTOBJECT", "DELETEOBJECT", "DELETEDC", "CREATEFONTINDIRECT", "CREATESOLIDBRUSH", "CREATEPENINDIRECT", "CREATERECTRGNINDIRECT", "COMBINERGN", "FILLRECT", "FRAMERECT", "INVERTRECT", "PATBLT", "SETBKCOLOR", "SETBKMODE", "SETTEXTCOLOR"}
    USER_APIS = {"PEEKMESSAGE", "GETMESSAGE", "TRANSLATEMESSAGE", "DISPATCHMESSAGE", "POSTMESSAGE", "SENDMESSAGE", "REGISTERCLASS", "CREATEWINDOW", "DESTROYWINDOW", "SHOWWINDOW", "UPDATEWINDOW", "MOVEWINDOW", "BEGINPAINT", "ENDPAINT", "INVALIDATERECT", "VALIDATERECT", "GETDC", "RELEASEDC", "GETKEYSTATE", "GETASYNCKEYSTATE", "SETCURSOR", "LOADCURSOR", "SETFOCUS", "GETFOCUS", "CLIENTTOSCREEN", "SCREENTOCLIENT", "DEFWINDOWPROC", "MESSAGEBOX"}
    RES_APIS = {"FINDRESOURCE", "LOADRESOURCE", "LOCKRESOURCE", "FREERESOURCE", "SIZEOFRESOURCE", "LOADLIBRARY", "FREELIBRARY"}

    func_classifications = {}
    for f in code_funcs:
        ep = f["entry"]
        seg = f["segment"]
        off = int(ep.split(":")[1], 16)
        imps = set(f["imports"])
        res_list = func_res.get(ep, [])
        owners = func_defining_tables.get(ep, [])

        owner_class = ""
        owner_slot = ""
        if owners:
            t_addr, slot_hex = owners[0]
            owner_class = vtable_info[t_addr]["class_name"]
            owner_slot = slot_hex

        # Determine Bucket & Subsystem
        if seg == "1000":
            bucket = "A: CRT & Startup"
            subsys = "CRT/Startup & DOS-Win16 Runtime"
        elif imps & AUDIO_APIS or (0x3170 <= off <= 0x3A50 and seg == "1010") or (0x22C0 <= off <= 0x26A0 and seg == "1008"):
            bucket = "B: Bright Star Framework"
            subsys = "Framework/Audio & WaveOut"
        elif imps & RES_APIS or (0x0FE0 <= off <= 0x13E0 and seg == "1010") or (0x26A0 <= off <= 0x2B80 and seg == "1010") or (0x9080 <= off <= 0x9520 and seg == "1018"):
            bucket = "B: Bright Star Framework"
            subsys = "Framework/Resource Manager"
        elif owners:
            t_addr = owners[0][0]
            subsys = vtable_info[t_addr]["subsystem"]
            bucket = "C: Game Logic" if subsys.startswith("Game/") else "B: Bright Star Framework"
        elif res_list:
            subsys = res_list[0]["subsystem"]
            if subsys.startswith("Audio"):
                subsys = "Framework/Audio & WaveOut"
                bucket = "B: Bright Star Framework"
            elif subsys.startswith("Graphics"):
                subsys = "Framework/Graphics & Offscreen"
                bucket = "B: Bright Star Framework"
            elif subsys.startswith("UI"):
                subsys = "Framework/UI Views & Windows"
                bucket = "B: Bright Star Framework"
            else:
                bucket = "C: Game Logic"
        elif imps & GDI_APIS:
            bucket = "B: Bright Star Framework"
            subsys = "Framework/Graphics & Offscreen"
        elif imps & USER_APIS:
            bucket = "B: Bright Star Framework"
            subsys = "Framework/UI Views & Windows"
        elif seg in ("1008", "1010"):
            # Segment 1008 and 1010 contain both framework and some game scenes
            if seg == "1010" and off >= 0x6200:
                bucket = "C: Game Logic"
                subsys = "Game/Board & Actors"
            else:
                bucket = "B: Bright Star Framework"
                subsys = "Framework/Core & UI Helpers"
        else:
            # Segments 1018 and 1020
            bucket = "C: Game Logic"
            if seg == "1018" and 0xA600 <= off <= 0xAF60:
                subsys = "Game/Player & High Scores"
            elif seg == "1018" and 0x2600 <= off <= 0x6000:
                subsys = "Game/Controller & Menus"
            elif seg == "1020" and 0x1000 <= off <= 0x2B00:
                subsys = "Game/Spelling & Words"
            else:
                subsys = "Game/Logic & State"

        func_classifications[ep] = {
            "entry": ep,
            "name": f["name"],
            "size": f["size"],
            "purge": f["purge"],
            "bucket": bucket,
            "subsystem": subsys,
            "owner_class": owner_class,
            "owner_slot": owner_slot,
            "resources": [r["mac_fourcc"] for r in res_list],
            "imports": sorted(imps),
        }

    return seg7_table, vtable_info, func_classifications, func_res


def write_outputs():
    seg7_table, vtable_info, func_classifications, func_res = classify_all()

    # 1. Write JSON
    out_json = {
        "resource_descriptor_table_seg7": list(seg7_table.values()),
        "vtables": list(vtable_info.values()),
        "functions": list(func_classifications.values()),
    }
    with open(ROOT / "build/subsystem_map.json", "w") as fp:
        json.dump(out_json, fp, indent=2)

    # 2. Enrich docs/reconstruction-status.csv
    rows = []
    comment_header = ""
    with open(ROOT / "docs/reconstruction-status.csv", "r") as fp:
        raw_lines = fp.readlines()
        if raw_lines and raw_lines[0].startswith("#"):
            comment_header = raw_lines[0]
            raw_lines = raw_lines[1:]
        reader = csv.DictReader(raw_lines)
        for row in reader:
            ep = row["entry"]
            fc = func_classifications.get(ep, {})
            row["bucket"] = fc.get("bucket", "")
            row["subsystem"] = fc.get("subsystem", "")
            row["owner_class"] = fc.get("owner_class", "")
            row["owner_slot"] = fc.get("owner_slot", "")
            row["resources"] = ";".join(fc.get("resources", []))
            if row.get("scope") == "untriaged" and fc.get("subsystem"):
                row["scope"] = fc["subsystem"]
            rows.append(row)

    fieldnames = [
        "entry",
        "ghidra_name",
        "body_bytes",
        "status",
        "scope",
        "bucket",
        "subsystem",
        "owner_class",
        "owner_slot",
        "resources",
        "source",
        "notes",
    ]
    with open(ROOT / "docs/reconstruction-status.csv", "w", newline="") as fp:
        if comment_header:
            fp.write(comment_header)
        writer = csv.DictWriter(fp, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    # 3. Write docs/subsystem-map.md
    bucket_counts = defaultdict(lambda: [0, 0])
    subsys_counts = defaultdict(lambda: [0, 0])
    for fc in func_classifications.values():
        bucket_counts[fc["bucket"]][0] += 1
        bucket_counts[fc["bucket"]][1] += fc["size"]
        subsys_counts[(fc["bucket"], fc["subsystem"])][0] += 1
        subsys_counts[(fc["bucket"], fc["subsystem"])][1] += fc["size"]

    lines = [
        "# Subsystem Map & Resource Rosetta Stone",
        "",
        "Generated by `tools/map_subsystems.py` from `original/BLIZZARD.EXE`, `build/decomp/decomp_index.json`, and `build/method_tables.json`.",
        "",
        "## 1. High-Level Partition (The 3 Buckets)",
        "",
        "| Bucket | Functions | Code Bytes | % of Code Bytes | Strategy |",
        "|---|---:|---:|---:|---|",
    ]
    total_bytes = sum(fc["size"] for fc in func_classifications.values())
    for b in sorted(bucket_counts.keys()):
        cnt, bts = bucket_counts[b]
        pct = 100.0 * bts / total_bytes
        strat = (
            "Replace/bridge with standard C runtime & Win16 shim"
            if b.startswith("A:")
            else (
                "Decompile class-by-class against Win16 GDI/USER/MMSYSTEM/Resource shim"
                if b.startswith("B:")
                else "Decompile game classes, board/actor logic, and spelling rules to clean C/C++"
            )
        )
        lines.append(f"| **{b}** | {cnt} | {bts:,} | {pct:.1f}% | {strat} |")
    lines.append(f"| **Total** | **{len(func_classifications)}** | **{total_bytes:,}** | **100.0%** | |")
    lines.append("")

    lines.extend([
        "## 2. Detailed Subsystem Breakdown",
        "",
        "| Bucket | Subsystem | Functions | Code Bytes |",
        "|---|---|---:|---:|",
    ])
    for (b, s), (cnt, bts) in sorted(subsys_counts.items()):
        lines.append(f"| {b} | `{s}` | {cnt} | {bts:,} |")
    lines.append("")

    lines.extend([
        "## 3. Segment 7 Resource Descriptor Rosetta Stone (`0x36c80`)",
        "",
        "Bright Star's cross-platform Mac/Win16 engine embeds a 37-entry table in Segment 7 (`0x00..0xd8`, 6 bytes per entry) that maps 4-character Macintosh resource tags (`FourCC`) to 16-bit Windows NE resource type IDs (`0x03e9..0x040c`, stored in `SB_CDRES.DLL` as `0x83e9..0x840c`).",
        "",
        "| Seg 7 Offset | Mac FourCC | Win16 ID | NE Resource Type | Subsystem | Meaning / Role |",
        "|---|---|---|---|---|---|",
    ])
    for off, info in sorted(seg7_table.items()):
        lines.append(
            f"| `{info['seg7_offset']}` | `{info['mac_fourcc']}` | `{info['win16_type_id']}` (`0x{info['win16_type_id']:04x}`) | `{info['win16_rt_hex']}` | `{info['subsystem']}` | {info['description']} |"
        )
    lines.append("")

    lines.extend([
        "## 4. Direct Resource-Loading Functions & Class Anchors",
        "",
        "Every function that passes a Segment 7 resource descriptor to `1010:10f2` (`ResMgr_GetResource`) or `1018:908c` (`ResObject_InitFromResource`):",
        "",
        "| Function | Owning Class (Vtable) | Slot | Loaded Resource FourCCs | Subsystem |",
        "|---|---|---|---|---|",
    ])
    for ep in sorted(func_res.keys()):
        fc = func_classifications[ep]
        r_tags = ", ".join(f"`{r['mac_fourcc']}` (`0x{r['win16_type_id']:04x}`)" for r in func_res[ep])
        owner = f"`{fc['owner_class']}`" if fc["owner_class"] else "*(helper / non-virtual)*"
        slot = f"`+{fc['owner_slot']}`" if fc["owner_slot"] else "—"
        lines.append(f"| `{ep}` | {owner} | {slot} | {r_tags} | `{fc['subsystem']}` |")
    lines.append("")

    lines.extend([
        "## 5. Major Anchored Classes (from 142 Vtables)",
        "",
        "| Vtable | Working Class Name | Slots | Parent Chain | Subsystem | Role |",
        "|---|---|---:|---|---|---|",
    ])
    for t_addr, vt in sorted(vtable_info.items()):
        if t_addr in (
            "1008:0bc8", "1008:07f4", "1008:0824", "1008:0904", "1008:0b08", "1008:0a48",
            "1008:2da6", "1010:2b12", "1008:2e32", "1008:0b84", "1008:7f84", "1008:7edc",
            "1010:bdd4", "1008:8278", "1008:6448", "1008:b310", "1008:1aac", "1008:7d10",
            "1010:5942", "1008:6f82", "1008:97e6", "1018:3228", "1008:2b04", "1010:cc76",
            "1010:be64", "1020:52ee", "1010:92ae", "1010:90be", "1010:bc7c",
        ):
            p_str = " -> ".join(f"`{p}`" for p in vt["parents"]) if vt["parents"] else "*(root)*"
            lines.append(
                f"| `{t_addr}` | `{vt['class_name']}` | {vt['slots']} | {p_str} | `{vt['subsystem']}` | {vt['role']} |"
            )
    lines.append("")

    with open(ROOT / "docs/subsystem-map.md", "w") as fp:
        fp.write("\n".join(lines))

    print(f"Wrote build/subsystem_map.json, docs/subsystem-map.md, and updated docs/reconstruction-status.csv")


if __name__ == "__main__":
    write_outputs()
