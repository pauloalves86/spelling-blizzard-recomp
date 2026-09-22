# Phase 5 First Reconstruction

## Original address

`1018:aeaa`

## Recovered function

`sb_recovered_1018_aeaa` in `src/recovered/application/nested_field.c`.

## Calling convention

FACT: Ghidra identifies `1018:aeaa` as `__stdcall16far` with one `undefined4` parameter and a `undefined2` return value.

FACT: The instruction sequence is:

```text
PUSH BP
MOV BP,SP
LES BX,[BP+0x6]
LES BX,ES:[BX+0x250]
LES BX,ES:[BX]
MOV AX,ES:[BX+0x6]
LEAVE
RETF 0x4
```

The parameter is therefore one 4-byte far pointer at `[BP+0x6]`. The callee removes four parameter bytes with `RETF 0x4`. No general-purpose register is explicitly saved. The result is returned in `AX`.

## Inputs and memory accesses

The function performs no null checks and no platform calls. It follows two pointers:

```text
parent far pointer
    +0x250 -> far pointer to a holder
holder +0x00 -> far pointer to a value object
value +0x06 -> 16-bit result
```

The exact Win16 far-pointer representation is not retained in the native model. The native structures preserve the observed two-level dereference and the offsets that are known.

## Native reconstruction

```c
uint16_t sb_recovered_1018_aeaa(const SBRecoveredParentObject *object)
{
    return object->nested_holder_250->value_at_00->field_06;
}
```

No validation was added because the original function performs none.

## Callers and uses

FACT: Direct callers found by Ghidra are:

- `1008:ac80`, where the result is incremented and stored in `DAT_1060:01de`;
- `1018:3c70`, where the result is clamped to `99`, divided into a loop bound, and used to control repeated UI/object operations;
- `1018:509c`, where it is used in the input/controller path.

STRONG INFERENCE: The value is a bounded application-state quantity used both for initializing `DAT_1060:01de` and for controlling repeated output work. Its semantic identity remains unknown.

## Initialization and mutation evidence

`1018:ae3a` follows the same two-level pointer chain and conditionally writes a 16-bit value at the final object offset `+0x06` when the existing value is smaller than its parameter. Known callers include `1018:509c` and `1018:267e`.

`1008:ac80` calls `1018:a9a0` and then reads the value through `1018:aeaa`; another application path calls `1018:ae3a(..., 99)` before reading it. This establishes a current observed upper-bound operation, not the field's semantic name or complete valid range.

No destructor or allocation site for the final value object has been proven. `DAT_1060:01e4` is a far pointer to an application object that is allocated in `1008:ac80` and other game paths, but its complete nested-object construction path remains unresolved.

## Tests

`tests/test_recovered_1018_aeaa.c` covers zero, one, an arbitrary 16-bit value, `UINT16_MAX`, and independent parent/nested instances.

These are native unit tests of the reconstructed semantics. They are not differential tests against the original executable because no mechanism currently invokes `1018:aeaa` inside the original binary.

## Confidence

High for the instruction-level behavior and calling convention. Medium for the native structure labels because the surrounding object ownership and semantic meaning of `+0x06` remain unresolved.
