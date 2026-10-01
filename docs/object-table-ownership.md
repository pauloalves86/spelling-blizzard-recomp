# Object/Table Ownership (Step 13)

## Scope, baseline, and method

This report distinguishes direct executable facts from table/owner inferences. No semantic class names or native object layouts are introduced.

Step 13 baseline, before changes:

- Branch `master`, clean worktree, HEAD `66ae1e0` (`feat: add Ghidra step 12 trace and verification scripts alongside callback dispatch documentation`).
- `original/BLIZZARD.EXE`, `ghidra/SpellingBlizzard.gpr`, and `ghidra/SpellingBlizzard.rep/` existed.
- Native build succeeded and CTest passed 2/2.
- Canonical executable hashes:
  - MD5 `9689e835fb4d553b062c87345c1fafa7`
  - SHA-1 `74ced33706f70089f70a58b68ec03cc698f184cb`
  - SHA-256 `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`

`ghidra/Step13Verify.java` was run against the existing Ghidra 12.1.3 project using the `x86:LE:16:Protected Mode` program, with `-readOnly -noanalysis`. It checks references, decoded instruction immediates, raw word/far-pointer occurrences, candidate memory windows, known constructor-like functions, and indirect-call slot displacements. It does not rename functions, define data, or change the project. The final successful run's output/log are workspace-local at `/home/pmfa/.hermes/cache/scratch/step13verify-final3.out` and `.log`.

Command used:

```text
/home/pmfa/Projects/ghidra_12.1.3_PUBLIC/support/analyzeHeadless \
  /home/pmfa/Projects/spelling-blizzard-recomp/ghidra SpellingBlizzard \
  -readOnly -noanalysis -process BLIZZARD.EXE \
  -postScript Step13Verify.java \
  -scriptPath /home/pmfa/Projects/spelling-blizzard-recomp/ghidra \
  -log /home/pmfa/.hermes/cache/scratch/step13verify-final3.log \
  > /home/pmfa/.hermes/cache/scratch/step13verify-final3.out 2>&1
```

## 1. Executive summary

> Step 14 correction (`docs/ac80-indirect-caller.md`). The raw scans below missed NE selector fixup chains: in the file, a far pointer's selector word holds a chain link, not a selector. With chains resolved, `1018:3230` is table **`1018:3228`** (`+0x1c` = `1018:553a`, `+0x20` = `1018:509c`, installed at `1018:2c6d`). `1008:97c2` is table **`1008:97e6`** (`+0x1c` = `1018:267e`, installed at `1008:932e`). The `+0x..` offsets below use the old misaligned bases.

Ownership was not recovered for any of the three candidate regions.

- `1018:3230` is a strongly table-like raw 16:16 pointer run: `+0x18` contains `1018:509c`; `+0x12c` contains `1020:4e8e`. No object stores this base in a statically identified field, and no constructor or dispatch path is tied to the run.
- `1008:97c2` is a strongly table-like run: `+0x40` contains `1018:267e`. Extending the inspected window through `+0x12c` shows that cell contains `1010:1d84`, not `1020:4e8e`. No owner or installation is proven.
- `1008:7f80` is a strongly table-like run: `+0x3c` contains `1020:2534`. The wrapper directly calls `1010:3d56`, but no object/table initialization or message-origin path to `1020:2534` is proven.

All candidate bases and highlighted pointer cells have zero Ghidra references. Full far-pointer scans found no literal pointer to any candidate base. Instruction-immediate scans found no `0x3230` or `0x97c2` installation constant. The only decoded code immediate `0x7f80` is paired with segment `0x1018` to address the distinct string `1018:7f80` (`"itsRemovalList"`), not candidate base `1008:7f80`.

The three regions remain candidate pointer runs, not confirmed object method tables. No native object model or callback code was added.

## 2. Candidate `1018:3230`

### Facts

- The inspected `0x140`-byte window is in Ghidra block `Code4`. Its entries are untyped bytes in the listing (`??`), not a Ghidra-defined table or structure.
- The exact raw far pointer at `1018:3248` is `9c 50 18 10`, decoding as `1018:509c` at `+0x18`. Ghidra has a function boundary there; its recorded stack purge is 12 bytes (`RETF 12`).
- The exact raw far pointer at `1018:335c` is `8e 4e 20 10`, decoding as `1020:4e8e` at `+0x12c`. Ghidra has a function boundary there; its recorded stack purge is 8 bytes (`RETF 8`).
- The neighboring `+0x1c` cell at `1018:324c` decodes to `1008:b82e`, which has no Ghidra function boundary in this analysis.
- Ghidra reports zero references to base `1018:3230`, to the highlighted cells, and direct code references to functions `1018:509c` and `1020:4e8e`. Exact byte scan for `30 32 18 10` found zero candidate-base pointer occurrences. No decoded instruction contains `0x3230` as an immediate scalar.
- The three raw `0x3230` word occurrences elsewhere in initialized memory are at `1060:06dc`, `1060:072f`, and `1060:0766`. Their surrounding `Data13` bytes spell text fragments `R6020`, `R6021`, and `R6002`; they are not pointer stores.

### Owner, initialization, allocation, and dispatch

No object field containing `1018:3230`, allocation site for an object using it, or constructor/factory write installing it was found. No indirect call was tied to either highlighted cell. The 16:16 pointer occurrences establish only their locations and decoded targets, not table ownership.

A matching slot displacement does appear elsewhere: `1018:267e` performs an indirect `CALLF [BX + 0x12c]` at `1018:27eb` through its incoming receiver's first-word table pointer. That branch corresponds to event `0x2ee2`, not the `0x2714` branch. It pushes the receiver far pointer (four bytes) before the call. The candidate `1020:4e8e` target purges eight bytes, so this callsite does not establish an ABI-compatible target assignment to `4e8e` under ordinary explicit-argument accounting. The receiver's runtime table base is unknown.

`1018:509c` separately calls its first-parameter receiver's `+0x1c` slot on the `CHEAT` path; `1020:4e8e` calls its receiver's `+0x1c` slot at `1020:4f10` when emitting `0x2714`/`0x2718`. The candidate run's `+0x18 -> 509c`, the `+0x1c` calls, and the matching event values do not prove a common runtime object or dispatch table.

### Constructor comparisons

- `1008:2bf4` has a direct reference from startup at `1000:00f6`. It allocates and initializes objects stored in globals such as `DAT_1060:0a14` and `DAT_1060:07c4`; the recovered far-pointer stores use other offsets/segments, not `1018:3230`.
- `1018:649a` allocates and initializes further objects with other offset/segment pairs. The inspected code does not install `1018:3230`.
- `1008:ac80` is the known replacement-object path; its final visible table is `1008:2b04`, not `1018:3230`.
- The comparison initializer `1008:0eba` stores `1008:0f80` as an object's first far pointer. This is a concrete table-install pattern, but it is a different address and provides no ownership evidence for `1018:3230`.

## 3. Candidate `1008:97c2`

### Facts

- The inspected `0x140`-byte window is in `Code2`; the entries remain untyped listing bytes.
- Cell `1008:9802` (`+0x40`) contains `7e 26 18 10`, decoding to function `1018:267e`, with `RETF 8`.
- The extended window's cell `1008:98ee` (`+0x12c`) contains `84 1d 10 10`, decoding to `1010:1d84`, with `RETF 12`.
- Ghidra reports zero references to `1008:97c2`, the highlighted cell, and function `1018:267e`. The exact far-pointer scan for `c2 97 08 10` and the decoded instruction-immediate scan for `0x97c2` both returned zero.

### Owner and actual dispatch

No allocation, constructor, table-pointer installation, or actual call through `1008:9802` was recovered. `1018:267e` has no direct code-reference caller. Its own `+0x12c` call at `1018:27eb` uses the runtime table pointer from its receiver; neither the `1008:97c2` nor `1018:3230` base is established as that receiver's table.

The `+0x40` calls inside `1010:3d56` at `1010:3eb0` and `1010:3f37` are real indirect calls, but they pass a far receiver plus a word (six explicit argument bytes). `1018:267e` purges eight bytes. Those `1010:3d56` calls therefore do not establish a direct ABI-compatible dispatch to `1018:267e`; matching `+0x40` offsets are insufficient.

If `1008:97c2` were the receiver table used by `1018:267e`, the candidate `+0x12c` cell would target `1010:1d84`, not `1020:4e8e`. This is conditional raw-table evidence, not proof that either receiver uses this base.

## 4. Candidate `1008:7f80`

### Facts

- The inspected `0x80`-byte window is in `Code2`; the listing cells are untyped bytes.
- Cell `1008:7fbc` (`+0x3c`) contains `34 25 20 10`, decoding to function `1020:2534`, with `RETF 8`.
- Ghidra reports zero references to base `1008:7f80`, cell `1008:7fbc`, and function `1020:2534`. The exact far-pointer scan for `80 7f 08 10` found zero occurrences.
- `1020:2534` directly calls `1010:3d56` at `1020:2548`. `1010:3d56` then dispatches through its own receiver's `+0x40` slot at `1010:3eb0` and `1010:3f37`. The sole direct Ghidra caller of `1010:3d56` is `1020:2534`; `1020:2534` itself has no direct code-reference caller.

### Segment collision and initialization search

The only decoded instruction immediate `0x7f80` is at `1018:74e4` inside `1018:71da`. On the failure path, the instructions push `0x1018`, then `0x7f80`, then `0x1008`, then `0x5554`, before calling `1008:b6f8` at `1018:74ed`. Ghidra identifies `1018:7f80` as the string `"itsRemovalList"`. Thus the observed far string pointer is `1018:7f80`, not the candidate base `1008:7f80`; it is not an initializer for that candidate table.

No allocation site, constructor, object field, message-origin edge, or dispatch through candidate cell `1008:7fbc` was established. The code path proven so far begins only at `1020:2534`, continues by direct call to `1010:3d56`, and ends at that function's unresolved runtime receiver `+0x40` callback.

## 5. Recovered object/table relationships

| Object/owner | Candidate table | Installation evidence | Dispatch evidence | Confidence |
|---|---|---|---|---|
| Unknown | `1018:3230` | No base reference, literal far pointer, immediate offset store, or constructor write found | Raw `+0x18 -> 1018:509c`, `+0x12c -> 1020:4e8e`; no dispatch through either cell proven | Strong inference that the raw run is table-like; owner unresolved |
| Unknown | `1008:97c2` | No base reference, literal far pointer, immediate offset store, or constructor write found | Raw `+0x40 -> 1018:267e`; no dispatch through that cell proven | Strong inference that the raw run is table-like; owner unresolved |
| Unknown | `1008:7f80` | No base reference, literal far pointer, or constructor write found; the matching offset immediate is for string `1018:7f80` | Raw `+0x3c -> 1020:2534`; no dispatch through that cell proven | Strong inference that the raw run is table-like; owner unresolved |
| Replacement object from the known `1008:ac80` path | `1008:2b04` | Existing Step 11 evidence shows this final table installed before the object is stored in `DAT_1060:01e4` | Existing replacement lifecycle dispatches through its known entries | Fact for that previously recovered path; unrelated to the three candidates |

The verifier emits every four-byte cell in the analyzed windows, with the decoded segmented target and any Ghidra function boundary/stack-purge information. The full deterministic dump is in the workspace-local output named in the method section; the rows above focus on the ownership-relevant entries.

## 6. Recovered call chains

Proven partial chains:

```text
1018:267e, event 0x2ee2
    -> load incoming receiver's first-word table pointer
    -> CALLF [BX + 0x12c] at 1018:27eb
    -> runtime target unresolved
```

```text
1020:4e8e, condition word == 8
    -> PUSH 0x2714
    -> CALLF [receiver table + 0x1c] at 1020:4f10
    -> runtime target unresolved
```

```text
1020:2534
    -> direct CALLF 1010:3d56 at 1020:2548
    -> CALLF [receiver table + 0x40] at 1010:3eb0 / 1010:3f37
    -> runtime target unresolved
```

Static pointer-run occurrences that are not proven dispatch chains:

```text
1018:3230 + 0x18  -> 1018:509c
1018:3230 + 0x12c -> 1020:4e8e
1008:97c2 + 0x40  -> 1018:267e
1008:97c2 + 0x12c -> 1010:1d84
1008:7f80 + 0x3c  -> 1020:2534
```

No allocation → object → installed candidate table → indirect call → target chain was recovered.

## 7. Unresolved boundaries

- Which object, if any, owns each candidate run; the object field containing a candidate base; and how that field is initialized or copied.
- Whether the raw runs are runtime method/callback tables, unused or alternate tables, or another structure containing executable far pointers.
- Whether any object using `1018:3230` can reach both `1018:509c` and `1020:4e8e`.
- The actual receiver/table used at `1018:267e`'s `+0x12c` call and at each target handler's object-relative calls.
- The message-origin path into `1020:2534` and the receiver/target for `1010:3d56`'s `+0x40` dispatch.
- Any runtime copy/registration path that installs a candidate table without a literal far-pointer constant.

## 8. Recommended next step

Trace the receiver far pointer and its first-word table value on the actual indirect invocation paths into `1018:267e` and `1018:509c`. Start with the real `1018:267e` `+0x12c` call for event `0x2ee2`, then identify its allocation/constructor or registration source. This directly tests whether a runtime receiver uses `1008:97c2`, `1018:3230`, or a different table, while checking stack arguments against each target's `RETF` cleanup. Do not add native table/object structures until that owner and installation edge is proven.
