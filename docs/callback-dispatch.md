# Phase 4 Callback and Dispatch Analysis

## Target: 1018:509c

### Facts and Step 12 correction

`1018:509c` still has no direct code reference in Ghidra's reference manager. A read-only scan of initialized Ghidra memory finds one exact far-pointer encoding, `9c 50 18 10`, at `1018:3248`. This corrects the Phase 4 statement below that the encoding was absent.

The hit is at `+0x18` in a contiguous, four-byte-stride run beginning at `1018:3230`. The 0x140-byte window decodes as 16:16 addresses into executable segments; `+0x12c` contains `1020:4e8e`. This is strong evidence for a static function-pointer sequence, but not proof of one object's vtable: Ghidra has no references to the base or the two entries, and the initializer and owner remain unknown. The Ghidra listing has no instruction boundary at `1018:3230`; the conclusion is based on raw bytes and target resolution, not an auto-created data definition.

The same scan finds `1018:267e` once, at `1008:9802`, within a separate four-byte-stride sequence beginning at `1008:97c2` (`+0x40`). It also finds `1020:2534` at `1008:7fbc`, within a separate sequence beginning at `1008:7f80` (`+0x3c`). These are candidate static pointer tables/runs; no base address has a Ghidra reference, and raw scans found no literal far pointer to the candidate bases. Do not infer that any candidate run is installed in a runtime object.

The other observed 16-bit values `0x509c` in the `0x86xx` and `0xae8x` ranges are instruction displacement bytes, not function-pointer evidence. The pointer-sequence finding at `1018:3248` supersedes the earlier interpretation of that occurrence as unrelated undefined data.

### Current conclusion

There is now a static far-pointer occurrence of `1018:509c` in a table-like run. Its owning object/table, initialization, and dispatch site are still unresolved. The bytes alone do not establish that this run is the first parameter's method table or that any caller reaches `1018:509c` through it.

## Phase 5 data-flow follow-up

The unresolved `1018:509c` path shares application-state evidence with the `0x2af9` branch of `1008:ac80`: both can initialize/use `DAT_1060:01e4`, and both call `1018:a9a0`, `1018:aeaa`, and `1010:d0a6` in their respective paths. This is a shared-state relationship, not proof that one dispatches to the other.

`DAT_1060:0a3c` remains directly tied to the input accumulation and `CHEAT` comparison in `1018:509c`. Current decompilation does not show `1008:ac80` accessing it.

## Confirmed indirect-dispatch conventions

The executable does contain many object-relative indirect calls. A common form is:

```text
load object vtable pointer
add method-slot offset
load near code pointer
call through pointer
```

Examples:

- `1010:596e` invokes object methods at vtable-relative slots corresponding to `+0x14` and `+0x18`.
- `1010:3d56` invokes an event callback at an object-relative slot corresponding to `+0x40`.
- `1018:509c` invokes its receiver's table slot `+0x1c`. Its first-parameter fields `+0x52`, `+0x62`, `+0x66`, and `+0x6a` are loaded as far object pointers and dispatched through the referenced object's table slot `+0x118`; `+0x5e` is another far object pointer dispatched through slots `+0x124` and `+0x140`.

These establish an indirect-dispatch architecture, but not ownership of the particular `1018:509c` entry or of the table-like run at `1018:3230`.

## Callback/function-table initialization evidence

`1008:2bf4` allocates several objects through `FUN_1008_ef2c` and writes repeated offset/segment pairs such as:

```text
0xbc8:1008
0x824:1008
0x904:1008
0xb08:1008
0x2ece:1008
0x2da6:1008
0x2ff2:1008
0x2f5a:1008
0x3042:1008
0x2e32:1008
```

It stores these object pointers in globals including `DAT_1060_0a14`, `DAT_1060_0a1e`, and `DAT_1060_07c4`, then invokes methods through the resulting tables. This is confirmed function-table/vtable-style initialization, but none of the literal entries identified in this path is `1018:509c`.

`1018:649a` similarly allocates and initializes several nested objects with offset/segment pairs in the `1008` and `1018` segments. This is a strong application/object construction candidate, but it is not yet connected to `1018:509c`.

## Event path currently supported by evidence

```text
unknown message-origin caller
    |
    v
1020:2534  -- forwards two far-pointer parameters
    |           (one raw occurrence at 1008:7f80 + 0x3c;
    |            no inbound Ghidra reference to this wrapper)
    v
1010:3d56  -- direct calls through receiver table +0x40
    |
    +-- concrete receiver table/target unresolved

separate unresolved route:
1018:509c  -- raw pointer occurrence at 1018:3230 + 0x18;
              no caller or owner proven
```

This diagram intentionally stops at the unresolved edge. The binary does not yet support a more specific claim.

## Highest-value next trace

The next useful trace is the constructor or initialization path that installs a pointer to one of the candidate runs (`1008:97c2`, `1018:3230`, or `1008:7f80`) in an object. For `1018:509c`, the first-parameter constructor remains unknown; its observed offsets `+0x52`, `+0x5e`, `+0x62`, `+0x66`, and `+0x6a` now classify as far object references rather than naked callback addresses. Treat the raw runs as candidates until an object write and a matching dispatch site agree.

## Phase 7 callback findings

The replacement object's final method-table pointer is `1008:2b04`. Reading its far entries gives:

| Slot | Entry | Status |
|---:|---|---|
| `+0x1c` | `1008:e8b6` | target address established; Ghidra function boundary unresolved |
| `+0x20` | `1008:cf6c` | function identified; selects a resource/string pointer based on a byte and stores it near object `+0x24c` |
| `+0x44` | `1008:cfc2` | target address established; implementation boundary unresolved |
| `+0x54` | `1008:d0f2` | target address established; implementation boundary unresolved |
| `+0x68` | `1008:e838` | target address established; implementation boundary unresolved |

`1018:a9a0` uses `+0x20` and `+0x44` while establishing `+0x250`; `1018:5496` uses `+0x1c` for event `0x271b`.

The global object at `DAT_1060:01ec` is read at `1018:5496`, `1018:284c`, `1020:1a9a`, `1008:75b2`, `1008:16ce`, and `1018:9f90`; its analyzed image bytes are zero. The Ghidra listing has no instruction boundary in `1020:5f20..5f5a`, but raw bytes there decode to a call through the incoming receiver's `+0x11c` slot with one at `1020:5f33`, followed by a far-pointer store to `01ec` at `1020:5f3f`, a zero store to `01f0` at `1020:5f43`, and a set of receiver byte `+0x15a` at `1020:5f50`. Later `1020:678c` stores its receiver in `DAT_1060:01f0` at `1020:692b` and clears receiver `+0x15a` at `1020:6934` after `+0x128/+0x148` dispatches. This supports an activation/completion-context inference, not ownership or concrete callback identities; no later clear of `01ec` is established.

Phase 10 confines this object to documented boundaries: `1018:284c` calls its `+0x124`, `+0x128`, `+0x12c`, and `+0x118` slots while applying `01de` thresholds, and `1020:4e8e` may drain its `+0x118` loop before emitting `0x2718`. No direct `0x2714` ordered-record consequence through this runtime object is proven.
