# Phase 10 application state machine

## Scope

This is the first instruction-backed application-state model around the replacement object and ordered record. It deliberately uses mechanical labels. It does not establish a game-domain interpretation for values, text, events, or globals.

## Confirmed state variables

| Location | Observed operations | Limits |
|---|---|---|
| `DAT_1060:01e4` | replacement object pointer; constructed by `0x2af9` and the `1018:509c` `CHEAT`-comparison path; released through final-table slot 0 on later replacement | object identity is unresolved |
| `DAT_1060:01de` | derived from nested slot-0 `+0x06 + 1`; incremented/decremented by `1018:267e`; one-sided signed clamp of 100 in `1018:438e`; used as unchecked indexes/selectors | 16-bit increment/decrement wrap is confirmed; no global range invariant is proven |
| `DAT_1060:0a36` | byte tested and written by `1018:267e`; cleared by `0x2af9` and the `1018:509c` construction path | role unresolved |
| parent `+0x254` | written by nested mutation helpers; conditionally consumed and cleared by `1018:aa84` before nested release | no direct runtime causal call from `1018:a704` to `1018:aa84` is proven |

## Event 0x2714

`1018:267e` is `__stdcall16far`, has `RETF 8`, receives a far receiver and a 16-bit event/command word with an additional word argument. Its event switch handles `0x2711`, `0x2712`, `0x2714`, `0x2718`, `0x2719`, and `0x271b` among others.

Confirmed `0x2714` behavior:

```text
if DAT_1060:0a36 == 0:
    DAT_1060:0a36 = 1
    receiver virtual slot +0x118(receiver)
    return

1018:ae3a(DAT_1060:01e4, DAT_1060:01de)
if DAT_1060:0a52 != 0:
    1018:ae7e(DAT_1060:01e4, signed(DAT_1060:01de) / 10 - 1)
DAT_1060:0a36 = 0
DAT_1060:01de++              // 16-bit wrap
if DAT_1060:01de == 0x66:
    receiver virtual slot +0x120(receiver)
else:
    receiver virtual slot +0x114(receiver, 1)
```

The first `0x2714` occurrence therefore sets the byte and leaves through receiver slot `+0x118`; a later occurrence while the byte is nonzero performs the ordered-record transition. The event is delivered to `1018:267e` through an unresolved callback/table boundary.

A concrete event emitter is known: `1020:4e8e` reads a word through receiver `+0x4a -> +0x46 -> +0x68`. If it is `8`, it pushes `0x2714` and invokes the receiver's virtual slot `+0x1c`. If it is `7`, it first drains a loop through `DAT_1060:01ec` slot `+0x118`, then sends `0x2718` through the same receiver slot. This is a confirmed producer of the `0x2714` literal, but the target table relationship to `1018:267e` is not established.

## ae3a and ordered-record vertical slice

`1018:ae3a` is `__stdcall16far`, takes a far source parent and a 16-bit signed candidate, and ends in `RETF 6`.

```text
nested = source +0x250 -> +0x00
if nested.slot0.value_06 < candidate (signed):
    nested.slot0.value_06 = candidate
source +0x254 = 1
1018:a704(DAT_1060:0a46, source)
```

It always sets source `+0x254` and always calls `1018:a704`; only the slot-0 write is conditional. `1018:267e` supplies current `DAT_1060:01de` as candidate. The `1018:509c` construction path supplies literal `99`.

This establishes the first complete internal vertical slice:

```text
0x2714 with DAT_1060:0a36 != 0
  -> 1018:ae3a(global_01e4, global_01de)
  -> conditional slot-0 signed max update
  -> 1018:a704(global_0a46, global_01e4)
  -> ordered seven-slot destination update
  -> destination +0x254 = 1
  -> optional ae7e source mutation
  -> clear 0a36; increment 01de; receiver callback
```

The destination update-control byte belongs to `DAT_1060:0a46`, while `ae3a` writes the source object's byte. Static evidence does not prove either write causes an immediate `1018:aa84` call. `aa84` consumes its receiver's byte during that receiver's later final-table release path.

## DAT_1060:01de direct-reference graph

The current Ghidra database contains 22 direct references in eight functions:

| Address | Operation | Condition / use |
|---|---|---|
| `1008:ad5d` | write `aeaa(01e4) + 1` | command `0x2af9` |
| `1018:273a` | `INC` | `0x2714` active-byte path; wraps 16-bit |
| `1018:27c0` | `DEC` | `0x2719` when `0a36 == 0`; wraps 16-bit |
| `1018:43a1` | write `100` | only if signed word exceeds 100 |
| `1018:5323` | write `aeaa(new 01e4) + 1` | `1018:509c` construction path |
| `1018:284c` | signed threshold reads | `==1`, `>90`, `>75`, `>50`, gates `af08/aede` |
| `1018:3ee0` | `global - 1`, scaled by four | unchecked per-parent field selection |
| `1020:1a9a` | input to `adb8` | unchecked one-based selector |
| `1020:678c` | callback arguments and equality guard | runtime callback context boundary |

The loop from nested slot 0 to `01de`, then from `01de` to `ae3a` and `a704`, is confirmed. A separate `01de -> adb8` path mutates a nested word sequence and accumulated field. Direct instructions do not prove that either path feeds back to slot-0 `+0x06` after construction.

## Pure nested state primitives

`1018:adb8` is `__stdcall16far`, `RETF 8`: far parent, flag word, index word. It applies 16-bit offset arithmetic with no bounds check:

```text
word = *(int16_t *)(nested + 0x08 + 2 * (index - 1))
candidate = flag.low == 0 ? word + 40 : word - 20
value = signed clamp(candidate, 1, 200)
store value at selected address
nested[+0x1a4/+0x1a6] += sign_extend(value - word)
parent[+0x254] = 1
```

Index zero and oversized indexes are not rejected; offset arithmetic wraps at 16 bits. Therefore it is not represented as a safe native array helper.

`1018:aede` (`RETF 6`) writes an unmodified word to nested `+0x1a8` and sets parent `+0x254`. `1018:af08` (`RETF 6`) returns `AL=1` exactly when signed argument `<=` signed nested `+0x1a8`; it writes nothing.

`1018:284c` calls `af08` with `1`, `4`, `3`, or `2` after the `01de` thresholds shown above. When false, it performs runtime callback slot `+0x128` and records that same value through `aede`. This creates a coherent threshold/marker relationship for `+0x1a8`, but no connection to ordered count `+0x38` is established.

## 1018:509c input state machine

`1018:509c` is `__stdcall16far` with `RETF 12`. It first tests bit 0 of a byte in its second far argument at `+0x0f`.

| Input state / byte | Confirmed action | Resulting state |
|---|---|---|
| bit `+0x0f` set; `n` or `N` | drain message group 8; if parent `+0x66` exists and globals `01e1/01e2` are zero, call its `+0x118(1)` | external object controls outcome |
| bit set; `P` or `p` | same gating around parent `+0x52`, then target `+0x118(1)` | external object controls outcome |
| bit set; `Q` or `q` | drain group 8; invoke object at parent `+0x6a` through `+0x118(1)` | external object controls outcome |
| bit set; `r` | same gating around parent `+0x62`, then target `+0x118(1)` | external object controls outcome |
| bit clear; `?` or `/` | drain group 8; invoke object at parent `+0x5e` slots `+0x124`, `+0x140` | callback boundary |
| bit clear; Ctrl-C (`0x03`) or Enter (`0x0d`) | drain group 8; use parent `+0x52` through `+0x118(1)` if non-null | callback boundary |
| bit clear; other byte while `0a3c` length < 5 | append byte to `DAT_1060:0a3c` | input buffer state |
| input buffer equals `CHEAT` and `01e4` is null | allocate/initialize `01e4`; `a9a0`; `ae3a(...,99)`; `01de=aeaa()+1`; `0a36=0`; `d0a6`; send `0x2711` through first parameter slot `+0x1c` | replacement-object construction path |
| unmatched/otherwise | call `1008:b802` with the received argument set | unknown external handling |

The literal `CHEAT` comparison and fixed length check are facts. No semantic interpretation beyond the comparison is asserted.

## 0x2af9 comparison

| Property | `0x2af9` in `1008:ac80` | `0x2714` active branch in `1018:267e` |
|---|---|---|
| `DAT_1060:01e4` | releases/replaces and initializes it | reads it as `ae3a` source |
| slot-0 `+0x06` | reads it through `aeaa` after initialization | conditionally raises it through `ae3a` |
| `DAT_1060:01de` | writes `aeaa + 1` | supplies candidate; then increments |
| `DAT_1060:0a36` | clears it | requires nonzero, then clears it |
| ordered slots/count | no direct `a704` call | updates destination through `a704` |
| callback boundary | `d0a6`, `5496`, final `+0x1c` event `0x271b` | receiver `+0x114` or `+0x120` after mutation |
| lifecycle | destroys old known final-table object | no object release observed in the branch |

The two paths share state but no direct call path between their dispatchers is proven.

## Unknown boundaries

- The table/receiver edge that sends `0x2714` to `1018:267e`.
- Domain meaning and normal valid range of `01de`.
- Runtime targets of `DAT_1060:01ec` callback slots.
- A non-destruction consumer of parent `+0x254`; current evidence only shows release-time consumption in `aa84`.
- Any rendering, audio, or external visible consequence of the ordered-record mutation.
