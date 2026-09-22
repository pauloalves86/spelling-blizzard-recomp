# Step 11 semantic debt record

## Rules

This record distinguishes machine-level knowledge from source-level meaning.
A neutral field name is retained unless direct executable evidence establishes
more than storage, arithmetic, or control-flow behavior.

## Nested slot-0 word `+0x06`

Known: signed 16-bit slot-relative word; initialized to zero on the observed
fallback path; returned by `1018:aeaa`; conditionally raised by `1018:ae3a`;
copied by `1018:a704` into ordered slots.

Unknown: source-level meaning.

Evidence: `1018:ab1c`, `1018:aeaa`, `1018:ae3a`, `1018:a704`.

Confidence: Fact for accesses and signed comparisons; unknown semantics.

Next possible investigation: trace all non-slot-0 reads/writes of the word and
compare visible UI consumers without naming it from nearby strings.

## Nested bytes `+0x19a..+0x1a3`

Known: a byte range cleared by initialization and read/written by `ae7e` and
`aec0` paths.

Unknown: individual byte roles and aggregate meaning.

Evidence: instruction-backed accesses in `1018:ab1c`, `1018:ae7e`, `1018:aec0`.

Confidence: Fact for range/accesses.

Next possible investigation: map each byte's direct references and caller
conditions.

## Nested signed 32-bit field `+0x1a4/+0x1a6`

Known: initialized to `1000` in the fallback initialization; `1018:adb8`
adds the sign-extended, 16-bit-wrapped selected-word delta using low-word ADD
and high-word ADC.

Unknown: source-level meaning or valid application range.

Evidence: `1018:ab1c`, `1018:adb8`.

Confidence: Fact for storage and arithmetic.

Next possible investigation: identify non-mutating readers and any rendering or
serialization boundary.

## Nested word `+0x1a8`

Known: `1018:aede` writes an unmodified word; `1018:af08` returns true exactly
when a signed argument is less than or equal to it.

Unknown: source-level meaning.

Evidence: `1018:aede`, `1018:af08`, callers in `1018:284c`.

Confidence: Fact for write/comparison behavior.

Next possible investigation: trace its initializer and all direct callers.

## Parent byte `+0x254`

Known: set to one by `ae3a`, `a704`, `adb8`, and `aede`; tested and cleared by
`1018:aa84` during the known final-table release path.

Unknown: whether it represents a general pending-update condition, and whether
it has a non-release consumer.

Evidence: mutation helpers and `1018:aa84`.

Confidence: Fact for writes/test/clear; strong inference only for an
update-control role.

Next possible investigation: find dynamic/runtime paths from any set to a call
of `aa84` or another reader.

## Parent range `+0x256..+0x261`

Known: it lies inside the observed `0x262` replacement-object allocation and
is reserved by the native layout.

Unknown: all subfield boundaries and meanings.

Evidence: `1008:ef2c(0x262)` and parent-relative initialization references.

Confidence: Fact for preserved extent; unknown contents.

Next possible investigation: targeted field ledger for `1018:a9a0` and all
parent-relative references in this range.

## Seven physical slots and count `+0x38`

Known: seven inline eight-byte physical slots at final-nested `+0x00..+0x37`;
slot 0 begins at `+0x00`, slot 1 at `+0x08`; signed active count at `+0x38` is
kept within `[0,7]` through the recovered `a704` path.

Unknown: source-level identity of slot bytes/words and consumers outside the
known ordered-record update.

Evidence: `1018:a704` indexed addressing and count accesses.

Confidence: Fact.

Next possible investigation: find all final-nested pointer consumers and their
count/slot accesses.

## `DAT_1060:01de`

Known: a 16-bit global initialized as `aeaa(01e4)+1`, incremented/decremented
with 16-bit wrap, used as an `ae3a` candidate and as unchecked selectors.

Unknown: source-level meaning and complete valid range.

Evidence: direct-reference inventory in `1008:ac80`, `1018:267e`, `1018:284c`,
`1018:438e`, and `1020:1a9a`.

Confidence: Fact for data flow.

Next possible investigation: map its origin before construction and each
threshold/callback result.

## `DAT_1060:0a36`

Known: byte set on the first observed `0x2714` path, tested by later event
branches, and cleared by construction/active paths.

Unknown: source-level state identity.

Evidence: `1018:267e`, `1008:ac80`, `1018:509c`.

Confidence: Fact for state-machine operations.

Next possible investigation: recover the callback table that reaches
`1018:267e`.

## `DAT_1060:01ec`

Known: runtime-populated far callback-context pointer; activation and later
completion-context writes are observed.

Unknown: concrete receiver type, table targets, ownership, and lifetime.

Evidence: `1020:5f29..5f58`, `1020:678c`, and dispatches in `1018:5496`.

Confidence: Fact for stores/dispatch offsets; unknown semantic identity.

Next possible investigation: capture runtime table values or trace constructor
registration.

## Rendering/UI consumers

Known: `1018:267e` invokes receiver slots `+0x114`, `+0x118`, or `+0x120` in
specific event branches; `1018:5496` crosses callback/UI boundaries.

Unknown: rendering, audio, visible UI, and input consequences of ordered-record
mutation.

Evidence: direct virtual-call instructions.

Confidence: Fact for call sites; unknown effect.

Next possible investigation: resolve the runtime receiver tables and trace
imports/side effects of their targets.

## Event `0x2714` origin

Known: `1020:4e8e` can send literal `0x2714` through receiver slot `+0x1c`.
`1018:267e` has a confirmed `0x2714` case.

Unknown: static proof that the producer's receiver dispatches to `1018:267e`.

Evidence: direct instructions in both functions.

Confidence: Fact for producer and handler independently; unresolved edge.

Next possible investigation: recover the producer receiver's runtime table.

## Final method table and application behavior

Known: `1008:2b04` has raw far entries; known constructors install it; slot
`+0x00` reaches `1018:aa84` on later replacement for those objects.

Unknown: source-level class identity, complete table extent, and application
behavior of unresolved entries.

Evidence: table bytes, construction stores, and old-object first-slot call.

Confidence: Fact for table values and release edge; unknown semantics.

Next possible investigation: map individual entries from callers and compare
normal/fallback nested initialization paths.
