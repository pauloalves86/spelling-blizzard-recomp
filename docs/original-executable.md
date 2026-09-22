# Original Executable

## Canonical file

Filename: `BLIZZARD.EXE`
Path: `original/BLIZZARD.EXE`
Size: 229,472 bytes
MD5: `9689e835fb4d553b062c87345c1fafa7`
SHA-1: `74ced33706f70089f70a58b68ec03cc698f184cb`
SHA-256: `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`
Filesystem access/modify time: `1994-07-19 01:10:00 -0300`
Filesystem change/birth times are workspace-copy metadata and are not treated as original release metadata.

## Identification facts

- `file` identifies the file as: `MS-DOS executable, NE version 5 for MS Windows 3.10 (EXE) (GUI)`.
- The embedded VERSIONINFO resource identifies the product as `Spelling Blizzard`, version `1.10.CD for Windows`.
- VERSIONINFO `InternalName`: `BLIZZARD`.
- VERSIONINFO `OriginalFilename`: `BLIZZARD.EXE`.
- VERSIONINFO company: `Bright Star Technology, Inc., a wholly-owned subsidiary of Sierra On-Line, Inc.`.
- Copyright resource text: `1994 by Bright Star Technology, Inc.`.
- The resource comment says the product was formerly called `Advanced Spelling Tricks`.

These resource/version strings are direct binary evidence. They establish the analyzed file as the Windows CD build identified by its own version resource; they do not establish that it is the only historical release.

## Format and execution model

- Outer compatibility signature: MZ.
- Real executable format: NE (New Executable), signature at file offset `0x80`.
- NE version: `5.50` (`ne_ver=0x05`, `ne_rev=0x32`).
- Target: 16-bit Windows 3.x protected-mode application. Ghidra's automatic loader selected `New Executable (NE)` and language `x86:LE:16:Protected Mode:default`.
- This is not a DOS extender executable such as DOS/4GW, and it is not a 32-bit PE/LE/LX executable.
- The MZ stub prints `This program requires Microsoft Windows.` and exits through DOS `INT 21h`; that stub is not the application's main execution path.
- There is no evidence from Ghidra's recognized instructions of BIOS `INT 10h`, BIOS keyboard `INT 16h`, or BIOS time `INT 1Ah` application calls. The NE code does contain `INT 21h` instructions and uses the Windows KERNEL `DOS3CALL` interface, which is normal for a Win16 program that reaches DOS services through Windows.

## MZ header facts

The MZ header at offset zero contains:

- `e_magic=0x5a4d` (`MZ`)
- `e_cblp=0x0199`, `e_cp=1`: the DOS image occupies one 512-byte page with 409 bytes used in the final page (consistent with the small stub/image header area).
- `e_crlc=0`: no MZ relocations in the stub.
- `e_cparhdr=4`: MZ header size is 64 bytes.
- `e_minalloc=0`, `e_maxalloc=0xffff`.
- `e_ss=0`, `e_sp=0x00b8`, `e_cs=0`, `e_ip=0`: stub entry is at the beginning of the post-header stub code.
- `e_lfarlc=0x40`: conventional relocation-table offset, unused here because `e_crlc=0`.
- `e_lfanew=0x80`: file offset of the NE header.

The bytes at MZ stub offset `0x40` are DOS code which prints the Windows-only message using `INT 21h/AH=09h`, then exits using `INT 21h/AH=4Ch`.

## NE header and segments

The NE header is at file offset `0x80`. Important decoded fields:

- Entry table offset: `0x183` relative to the NE header; entry table size: `9` bytes.
- Flags: `0x0302`.
- Automatic data segment: segment `13` (`0x000d`).
- Initial heap: `0x1000`; initial stack: `0x2000`.
- Initial CS:IP: segment `1`, offset `0x001a` (`1000:001a` in Ghidra's segmented address notation).
- Initial SS:SP: segment `13`, offset `0` (`100d:0000` conceptually; Ghidra maps the data segment as `1028` because of its load model).
- Code segments: 5; data segments: 8; total NE segment count: 13.
- Nonresident-name table offset: `0x20c` relative to the NE header.
- Segment alignment shift: `4`, so segment-table file offsets are in 16-byte paragraphs.
- No overlays were indicated by the NE header fields inspected; the executable is a single NE image with resident code/data/resources.

Ghidra loaded the initialized code segments as:

- `1000:0000-23e6`, 9,191 bytes
- `1008:0000-efd9`, 61,402 bytes
- `1010:0000-efe9`, 61,418 bytes
- `1018:0000-ef7a`, 61,307 bytes
- `1020:0000-6fe9`, 28,650 bytes

Ghidra also identified initialized/uninitialized data segments and seven resource blocks. The five code segments total 161,968 bytes as loaded; the remainder is data, resources, headers, and alignment.

## Resources and embedded data

Direct NE resource-table parsing found resource types:

- type `0x800e`, 1 resource, 32 bytes
- type `0x8006`, 1 resource, 208 bytes
- type `0x8010`, 1 resource, 624 bytes
- type `0x8003`, 1 resource, 752 bytes
- type `0x83e8`, 2 resources, 16 and 352 bytes
- type `0x83f0`, 1 resource, 32 bytes

The type IDs are numeric/custom or standard Win16 resource identifiers; exact semantic names should be confirmed in a later resource-focused pass. One resource is clearly `VS_VERSION_INFO`, and another contains the embedded textual comment. The executable therefore is not purely code: it contains Windows resources and version metadata.

## External files and dependencies

Strings name the following external files or file-like objects:

- `BRGHTSTR.INI`
- `SB_CDRES.DLL`
- `_DEMONS_.TMP`

The imported module names are `KERNEL`, `GDI`, `USER`, `KEYBOARD`, `MMSYSTEM`, and `SHELL`. Ghidra resolved Win16 export names from its symbol databases, but the corresponding DLL files were not present in the project library search path. This is expected for a Windows system executable and is not evidence that these DLLs are embedded in `BLIZZARD.EXE`.

## Immutability

`original/BLIZZARD.EXE` was only read. No executable bytes were changed. The Ghidra project is the `SpellingBlizzard` project stored at `ghidra/SpellingBlizzard.gpr` with its database in `ghidra/SpellingBlizzard.rep/`, separate from `original/`.
