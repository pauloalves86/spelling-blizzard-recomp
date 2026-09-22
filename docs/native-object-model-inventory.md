# Step 11 native object-model inventory

## Scope

This inventory consolidates native representations of the evidence-backed
replacement-parent, holder, final nested object, globals, and final method
pointer table. It does not assign source-level or game-domain identities.

## Canonical native definitions

`src/recovered/application/object_model.h` is the single canonical layout
header. It defines:

- `SBFarPtr16`: a four-byte offset/segment value in executable order;
- `SBRecoveredNestedSlot`: eight bytes, with six observed NUL-scanned bytes
  and a signed word at slot-relative `+0x06`;
- `SBRecoveredNestedObject`: the final nested object after the holder
  indirection, size `0x1aa`;
- `SBRecoveredNestedHolder`: a four-byte far pointer at holder `+0x00`;
- `SBRecoveredParentObject`: the `0x262`-byte replacement object;
- `SBRecoveredGlobals`: one neutral image covering the directly observed
  `DAT_1060` offsets;
- `SBRecoveredFinalMethodTable`: raw 16:16 method-table entries at
  `1008:2b04` through the observed `+0x70` entry.

`SBRecoveredMemory16` is a host-only resolver seam. It resolves a supplied
`SBFarPtr16` for a test fixture while all model fields and function parameters
remain explicit far pointers.

## Known structural offsets

| Structure | Offset | Width | Native member | Evidence | Confidence |
|---|---:|---:|---|---|---|
| final nested object | `+0x00..+0x37` | 56 | `slots_00[7]` | `1018:a704` uses stride 8 | Fact |
| slot | `+0x00` | 6 | `bytes_00` | NUL-scan/copy/compare | Fact for access |
| slot | `+0x06` | signed 16-bit | `word_06` | `aeaa`, `ae3a`, `a704` | Fact |
| final nested object | `+0x38` | signed 16-bit | `count_38` | `a704` active count | Fact |
| final nested object | `+0x19a..+0x1a3` | 10 | `bytes_19a` | observed flag-byte range | Fact for range |
| final nested object | `+0x1a4` | 32-bit | `field_1a4` | `adb8` low/high arithmetic | Fact |
| final nested object | `+0x1a8` | 16-bit | `word_1a8` | `aede`, `af08` | Fact |
| holder | `+0x00` | far pointer | `far_00` | `aeaa` pointer chain | Fact |
| parent | `+0x00` | far pointer | `far_000` | old-object first-slot call/table dispatch | Fact |
| parent | `+0x250` | far pointer | `far_250` | `aeaa`, `a704`, `aa84` | Fact |
| parent | `+0x254` | byte | `byte_254` | mutation writes / `aa84` test+clear | Fact |
| parent | `+0x256..+0x261` | 12 | `unknown_256` | preserve object extent | Fact for range; unknown use |
| globals | `01de` | 16-bit | `word_01de` | direct references | Fact |
| globals | `01e4` | far pointer | `far_01e4` | known replacement pointer | Fact |
| globals | `01ec` | far pointer | `far_01ec` | runtime callback context | Fact |
| globals | `0a36` | byte | `byte_0a36` | state-machine paths | Fact |
| globals | `0a46` | far pointer | `far_0a46` | `ae3a` destination | Fact |

The final nested object's slot 0 begins at `+0x00`; `+0x08` is slot 1. The
parent's `+0x250` points to a holder, whose `+0x00` points to this final nested
object. It is not a direct parent-to-slot pointer.

## Consolidated accessors and mutations

| Function | Native function | Preserved fact |
|---|---|---|
| `1018:aeaa` | `sb_recovered_1018_aeaa` | `RETF 4`; parent → holder → final nested object; returns raw AX bits from signed word `+0x06` |
| `1018:ae3a` | `sb_recovered_1018_ae3a` | `RETF 6`; signed conditional maximum, source `+0x254=1`, then `a704` |
| `1018:a704` | `sb_recovered_1018_a704` | `RETF 8`; fixed seven physical slots, signed descending insertion, first-match removal, count cap, destination `+0x254=1` |
| `1018:adb8` | `sb_recovered_1018_adb8` | `RETF 8`; modulo-16-bit unchecked selected offset, low-byte flag, signed clamp, signed delta addition, parent byte write |
| `1008:ad57..ad60` | `sb_recovered_1008_ac80_0x2af9_state_update` | `aeaa`, 16-bit increment, `01de` write, `0a36` clear |

The original `__stdcall16far` ABI is recorded beside each native interface.
Native host C cannot emit Win16 `RETF imm`; therefore a resolver seam is used
for executable-layout testing rather than claiming host calls are ABI-compatible.

## Final method table

The raw table is represented by `sb_recovered_final_method_table_1008_2b04`.
Instruction-backed entries include:

| Slot | Far target | Observed fact |
|---:|---|---|
| `+0x00` | `1018:aa84` | known release-path entry |
| `+0x10` | `1008:ce88` | raw table target |
| `+0x1c` | `1008:e8b6` | `RETF 8` no-op |
| `+0x20` | `1008:cf6c` | bounded observed field behavior |
| `+0x44` | `1008:cfc2` | `RETF 4`; conditional far result |
| `+0x54` | `1008:d0f2` | `RETF 8`; bounded forwarding behavior |
| `+0x5c` | `1008:e812` | release continuation |
| `+0x68` | `1008:e838` | `RETF 8` no-op |

The release entry is `+0x00`, not `+0x1c`.

## Former duplication removed

The old `nested_field` and `nested_entries` headers defined incompatible
host-pointer parent/holder graphs. They now only include `object_model.h`, and
the old C source locations no longer contain active implementations. No
canonical model field is a host pointer.

## Automated structural coverage

`tests/test_recovered_object_model.c` verifies structure sizes/offsets, far
pointer order, `aeaa`, `ae3a` → `a704` ordering effects, signed `adb8` clamp
and accumulation behavior, and hook-visible replacement ordering. The existing
message-pump test remains separate because it is not part of this object model.
