# `SB_CDRES.DLL` Resource Library Analysis

## 1. File Identity (Fact)

- Filename: `original/SB_CDRES.DLL`
- Filesystem timestamp: `1994-07-19 01:10:00`
- Size: `9,808,128` bytes (~9.35 MB)
- MD5: `5786039ca3726dc2df52489b9d40a4e0`
- SHA-256: `1c6c36bde51b8c9ad3b3a0eaba0283d0468d81326b3dbe4dbfbf7305863b70e1`
- Format: 16-bit Windows NE executable (DLL)
- Internal Module Name: `SS_CDRES`
- Module Description: `Resource Library Module for CD version`
- Flags: `0x8309` (Library / DLL)
- Alignment shift: 8 (256-byte granularity)

Like `original/BLIZZARD.EXE`, this file is treated as immutable canonical ground truth.

---

## 2. Executable Structure (Fact)

`SB_CDRES.DLL` is a **pure resource-only DLL**:

- **Segment 1**: 452 bytes (code). Minimal Win16 `LibMain` / `WEP` initialization and exit stub.
- **Segment 2**: 84 bytes (initialized data).
- **Exports**: Ordinal 1 is `WEP` (Windows Exit Procedure). No application logic or routines are exported.
- **Payload**: Over 9.33 MB (9,785,088 bytes) consists of the NE Resource Table and embedded resource payloads.

---

## 3. Resource Inventory (Fact)

The DLL contains **1,751 total resources** across **43 distinct resource types**.

The resource types use custom integer IDs ranging from `1001` (`0x83e9`) to `1042` (`0x8412`):

| Resource Type | Integer ID | Count | Total Size | Typical Format / Content |
|---|---|---|---|---|
| `TYPE_0x83ee` | `1006` | **430** | 7,104,256 B (7.10 MB) | **BSW Audio**: Magic `BSW\0`, 11,025 Hz 8-bit mono speech & sound |
| `TYPE_0x83f3` | `1011` | **242** | 2,196,736 B (2.20 MB) | **Animation / Sprites**: Sprite frame sequences & visual assets |
| `TYPE_0x8403` | `1027` | **346** | 88,576 B (0.09 MB) | **Game Records**: Structured spelling level data & word tables |
| `TYPE_0x8407` | `1031` | **202** | 176,384 B (0.17 MB) | **Record Buffers**: Level definitions and coordinates |
| `TYPE_0x8406` | `1030` | **101** | 51,712 B (0.05 MB) | **Table Records**: Short word lists and indices |
| `TYPE_0x840c` | `1036` | **85** | 30,720 B (0.03 MB) | **State / Map Data**: Tile maps & board layouts |
| `TYPE_0x83fc` | `1020` | **58** | 14,848 B (0.01 MB) | **Control Data**: UI control layout parameters |
| `TYPE_0x8402` | `1026` | **26** | 6,656 B (0.01 MB) | Binary records |
| `TYPE_0x83f5` | `1013` | **24** | 6,144 B (0.01 MB) | Control metadata |
| `TYPE_0x8401` | `1025` | **24** | 6,144 B (0.01 MB) | Binary records |
| `TYPE_0x83ec` | `1004` | **23** | 28,928 B (0.03 MB) | **DIB Bitmaps**: Standard `BITMAPINFOHEADER` (size 40) |
| `TYPE_0x83f1` | `1009` | **20** | 5,120 B (0.01 MB) | Graphic parameters |
| `TYPE_0x83ff` | `1023` | **19** | 9,728 B (0.01 MB) | Coordinate data |
| `TYPE_0x8400` | `1024` | **17** | 4,352 B (0.00 MB) | Coordinate data |
| `TYPE_0x83f7` | `1015` | **16** | 4,096 B (0.00 MB) | Graphic parameters |
| `TYPE_0x83f2` | `1010` | **15** | 3,840 B (0.00 MB) | Graphic parameters |
| `TYPE_0x83f6` | `1014` | **14** | 7,168 B (0.01 MB) | Graphic parameters |
| `RT_VERSION` | `16` | **1** | 512 B (0.00 MB) | Standard VERSIONINFO resource |
| Others (25 types) | various | **66** | 139,776 B (0.13 MB) | Miscellaneous tables, palettes, and configurations |
| **Total** | | **1,751** | **9,785,088 B (9.33 MB)** | |

The complete inventory is exported in `build/sb_cdres_resources.json`.

---

## 4. `BLIZZARD.EXE` Resource Loading Pipeline (Fact)

By contrast, `original/BLIZZARD.EXE` contains only 7 resources (2,016 bytes). Practically all runtime data is loaded dynamically from `SB_CDRES.DLL`:

```text
BLIZZARD.EXE Startup (1008:2bc0)
  -> Push "SB_CDRES.DLL" (1060:0124)
  -> Call KERNEL!LoadLibrary
  -> Save HMODULE into DAT_1060:001c

Resource Loading Dispatch (1008:2bdc, GetResourceModule):
  FUN_1008_2bdc(type):
    if (type == 'WIND') return DAT_1060:07d0;  // BLIZZARD.EXE hInstance
    else return DAT_1060:001c;                  // SB_CDRES.DLL HMODULE

Core Resource Locking (1010:0fe6):
  1. Calls 1008:2bdc to obtain module handle
  2. KERNEL!FindResource(hModule, resId, resType) -> HRSRC at [bx+0x10]
  3. KERNEL!LoadResource(hModule, HRSRC) -> HGLOBAL at [bx+0x0e]
  4. KERNEL!SizeofResource(hModule, HRSRC)
  5. KERNEL!LockResource(HGLOBAL) -> returns far pointer to asset data

High-Level Record Access (1010:10f2):
  1. Obtains global Resource Manager object DAT_1060:0a14
  2. Calls mgr->+0x80(...) to select resource collection
  3. Calls collection->+0x54(id) to fetch individual record object
  4. Returns far pointer to record data at [bx+0x04]
```

---

## 5. Authentic Bright Star C++ Class Identifiers (Fact)

Directly preceding `"SB_CDRES.DLL"` in segment `1060` (data segment 13, at `1060:0100..0180`), the executable stores the real class names for its GUI framework:

- `CView`
- `CWindow`
- `CDesktop`
- `SpellingDemonsWindow`
- `SpellingDemonsDesktop`
- `CEditText`
- `CAMEditText`
- `ColorText`

This confirms that the Bright Star object model uses the `C` prefix naming convention (`CView`, `CWindow`, `CDesktop`), superseding speculative `T` prefix designations.
