# Nested object behavior

## Scope and confidence

This document records the instruction-backed behavior of the nested record reached through the parent far pointer at `+0x250/+0x252`. It uses neutral names. No source-level domain identity is asserted.

Confidence labels:

- Fact: direct instructions, verified reference/caller data, or raw code bytes.
- Strong inference: conclusion supported by several direct facts.
- Hypothesis: intentionally not used as a replacement for evidence.

## Layout

| Offset | Width | Access pattern | Evidence | Confidence |
|---:|---:|---|---|---|
| `+0x00` | 6-byte slot text area | NUL-terminated text copied/compared | `1018:ab1c`, `1018:a704` | Fact for accesses; text capacity precondition unresolved |
| `+0x06` | 16-bit signed word | initialized, read, conditional max update, entry value | `1018:ab1c`, `1018:a704`, `1018:ae3a`, `1018:aeaa` | Fact |
| `+0x08` | 8-byte slot 1 | second physical entry begins here | `1018:a704` stride-8 addressing | Fact |
| `+0x00..+0x37` | seven inline 8-byte slots | physical repeated-entry store | `1018:a704` | Fact |
| `+0x38` | 16-bit signed word | active entry count | `1018:a704` | Fact |
| `+0x19a` | byte range | flags cleared/read/set | `1018:ab1c`, `1018:ae7e`, `1018:aec0` | Fact |
| `+0x1a4/+0x1a6` | 32-bit value | initialized to `1000`; adjusted by `1018:adb8` | `1018:ab1c`, `1018:adb8` | Fact for storage/arithmetic; semantic meaning unresolved |
| `+0x1a8` | 16-bit word | written by `1018:aede` | `1018:aede` | Fact |

The prior shorthand “repeated entries begin at `+0x08`” was imprecise: `+0x00` is slot 0 and `+0x08` is slot 1. Slot 0's word at `+0x06` is both the value returned by `1018:aeaa` and the first entry's value.

## 1018:ab1c initialization fallback

Fact: `1018:a9a0` uses `1018:ab1c` when the receiver method at `+0x18` returns zero. `1018:ab1c` calls `1010:035c` with `0x1aa`, stores the returned far pointer at parent `+0x250/+0x252`, copies its text parameter to nested slot-0 text, and initializes slot-0 `+0x06` to zero.

It also clears the first observed entry-value range, initializes later observed entry values to `10`, clears a ten-byte range starting at `+0x19a`, and writes `1000` at `+0x1a4`. The function then invokes parent methods and clears the parent byte at `+0x254`.

`1010:035c` is a wrapper that calls `1010:023e(param_1, 1)`. It is the immediate acquisition boundary, but the exact allocation size and meaning of `0x1aa` remain unresolved.

## 1018:a704 — ordered nested-entry update

### ABI and callers

Fact: `1018:a704` is `__stdcall16far` and ends with `RETF 0x8`, consuming two far-parent parameters.

`1018:ae3a` is its direct caller. It passes `DAT_1060:0a46` as destination and its own parent parameter as source. Before the call, `1018:ae3a` conditionally updates the source slot-0 `+0x06` word and sets source parent `+0x254`.

Known callers of `1018:ae3a` include `1018:267e` and `1018:509c`. This establishes a shared nested-record mutation path, not a domain name.

### Behavior

Fact: `1018:a704` captures source slot-0 text and signed word value, then:

1. scans destination active slots `[0, count)` for text equality;
2. removes only the first matching text entry, shifting later active slots left and decrementing `count`;
3. computes an insertion rank by counting destination entries whose signed value is strictly greater than the source value;
4. if rank is below seven, shifts physical slots down from slot 6 through the rank, inserts the captured text/value, increments count, and clamps count to seven;
5. sets destination parent byte `+0x254` to `1` even when no insertion occurs.

The result is descending signed-value order. For equal values, the new entry is inserted before existing equal-valued entries. Duplicate text is not globally eliminated: at most the first matching active entry is removed per invocation.

The right-shift always uses physical slot 6 as the high source boundary, even when the active count is lower. Thus all seven physical slots are part of the mutation surface, while only `[0,count)` is logically active.

### Pseudocode

```text
capture source.entry[0].text and source.entry[0].value
remove first destination entry with equal text, if any
rank = count(destination entries with value > source value)
if rank < 7:
    shift physical entries 6..rank+1 right
    write source text/value at rank
    count = min(count + 1, 7)
destination.parent[+0x254] = 1
```

## +0x06 behavioral contract

Fact:

- `1018:ab1c` initializes slot-0 `+0x06` to zero.
- `1018:aeaa` returns it without side effects.
- `1018:ae3a` replaces it only when a candidate is larger.
- `1018:a704` copies the source record's `+0x06` into the destination ordered-entry store and compares it against destination entry values.
- `1008:ac80` derives `DAT_1060:01de` as the result of `1018:aeaa` plus one.

Strong inference: `+0x06` is a slot-0 signed value that can be monotonically raised by `1018:ae3a` and is subsequently transferred into a bounded ordered-entry record. It is not the record count (`+0x38`) and is not used as an array index by `1018:a704`.

No instruction proves that `+0x06` is a domain-level aggregate, score, level, or identifier.

Phase 10 adds the complete immediate caller contract: `1018:ae3a` is `__stdcall16far`, `RETF 6`, takes a far source parent plus signed 16-bit candidate, stores the candidate to slot-0 `+0x06` only when the existing signed value is smaller, always writes source parent `+0x254 = 1`, then always calls `1018:a704(DAT_1060:0a46, source)`. `1018:267e` supplies `DAT_1060:01de`; the observed `1018:509c` construction path supplies literal `99`.

## Nested release path

Fact: `1018:aa84` tests parent `+0x254`, not `+0x95`. When nonzero, it invokes parent slots `+0x20`, `+0x54`, and `+0x10`, with framework calls `1010:00e8` and `1010:0122`, then clears `+0x254`.

Regardless of that condition, it passes parent `+0x250/+0x252` to `1010:01d0` and calls `1008:e812`. `1008:e812` invokes its receiver's `+0x10` slot and calls `1008:ef8e`.

`1018:aa84` is `__stdcall16far` with one far parent argument and `RETF 4`. When `+0x254` is nonzero, it calls parent slots `+0x20`, `+0x54`, and `+0x10`, with `1010:00e8` and `1010:0122` on the nested pointer, then clears `+0x254`. It then always calls `1010:01d0(parent +0x250/+0x252)` and `1008:e812(parent)`.

Phase 9 confirms the lifecycle connection for objects produced by the known constructors: final table `1008:2b04` slot `+0x00` is `1018:aa84`, and both the `0x2af9` and observed `1018:509c` construction paths install that table before storing `DAT_1060:01e4`. The next replacement calls the old object's slot 0. `1010:01d0` performs nested cleanup, and `1008:e812` reaches `1000:2032(parent)` through slot `+0x10` and `1008:ef8e`.

## Final method table and event boundary

The final replacement object table is `1008:2b04`.

| Slot | Target | Observed role | Confidence |
|---:|---|---|---|
| `+0x1c` | `1008:e8b6` | three-byte `RETF 8` no-op override | Fact |
| `+0x20` | `1008:cf6c` | selects one of three pointers by selector and stores at receiver `+0x24c` | Fact |
| `+0x44` | `1008:cfc2` | returns `0:0` for null `+0x24c`; otherwise obtains and returns a far pointer | Fact; conditional acquisition is a strong inference |
| `+0x54` | `1008:d0f2` | conditionally forwards data from a far input through receiver `+0x24c`; `RETF 8` | Fact for behavior; role unresolved |
| `+0x68` | `1008:e838` | three-byte `RETF 8` no-op override | Fact |

Raw code at `1008:e8b6` is `CA 08 00`: `RETF 8`. At the `1018:5496` dispatch site, the call's four 16-bit argument words include receiver far pointer, event `0x271b`, and zero. The override removes the arguments without reads, calls, or state writes. Therefore, for final-table receivers, `0x271b` is deliberately absorbed at this method boundary.

## DAT_1060:01ec callback boundary

Fact: `1018:5496` reads `DAT_1060:01ec` and dispatches through its `+0x124` once and `+0x128` three times in the nonzero-control path.

Fact: raw executable instructions at `1020:5f29..5f58` invoke the current object's `+0x11c` slot with `1`, copy that far object from stack `BP+6/BP+8` into `DAT_1060:01ec`, clear `DAT_1060:01f0`, and set object byte `+0x15a` to one. Later `1020:678c` stores its receiver into `DAT_1060:01f0` and clears receiver `+0x15a` after `+0x128/+0x148` dispatches.

The callback object's analyzed image value is zero, so its concrete `+0x124/+0x128` runtime targets are still unavailable statically. It is a separate runtime-populated callback context from the nested object stored at replacement `+0x250`; static evidence does not show it is populated during the `0x2af9` replacement path.

## Remaining uncertainties

- Exact allocation/ownership semantics of `1010:023e`.
- The method bodies at `1008:cfc2` and `1008:d0f2` beyond their bounded field/call behavior.
- Runtime identities of `DAT_1060:01ec` callback targets.
- Source-level semantic names for text, values, flags, and accumulated fields.
