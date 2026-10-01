# Indirect Caller of `1008:ac80` (Step 14)

## Question

How is `1008:ac80` reached? Ghidra has no code reference to it (`docs/command-flow.md`, `docs/message-loop-trace.md`).

## Method and inputs

- Input: `original/BLIZZARD.EXE`, SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2` (unchanged; checked by the tool on every run).
- `tools/ne_farptr.py` (Python 3, read-only) parses the NE segment table and per-segment relocation records, and walks every **selector fixup chain**.
- Raw 16-bit disassembly comes from `objdump -D -b binary -m i8086 -M intel` over segment bytes extracted from the file. Segment file offsets: `1008` = `0x2700`, `1010` = `0x11980`, `1018` = `0x20c60`.
- `ghidra/Step14Trace.java` was run with `-readOnly -noanalysis`. It gives the containing function, references, and decompilation for listed addresses; it does not change the project:

```text
<GHIDRA_HOME>/support/analyzeHeadless ghidra SpellingBlizzard -readOnly -noanalysis \
  -process BLIZZARD.EXE -scriptPath ghidra -postScript Step14Trace.java +1010:10f2 1008:ac80
```

### Why earlier pointer scans missed these edges

**Fact.** All five code segments are moveable (flags `0x0d50`). Every far pointer between code segments, and from a segment to itself, is relocated by a non-additive NE selector fixup chain (relocation source type 2, internal reference). In the file, the selector word of each pointer holds the offset of the **next chain link**, not a segment value.

Example: `1018:6160` is the bytes `80 ac 66 61`. Its selector word `0x6166` is a chain link, and the fixup chain resolves it to segment `1008`.

A raw scan for `<offset> <0x10xx selector>` therefore finds none of these table cells or `mov word [x+2], seg` immediates. That explains the "no literal pointer / no installation constant" results in `docs/object-table-ownership.md`. Use `tools/ne_farptr.py cells|refs|install` instead.

Reproduce the core facts:

```text
python3 tools/ne_farptr.py refs 1008:ac80      # far-pointer cell 1018:6160 -> 1008:ac80
python3 tools/ne_farptr.py cells 1018:6144 72  # table B
python3 tools/ne_farptr.py install 1018:6144   # immediate at 1018:56de
python3 tools/ne_farptr.py refs 1018:553a      # far-pointer cell 1018:3244
python3 tools/ne_farptr.py install 1018:3228   # immediate at 1018:2c6d (1020:318a is a false hit inside a far call)
python3 tools/ne_farptr.py refs 1018:2bf0      # far-pointer cell 1008:9902
python3 tools/ne_farptr.py install 1008:97e6   # immediate at 1008:932e
python3 tools/ne_farptr.py install 1008:b0c8   # 15 sites, including 1008:a76c/a811/a8ad/a949
```

## 1. `1008:ac80` is slot `+0x1c` of table `1018:6144`

**Fact.** The only far-pointer reference to `1008:ac80` in the image is the cell `1018:6160`. It belongs to a contiguous run of fixup-resolved cells. That run starts at `1018:6144` (= `1010:16da`, followed by the shared `1008:ef9e`, `1008:efa4`, `1008:b746`, …) and continues through `1018:6258`.

The run has the same shape as the neighbouring tables `1018:6034`, `1018:3228`, `1018:684e`, and `1008:97e6`. Within table `1018:6144`:

| Slot | Target | Notes |
|---|---|---|
| `+0x1c` | `1008:ac80` | event handler (below) |
| `+0x20` | `1008:add8` | |
| `+0x30` | `1010:45ea` | shared creation/initialization entry |
| `+0x34` | `1008:a656` | class-specific initializer |
| `+0x110` | `1008:aa42` | slot used by `1008:ac80` for `0x32c9` |
| `+0x114` | `1008:ab54` | slot used by `1008:ac80` for `0x32ca` |

**Fact: `+0x1c` is an event-handler slot.** `1008:ac80`, `1018:553a`, `1018:267e` and `1018:66ee` all have the same raw shape:

```text
mov ax,[bp+0xa]; mov dx,[bp+0xc]      ; 32-bit event
compare/switch on event
otherwise: call 1008:b7de(self, event)
RETF 8                                ; receiver far pointer + dword event
```

`1008:b7de`, the handler used where tables do not override `+0x1c` (for example `1018:684e`, `1008:b0c8`), forwards the event to the object stored at `self+0x04`:

```text
1008:b7de: push dword [bp+0xa]; push dword es:[si+4]; les bx,es:[si+4]; les bx,es:[bx]; call far es:[bx+0x1c]
```

**Fact.** `1008:b6fa` writes `self+0x04` from its second argument. For objects created through `+0x30` = `1010:45ea`, the argument order is preserved through:

```text
45ea -> +0x34 -> (1010:15f0 directly, or 1018:8cae -> 1010:15f0) -> 1010:4640 -> 1008:b6fa
```

So `self+0x04` is the first non-receiver argument passed to `+0x30`. For each creation site below, that argument is the creating (parent) object. **Strong inference:** unhandled events bubble from child to parent through `+0x04`.

## 2. Who creates the `1018:6144` object

**Fact.** `1018:553a` has no Ghidra function. It is the `+0x1c` handler stored at cell `1018:3244` of table `1018:3228`. It has this switch:

- event `0x2715`: allocates `0x52` bytes via `1008:ef2c`, installs the table chain `1008:0bc8 -> 1008:07f4 -> 1008:0838 -> 1008:1aac -> 1018:6034`, stores the object at `self+0x7e`, and calls its `+0x30(child, self, self, 0x98, …)`;
- event `0x2716` (at `1018:566a`): allocates `0x68` bytes and installs `1008:0bc8 -> 1008:07f4 -> 1008:0838 -> 1008:1aac -> 1018:6144` (final store at `1018:56dc`). It stores the object at `self+0x82` and calls its `+0x30(child, self, self, 0x98, …)`. It then dispatches on `DAT_1060:01ec` slots `+0x124`/`+0x128` with `(0x13, 0x6d73)`, and sets `DAT_1060:01e2 = 1`;
- any other event: `1008:b7de`.

Therefore the `1018:6144` object's `+0x04` is the `1018:3228` object (**fact**, from the argument trace above).

**Fact: `1018:5496` is the matching teardown.** It calls the `+0x82` object's slot 0, clears `self+0x82`, and sends `0x271b` to `self+0x1c`. `1008:ac80` calls it at the end of its `0x2af9` branch:

```text
1008:ad6f: push 1
1008:ad71: mov es,[bp+8]
1008:ad74: push dword es:[di+0x08]     ; di = [bp+6] = receiver (set at 1008:acc0)
1008:ad79: call far 1018:5496
```

`1010:4640` stores its `[bp+0x12]` argument, the second `+0x30` argument, into `self+0x08`. For B, that argument is the `1018:3228` owner. So `1018:5496` receives the owner, deletes B (the owner's `+0x82` is B itself), and sends `0x271b` to the owner's `+0x1c` = `1018:553a`, which passes it to `1008:b7de`.

**Correction to `docs/application-state-transition.md` / `docs/nested-object-behavior.md`.** Those docs describe the `1018:5496` call as `(replacement, 1)` and say `0x271b` is absorbed by the `1008:2b04` `RETF 8` stub. The raw call site shows the argument is `receiver->+0x08`, not `DAT_1060:01e4`. The absorbed-event conclusion for that call site is therefore unsupported. The `1008:e8b6` byte fact (`RETF 8`) itself is unaffected.

**Correction to `docs/object-table-ownership.md`.** The candidate run reported there as "`1018:3230`" is table `1018:3228`. The base was misaligned by 8: `+0x18 -> 1018:509c` there is `+0x20` here, and `+0x1c` is `1018:553a`. Its installer is `1018:2c6d`, inside the raw function `1018:2bf0`.

### Upward chain

**Fact.** `1018:2bf0`:
- allocates `0x8a` bytes;
- installs `1008:0bc8 -> 07f4 -> 0838 -> 1aac -> 1018:3108 -> 1018:3228`;
- stores the object at `self+0x46`;
- calls `+0x30(obj, self, …, 0x97, …)`.

`1018:2bf0` is cell `1008:9902`, which is slot `+0x11c` of table `1008:97e6`. That table's `+0x1c` is `1018:267e`, whose jump table at `1018:26a6` routes event `0x2711` (index 0) to `1018:26d4`, which calls `self->+0x11c`.

**Correction to `docs/object-table-ownership.md`.** The run reported as "`1008:97c2`" is table `1008:97e6`. Its base was misaligned by `0x24`: `+0x40 -> 1018:267e` there is `+0x1c` here. Table `1008:97e6` is installed at `1008:932e`. That site's containing function and creator were not traced in this step.

```text
[1008:97e6 object]  +0x1c = 1018:267e
   event 0x2711 --> +0x11c = 1018:2bf0 --creates--> [1018:3228 object] at +0x46
[1018:3228 object]  +0x1c = 1018:553a
   event 0x2716 --creates--> [1018:6144 object] at +0x82, +0x04 = 1018:3228 object
[1018:6144 object]  +0x1c = 1008:ac80
```

## 3. What sends events to `1008:ac80`

### Children created by `1008:a656`

**Fact.** `1008:a656`, slot `+0x34` of `1018:6144`, calls the base `1010:15f0` and then creates sub-objects. Each is created through `+0x30(sub, B, B->+0x46, id, …)`, so each sub-object's `+0x04` is the `1018:6144` object (B):

| Field | Size | Final table | `+0x30` id argument |
|---|---|---|---|
| `+0x46` | `0x6a` | `1008:9566` | `0xbb9` |
| `+0x4a` | `0x56` | `1008:b0c8` (install `1008:a76c`) | `0xbb9` |
| `+0x4e` | `0x56` | `1008:b0c8` (install `1008:a811`) | `0xbba` |
| `+0x56` | `0x56` | `1008:b0c8` (install `1008:a8ad`) | `0xbbc` |
| `+0x5a` | `0x56` | `1008:b0c8` (install `1008:a949`) | `0xbbd` |
| `+0x62` | `0x5c` | `1008:b1e4` | `0xfb` |

The `+0x52` field and the complete body after `1008:aa3e` were not exhaustively decoded.

### The `1008:b0c8` class

**Fact.** Table `1008:b0c8` leaves `+0x1c` as `1008:b7de`. It overrides:

- `+0x34` = `1010:ccee`: copies record words `+0x24`, `+0x26`, `+0x28`, `+0x2a` into `self+0x4c`, `+0x4e`, `+0x50`, `+0x52`;
- `+0x58` = `1010:cd6c`: calls `1008:e412(2)` and then `self->+0x118(self->+0x55)`;
- `+0x118` = `1010:ce0a`: the emitter below.

`1010:ce0a`, after its own `+0x110`/`+0x114` and `DAT_1060:0a2e` handling, does:

```text
1010:ce88: mov ax, es:[si+0x4c]; cwde; push eax          ; event = (long)self->word_4c
1010:ce93: push dword es:[si+4]                          ; receiver = self->+0x04
1010:ce9f: call far es:[bx+0x1c]                         ; receiver's event handler
```

For the `1008:b0c8` children of B, `self+0x04` is B, whose `+0x1c` is `1008:ac80`.

**Fact (resolved edge):**

```text
1010:cd6c (+0x58 of 1008:b0c8) -> 1010:ce0a (+0x118) --1010:ce9f, slot +0x1c--> 1008:ac80
```

B's own methods also call these slots directly:
- `1008:ae32`, inside `1008:add8` (B's `+0x20`, `RETF 0xc`), calls the `+0x56` child's `+0x118` with argument `1`. That child's `+0x118` is `1010:ce0a`, so this re-enters `1008:ac80` with the child's event.
- `1008:ac01`, which precedes the `1008:ac80` entry and lies inside the raw area of `1008:ab54` (B's `+0x114`), calls the `+0x62` child's `+0x118`. That child is a `1008:b1e4` object; its `+0x118` was not decoded.

### Where the event values come from

**Fact.** The emitted event is the `+0x24` word of a record. `1010:45ea` gets that record from `1010:10f2(id, …)`, a cached lookup through the object at `DAT_1060:0a14` (its `+0x80` and `+0x54` methods). `0x2af9`, `0x2afa`, `0x2afb`, `0x32c9` and `0x32ca` do not occur as bytes anywhere in `BLIZZARD.EXE` outside code comparisons, so the records are not embedded in this executable.

**Hypothesis.** The records come from external content, possibly `SB_CDRES.DLL` or a data file (see `docs/original-executable.md`). Which child emits which of `1008:ac80`'s five commands is therefore not established from the EXE alone. `0x32c9`/`0x32ca` equal 10000 + `0xbb9`/`0xbba`, but no code computing that sum was found. This is a numeric coincidence, not evidence.

## 4. A related but distinct producer (not an `ac80` path)

**Fact.** Raw function `1018:6a8e` is slot `+0x114` of table `1018:684e`. That table's `+0x1c` is `1008:b7de`. The function builds a string from its item list, validates it with `1010:cfba(DAT_1060:0a38, buf)`, and on success:
- replaces `DAT_1060:01e4` with a new `0x262`-byte object (final table `1008:2b04`);
- calls `1018:a9a0`/`1018:aeaa`;
- sets `DAT_1060:01de`;
- clears `DAT_1060:0a36`;
- calls `1010:d0a6`;
- sends `0x2af9` to itself.

Otherwise it may send `0x2afa`. Both go through `1008:b7de` to `self+0x04`.

Its objects are created by `1018:649a`, slot `+0x34` of table `1018:6034` (the `0x2715` child, stored at owner `+0x7e`), with `+0x04` set to that child. That child's handler is `1018:66ee`, which also tests `0x2af9`.

**Strong inference.** This is the parallel `1018:6034` branch. Its `0x2af9` reaches `1018:66ee`, not `1008:ac80`. The duplicated `0x2af9` work in `1008:ac80`'s `0x2af9` branch and in `1018:6a8e` is a sibling implementation, not a caller relationship.

## 5. Answer

`1008:ac80` has no direct caller. It is the `+0x1c` event handler of objects that use table `1018:6144`. Those objects are created by the `1018:3228` object's handler `1018:553a` on event `0x2716` and stored at the owner's `+0x82`.

The one concrete indirect call site proven so far is `1010:ce9f`, inside `1010:ce0a`, slot `+0x118` of `1008:b0c8` objects. It sends their record-defined event word to their `+0x04` parent. For the four `1008:b0c8` children created by `1008:a656`, that parent is the `1018:6144` object. Events that `1008:ac80` does not handle continue up the `+0x04` link to `1018:553a`, the `1018:3228` handler.

## 6. Open edges

1. Who calls `+0x58` on `1008:b0c8` objects. There are many generic `call [bx+0x58]` sites. The window procedure's mouse path (`1010:5db2` -> `+0x0c` -> `1010:527c`/`1010:553e`) is a candidate, but not proven.

   **Resolved in Step 15** (`docs/input-to-command.md`): base `+0x50` = `1010:491e` hit-tests down the view tree and calls `+0x58` on the clicked object for left-button events. The window-procedure `+0x0c` path only records the cursor position.
2. Who sends `0x2716` to the `1018:3228` object and `0x2711` to the `1008:97e6` object. `1018:5343`, inside `1018:509c`, sends dword `0x2711` through a receiver `+0x1c`; its receiver is not yet identified. No `0x2716` literal was found, which suggests it is data-driven like the events in section 3 (hypothesis).
3. The function and creator around the `1008:97e6` installer at `1008:932e`.
4. The source of the `1010:10f2` records, and the mapping from child id to command value.
5. No Ghidra function boundaries were added for `1018:553a`, `1018:2bf0`, `1018:6a8e`, `1010:ccee`, or `1010:45ea`. Adding them is a reversible, evidence-supported next step if the project wants decompiler output for them.
