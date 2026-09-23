# Event and Callback Dispatch (Step 12)

## Scope and method

This is a static ownership/dispatch trace, not a behavioral claim. The canonical input was `original/BLIZZARD.EXE` (SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`). Analysis used Ghidra 12.1.3 and the existing `ghidra/SpellingBlizzard.gpr` with `-readOnly -noanalysis`:

```text
/home/pmfa/Projects/ghidra_12.1.3_PUBLIC/support/analyzeHeadless \
  /home/pmfa/Projects/spelling-blizzard-recomp/ghidra SpellingBlizzard \
  -readOnly -noanalysis -process BLIZZARD.EXE \
  -postScript Step12Verify.java \
  -scriptPath /home/pmfa/Projects/spelling-blizzard-recomp/ghidra \
  -log /home/pmfa/.hermes/cache/scratch/step12verify-final.log \
  > /home/pmfa/.hermes/cache/scratch/step12verify-final.out 2>&1
```

The script `ghidra/Step12Verify.java` enumerates Ghidra references, decodes selected raw 16:16 address runs, and scans initialized program memory for exact far-pointer byte sequences. It does not modify the executable or Ghidra database. The successful captured output/log are `/home/pmfa/.hermes/cache/scratch/step12verify-final.out` and `/home/pmfa/.hermes/cache/scratch/step12verify-final.log` (workspace-local; not repository artifacts).

Confidence labels:

- Fact: instruction bytes, decoded call form, direct reference, or exact 16:16 pointer occurrence.
- Strong inference: a repeated four-byte sequence of executable far addresses is table-like, but no object use/owner is established.
- Unresolved: runtime table target, object owner, initializer, or edge not statically linked.

## Executive result

No complete producer → receiver → dispatch → `1018:267e` path is proven. The best-supported endpoint for `1020:4e8e` is `1020:4f10`, an indirect far call through the runtime receiver's table slot `+0x1c`. Its target remains unresolved. `1018:267e` independently consumes event `0x2714`; a matching event value does not establish that it receives this call.

A wider raw-memory scan found table-like far-pointer runs containing `1018:267e`, `1018:509c`, `1020:4e8e`, and `1020:2534`. These improve the static inventory but do not establish table ownership or initialization: Ghidra has no references to the candidate run bases or the pointer cells, and no literal far pointers to the candidate bases were found.

## Callback/table inventory

| Context or candidate run | Slot | Target | Initialization / owner | Dispatch site | Status and confidence |
|---|---:|---|---|---|---|
| Final replacement-object table `1008:2b04` | `+0x00` | `1018:aa84` | Installed on the recovered replacement-object path; object layout and lifecycle documented in Step 11 | Replacement cleanup path | Fact; established in prior reconstruction |
| Final replacement-object table `1008:2b04` | `+0x1c` | `1008:e8b6` (`RETF 8` no-op) | Same table | `1018:5496` sends `0x271b` through the receiver's `+0x1c` slot on the known replacement-object path | Fact for that object/path; not the `1020:4e8e` table |
| Raw run beginning `1008:97c2` | `+0x40` | `1018:267e` | No table-base reference, constructor write, or owner recovered | No dispatch to this entry proven | Exact pointer occurrence is a Fact; interpreting the run as a table is a Strong inference; owner unresolved |
| Raw run beginning `1018:3230` | `+0x18` | `1018:509c` | No table-base reference, constructor write, or owner recovered | No dispatch to this entry proven | Exact pointer occurrence is a Fact; run is Strongly table-like; owner unresolved |
| Raw run beginning `1018:3230` | `+0x12c` | `1020:4e8e` | Same unresolved run/owner as above | No direct code caller; possible table-entry use is unproven | Exact pointer occurrence is a Fact; run is Strongly table-like; owner unresolved |
| Raw run beginning `1008:7f80` | `+0x3c` | `1020:2534` | No table-base reference or owner recovered | `1020:2534` directly calls `1010:3d56` at `1020:2548` | Exact pointer occurrence is a Fact; run is Strongly table-like; owner unresolved |
| Receiver table used by `1020:4e8e` | `+0x1c` | Runtime far target unknown | Receiver's `+0x00` table pointer is loaded at runtime; no initializer or receiver origin recovered | `1020:4f10`, for events `0x2714` and `0x2718` | Indirect call is a Fact; target and owner unresolved |
| First-parameter receiver used by `1018:509c` | `+0x1c` | Runtime far target unknown | Receiver table pointer is loaded from its first word; owning object/constructor unknown | `1018:5343` on the `CHEAT` path, emitting `0x2711` | Call form is a Fact; target and owner unresolved |
| First-parameter receiver used by `1010:3d56` | `+0x40` | Runtime far target unknown | Receiver/table origin unresolved | `1010:3eb0`, `1010:3f37` | Call form is a Fact; target and owner unresolved |
| Runtime far context `DAT_1060:01ec` | `+0x118`, `+0x11c`, `+0x124`, `+0x128` | Runtime targets unknown | A raw instruction sequence stores an incoming receiver into `01ec` at `1020:5f3f`; no allocation/type owner recovered | Readers include `1018:284c`, `1018:5496`, `1008:75b2`, `1008:16ce`, `1018:9f90`, and `1020:4e8e` | Global use and call offsets are Facts; concrete targets unresolved |
| Receiver used by `1020:678c` | `+0x128`, `+0x148` | Runtime targets unknown | This function's receiver is used directly; this is not shown to be `DAT_1060:01ec` | `1020:67ad`, `1020:67df`, `1020:6816`, later `+0x128` calls | Call forms are Facts; identity with `01ec` context unresolved |

The candidate run at `1018:3230` also contains `1008:b82e` at `+0x1c`. If a receiver's table were this run, that cell—not `1018:267e`—would be reached by a `+0x1c` dispatch. The required receiver-to-run link is absent, and `1008:b82e` has no Ghidra function boundary in this analysis; this is conditional raw-table evidence, not a recovered route.

## Producer and consumer edges

### `1020:4e8e` producer

Facts from the function body:

- ABI is `RETF 8`; receiver far pointer is loaded from `[BP+6]:[BP+8]`.
- `1020:4ea4–4eb4` reads receiver `+0x4a`, follows `+0x46`, and reads word `+0x68`.
- If the word is `8`, `1020:4ec5` pushes literal `0x2714`; `1020:4f06–4f10` pushes the receiver and calls `CALLF [BX+0x1c]`.
- If the word is `7`, it drains `DAT_1060:01ec +0x118`, then dispatches `0x2718` through the same receiver slot.
- Ghidra reports no direct code callers for `1020:4e8e`. A single raw far-pointer occurrence is at `1018:335c`, `+0x12c` in the run at `1018:3230`; this does not identify the runtime receiver.

Boundary: `1020:4f10` is the earliest proven event-dispatch boundary. The concrete `+0x1c` target, receiver origin, and table initializer are unknown.

### `1018:267e` consumer

`1018:267e` is a `RETF 8` function and has a `0x2714` branch. It dispatches through its own receiver's `+0x118`, `+0x114`, or `+0x120` slots depending on the state path. Ghidra reports no direct callers. The only exact raw far-pointer occurrence is `1008:9802`, at `+0x40` in the run beginning `1008:97c2`; neither that cell nor the candidate base has a Ghidra reference or recovered initializer.

The candidate raw cell is at `+0x40`, whereas `1020:4e8e` calls `+0x1c`. More importantly, the `1020:4e8e` call visibly pushes a far receiver plus one event word (six argument bytes), while `1018:267e` returns with `RETF 8`. That makes a direct call from this site to `1018:267e` unsupported and ABI-inconsistent absent an unobserved adapter or extra argument. No such adapter is established. The two functions remain separate anchors.

### `1018:5496` and the repeated `+0x1c` offset

`1018:5496` uses `+0x1c` to send `0x271b` through its receiver. On the known replacement-object path, that receiver's final table is `1008:2b04`, whose `+0x1c` entry is `1008:e8b6`, a `RETF 8` no-op. This does not identify the runtime target of `1020:4e8e`'s `+0x1c`, nor the first-parameter receiver slot used by `1018:509c`. The common offset alone is not evidence of a common owner or dispatch mechanism.

## `1018:509c` ownership boundary

Ghidra reports `RETF 12`, no direct code references, and no direct Ghidra references to the raw pointer cell. The raw run at `1018:3230` contains one exact far-pointer occurrence to it at `+0x18`; the cell's owner and use remain unknown.

The first parameter's fields have more specific structural evidence:

- `+0x52`, `+0x62`, `+0x66`, and `+0x6a` are loaded as 32-bit far pointers. The function null-tests them, follows the referenced object's first-word table pointer, and calls that table's `+0x118` slot (for example `1018:5104–5130`, `1018:514c–5175`, `1018:518a–5193`, and `1018:51a6–51cd`). These fields are structurally far object references, not proven naked code pointers.
- `+0x5e` is another far object pointer; its referenced table is dispatched through `+0x124` and `+0x140` around `1018:5399–53c4`.
- `+0x1c` is a slot in the first parameter's table, used at `1018:5343` to emit event `0x2711` on the `CHEAT` path.

The first-parameter constructor/allocation origin, the identity of its table, and the dispatch path into `1018:509c` remain unresolved.

## `1010:3d56` message/input path

- The sole direct Ghidra caller of `1010:3d56` is `1020:2534` at `1020:2548`. The wrapper forwards two far-pointer parameters and has no direct Ghidra callers.
- A raw far-pointer occurrence for `1020:2534` is at `1008:7fbc`, `+0x3c` in the separate run at `1008:7f80`. No object/table owner or initializer for that run is known.
- `1010:3d56` loads its first parameter's table pointer and calls slot `+0x40` at `1010:3eb0` and `1010:3f37`, passing the receiver far pointer and a word from its second parameter. Concrete table and target are unresolved.
- The call sites pass six argument bytes; `1018:509c` has `RETF 12`. There is no direct static edge from these `+0x40` calls to `1018:509c` and their observed stack cleanup does not support a direct target match.

The chain therefore stops at the wrapper's unreferenced entry and the runtime receiver's `+0x40` callback. A Win16 message-origin → `1020:2534` path and any later route to `1018:509c` are not proven. `1008:e412` is used within `1018:509c`, but that use does not connect it to `1010:3d56`.

## `DAT_1060:01ec` lifecycle and slot uses

The analyzed image bytes at `DAT_1060:01ec` are zero. A fresh Ghidra reference query lists 47 reads but no Ghidra-recognized write because the relevant code bytes are not defined as instructions. Direct byte inspection of the initialized `Code5` block at `1020:5f20` recovers this sequence despite the absent function/instruction boundary:

```text
1020:5f29  PUSH 1
1020:5f2b  LES BX,[BP+6]
1020:5f2e–5f2f  push receiver segment:offset
1020:5f30  LES BX,ES:[BX]
1020:5f33  CALLF [BX+0x11c]
1020:5f38  MOV EAX,[BP+6]
1020:5f3c  MOV DX,[BP+8]
1020:5f3f  MOV dword ptr [0x1ec],EAX
1020:5f43  MOV dword ptr [0x1f0],0
1020:5f50  MOV byte ptr ES:[BX+0x15a],1
1020:5f58  RETF 0x10
```

This is a Fact about the raw bytes and their bounded instruction decoding. It proves a runtime store of the incoming far receiver into `01ec` after invoking that receiver's `+0x11c(1)` slot. The function entry/caller and object's allocation/type are not established. The image's initial zero is not proof that the pointer is later cleared; no later explicit clear of `01ec` was found in the direct-reference scan. Its complete lifetime remains unresolved.

Static users load the far receiver from `01ec`, load its first-word table pointer, then dispatch at observed offsets. Key examples:

- `1020:4e8e`: `+0x118` polling loop before `0x2718`; this is a call through the global context.
- `1018:284c`: `+0x124`, conditional `+0x128`/`+0x12c`, and `+0x118` drain loop.
- `1018:5496`: `+0x124` and `+0x128` on its nonzero-control path.
- `1008:75b2`, `1008:16ce`, and `1018:9f90`: additional `+0x118`, `+0x124`, `+0x128`, `+0x12c`, `+0x138`, or `+0x110` uses.

These call sites prove slot offsets and runtime use, not concrete targets. In particular, `1018:267e`'s `+0x118` call is through its event receiver, not proven to be the global `01ec` object. `1020:678c` acts on its own receiver, writes that receiver to `01f0` at `1020:692b`, and clears its `+0x15a` byte at `1020:6934`; it does not establish a clear or reassignment of `01ec`.

## Unresolved boundaries and reconstruction decision

- The object/table owner and initialization sites for the three raw pointer runs (`1008:97c2`, `1018:3230`, `1008:7f80`) are unknown; Ghidra references to each base are zero and the scanned image contains no literal far pointer to each candidate base.
- The `1020:4e8e → 1018:267e` edge is not proven; the receiver's runtime table entry at `+0x1c` is unresolved.
- The constructor and caller of `1018:509c` are unknown. Its far object fields are characterized structurally, but their owners and targets are not.
- `1010:3d56`'s message-origin caller and `+0x40` callback target are unknown; no path to `1018:509c` is established.
- `DAT_1060:01ec` has a raw runtime store and many reads, but no recovered allocation, concrete table target, or complete clear/lifetime path.

No native object-model layout or callback target was added, and no tests were added: the new table-like runs have no proven owner/initializer, so a native table or event-routing test would encode a hypothesis. Existing `SBFarPtr16` layouts remain unchanged.
