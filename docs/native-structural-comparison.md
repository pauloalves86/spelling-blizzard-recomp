# Step 11 binary-to-native structural comparison

Input: `original/BLIZZARD.EXE`

SHA-256: `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`

Method: focused read-only Ghidra inspection using the existing project with
`-readOnly -noanalysis`, cross-checked with prior Phase 8-10 reports. Native
layout assertions and tests are in `object_model.h` and
`tests/test_recovered_object_model.c`.

## Comparison table

| Binary anchor | Binary observation | Native preservation | Status |
|---|---|---|---|
| `1018:aeaa` | `RETF 4`; parent `+250` far load, holder `+00` far load, AX from final-nested `+06` | two explicit `SBFarPtr16` fields; raw `uint16_t` return; no null check | matches observed accesses |
| `1018:ae3a` | `RETF 6`; signed existing/candidate comparison; parent byte `+254=1`; calls `a704` after write | signed `int16_t` comparison, byte write before `a704` | matches known control flow |
| `1018:a704` | `RETF 8`; destination at `BP+6/+8`, source `BP+a/+c`; `SHL index,3`; signed value ordering; count `+38`; cap 7 | two `SBFarPtr16` arguments; seven explicit 8-byte slots; `int16_t` compare/count; shift 6 down to rank; cap 7 | matches valid-object path |
| `1018:adb8` | `RETF 8`; low byte of flag selects `+40`/`-20`; 16-bit wrapped unchecked offset; signed clamp; low ADD/high ADC accumulation | `uint16_t` flag/index; low-byte branch; modulo-16-bit offset; explicit 16-bit sign extension and modulo 32-bit accumulation | matches observed arithmetic |
| `1008:2b04` | far pointers stored offset then segment; known entries through `+70` | `SBFarPtr16` and `SBRecoveredFinalMethodTable`, with raw values through `+70` | matches observed table window |
| `1008:e8b6`, `1008:e838` | `CA 08 00` / `RETF 8`, no operation | explicit native no-op bodies; original far ABI recorded in comments | body matches; host ABI intentionally not claimed |
| `1008:ac80` replacement | old first-slot call before allocation; allocation `0x262`; final table written before global storage | `SBRecoveredParentObject` size assert `0x262`; hook-visible old-slot/replacement/final-table ordering | bounded lifecycle seam |
| `1018:aa84` release | final table `+00`; tests/clears parent `+254`; nested cleanup and continuation | represented as known table value and lifecycle boundary only | no speculative native cleanup implementation |

## Exact layout checks

Compile-time assertions verify:

```text
sizeof(SBFarPtr16)                    == 0x04
sizeof(SBRecoveredNestedSlot)         == 0x08
offsetof(slot.word_06)                == 0x06
offsetof(nested.count_38)             == 0x38
offsetof(nested.bytes_19a)            == 0x19a
offsetof(nested.field_1a4)            == 0x1a4
offsetof(nested.word_1a8)             == 0x1a8
sizeof(SBRecoveredNestedObject)       == 0x1aa
offsetof(parent.far_250)              == 0x250
offsetof(parent.byte_254)             == 0x254
offsetof(parent.unknown_256)          == 0x256
sizeof(SBRecoveredParentObject)       == 0x262
```

The `#pragma pack(push, 2)` scope deliberately matches the observed Win16
record alignment. No host pointer appears in the canonical layout.

## Native boundary intentionally retained

A modern host cannot natively reproduce segmented Win16 `CALLF`, `RETF imm`,
or unbounded far-memory dereferences. The native code therefore preserves
stored far-pointer representation and routes dereferences through an explicit
resolver. This is not a Win16 compatibility layer.

The resolver does not validate null, segment, index, or range. For `adb8`, the
selected address is calculated modulo 2^16 before resolution; any host fixture
that needs index-underflow/overflow behavior must map the corresponding
segment bytes. This keeps the binary's unchecked precondition visible instead
of replacing it with bounded array access.

`1018:aa84`, `1010:01d0`, `1008:e812`, allocation, and framework callbacks are
not reimplemented as host behavior. Their proven ordering is documented and
represented by injectable lifecycle boundaries; ownership/refcount/allocation
semantics remain unresolved.
