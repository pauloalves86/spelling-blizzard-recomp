# Phase 9 ordered-record integration

## Scope

This document integrates the instruction-backed ordered seven-slot record behavior with its known application paths and lifecycle boundary. Terms remain mechanical: no game-domain identity is assigned to text, values, `DAT_1060:01de`, or the parent update-control byte.

Confidence labels:

- Confirmed: exact instructions, raw bytes, or direct static caller/data flow.
- Strong inference: supported by multiple confirmed facts.
- Unknown: insufficient static evidence.

## Confirmed layout and ordered update

The nested record reached through parent `+0x250/+0x252` contains seven physical eight-byte slots at `+0x00..+0x37`; each slot has NUL-terminated text at its first six bytes and a signed 16-bit word at slot offset `+0x06`. `+0x38` is the active-slot count.

`1018:a704` is `__stdcall16far` with destination and source far-parent arguments and `RETF 0x8`. Its only direct caller is `1018:ae3a` at `1018:ae72`.

```text
1018:267e event 0x2714             1018:509c input/controller path
        |                                      |
        +---------------> 1018:ae3a <----------+
                              |
                              +-> 1018:a704(DAT_1060:0a46, source parent)
```

`1018:267e:2714` supplies the current `DAT_1060:01de` to `1018:ae3a` with `DAT_1060:01e4` as source. The observed `1018:509c` path constructs/initializes `DAT_1060:01e4`, invokes `1018:ae3a(..., 99)`, then derives `DAT_1060:01de` from `1018:aeaa(...) + 1`.

`1018:a704` removes at most the first active equal-text entry, ranks the captured source slot-0 value against active destination entries using signed comparison, shifts physical slots through slot 6, inserts before equal-valued entries, increments and caps `+0x38` at seven, and writes destination parent `+0x254 = 1`.

## Count and slot consumers

Confirmed: among the focused parent-chain instruction searches, only `1018:a704` reads or writes nested `+0x38`. It scans `[0,count)`, decrements after a first equal-text removal, and increments after insertion before clamping to seven. No observed path makes the count negative. The established invariant for paths through `1018:a704` is:

```text
0 <= nested[+0x38] <= 7
```

The physical shift always addresses slots through slot 6, including inactive storage. `1018:ab1c` initializes the nested allocation and source slot 0; `1018:aeaa` reads slot-0 `+0x06`; `1018:ae3a` conditionally raises that word then enters the ordered update. No additional consumer of the ordered slot store through the `parent +0x250` chain was found by this focused static analysis.

This is not proof that no indirect/runtime consumer exists.

## Relationship with DAT_1060:01de and related fields

Confirmed data flow:

```text
nested slot-0 +0x06
    -> 1018:aeaa
    -> DAT_1060:01de = value + 1      (1008:ac80 / 0x2af9; 1018:509c path)

DAT_1060:01de
    -> 1018:267e event 0x2714
    -> 1018:ae3a
    -> 1018:a704(DAT_1060:0a46, DAT_1060:01e4)
```

`1018:267e` also increments or decrements `DAT_1060:01de` in event branches. `1018:438e` clamps it to 100. In `1020:1a9a`, it is passed as the one-based selector parameter to `1018:adb8`.

`1018:adb8` changes a 16-bit word selected at `nested + 0x08 + 2 * (parameter - 1)` by `+0x28` or `-0x14`, clamps that word to `[1,200]`, adds the signed delta to nested `+0x1a4/+0x1a6`, and marks parent `+0x254`. Direct callers are two sites in `1020:1a9a` and one in `1020:6af6`.

`1018:ab1c` initializes nested `+0x1a4/+0x1a6` to 1000. `1018:aede` writes its third parameter to nested `+0x1a8` and marks `+0x254`; `1018:af08` compares an argument with `+0x1a8`. Their direct observed caller is `1018:284c`.

Strong inference: `DAT_1060:01de` participates in a one-based selection/progression loop tied to the nested data and its accumulated 32-bit field. It is not proven to name a level, score, record number, or other source-level concept.

## Parent +0x254 update-control behavior

Confirmed writers include `1018:a704`, `1018:ae3a`, `1018:adb8`, and `1018:aede`. `1018:a9a0` and `1018:aa84` clear it on their respective paths.

`1018:aa84` reads the byte. If nonzero, it invokes parent slots `+0x20`, `+0x54`, and `+0x10`, with `1010:00e8` and `1010:0122` on the nested far pointer, then clears the byte. It always proceeds to nested release afterward. Thus the byte controls a pre-release flush/update sequence; it is not merely a permanent state marker.

## Confirmed destruction chain

The final table at `1008:2b04` has slot `+0x00 = 1018:aa84`. Both known constructors—the `1008:ac80` `0x2af9` branch and the observed `1018:509c` path—finish by installing this final table before storing the allocation in `DAT_1060:01e4`.

On replacement, `1008:ac80:ace1..acea` calls the old non-null object's slot 0. Therefore, for objects installed by those known paths:

```text
old DAT_1060:01e4
  -> table 1008:2b04 slot +0x00
  -> 1018:aa84(parent)
     -> conditional +0x254 update sequence
     -> 1010:01d0(parent +0x250/+0x252)
     -> 1008:e812(parent)
        -> parent slot +0x10
        -> 1008:ef8e(parent)
        -> 1000:2032(parent)
```

`1018:aa84` is `__stdcall16far`, takes one far parent argument, and ends in `RETF 4`. It does not clear parent `+0x250/+0x252` before passing that exact pointer to `1010:01d0`.

`1010:01d0` returns immediately only for a null far pointer; otherwise it performs nested cleanup, zeroes the allocation's first `0x0e` bytes, and calls `1008:ef1e`. This establishes release of the parent-held nested allocation before parent cleanup for known final-table objects.

Unknown: an unsupported external runtime writer could install a differently tabled object in `DAT_1060:01e4`; static write inventory found only the two known construction paths.

## Final replacement methods and callback context

- `1008:cfc2` at table `+0x44` is `RETF 4`; when receiver `+0x24c` is non-null it performs framework calls and returns an acquired far pointer. Strong inference: it is the conditional nested-pointer acquisition used by `1018:a9a0`.
- `1008:d0f2` at `+0x54` is `RETF 8`; with non-null receiver context it conditionally forwards information derived from its far input. It is the update-sequence method called by `1018:aa84`, but its source-level purpose remains unknown.
- `1008:e838` at `+0x68` is raw `CA 08 00` / `RETF 8`, a no-op override.

`DAT_1060:01ec` is a separate runtime-populated far callback context. `1020:5f29..5f58` calls its receiver slot `+0x11c` with one, assigns that receiver to `DAT_1060:01ec`, clears `DAT_1060:01f0`, and sets receiver byte `+0x15a`. Later `1020:678c` writes its receiver to `DAT_1060:01f0` and clears `+0x15a` after dispatches through `+0x128` and `+0x148`.

Strong inference: this is an activated callback context with a later completion/current-pointer transition. The object type, allocation origin, concrete runtime callback targets at `+0x124/+0x128`, and ownership remain unknown.

## Remaining boundary

The strongest integrated static chain is:

```text
1018:267e / 1018:509c
  -> 1018:ae3a conditional source slot-0 update
  -> 1018:a704 ordered insertion into DAT_1060:0a46
  -> destination parent +0x254 = 1
  -> 1018:aa84 conditional update sequence when that parent is released
  -> nested allocation cleanup and parent cleanup
```

No static evidence yet closes this into a rendering/UI observation or proves the exact event origin of `1018:267e` event `0x2714`.

## Phase 10 event-source refinement

`1020:4e8e` is a confirmed literal producer of `0x2714`: when a word reached through its receiver `+0x4a -> +0x46 -> +0x68` equals `8`, it pushes `0x2714` and calls that receiver's virtual slot `+0x1c`. The receiver/table relationship to `1018:267e` remains unresolved, so this is an event-emission boundary rather than a complete caller edge.

The `0x2714` active branch in `1018:267e` also has a post-insertion consequence: after `ae3a/a704`, it optionally calls `ae7e`, clears `0a36`, increments `01de`, and invokes receiver slot `+0x114(1)` except at exact value `0x66`, where it invokes `+0x120`.
