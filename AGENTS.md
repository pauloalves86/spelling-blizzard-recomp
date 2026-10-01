# AGENTS.md — Spelling Blizzard Recomp

## What this is

- `Spelling Blizzard` is a 1994 Bright Star Technology, Inc. educational spelling game, distributed as a Windows CD product.
- The canonical executable is `original/BLIZZARD.EXE`, version `1.10.CD for Windows`, identified by its VERSIONINFO resource. It is a 229,472-byte Win16 NE executable. Preserve this file as immutable ground truth.
- The current project is a **recompilation/decompilation effort**, not a rewrite. The immediate goal is to recover the original program's structure and behavior from the executable and Ghidra project, then produce a clean, maintainable source implementation that can eventually reproduce the original behavior.
- The executable is a **16-bit Windows 3.x NE application**, not a DOS/4GW, PE, or LE/LX program. Ghidra loads it as `x86:LE:16:Protected Mode:default`.
- The application is strongly Win16-oriented: imports include `KERNEL`, `GDI`, `USER`, `KEYBOARD`, `MMSYSTEM`, and `SHELL`; graphics use GDI; audio uses Windows MMSYSTEM `waveOut`; and input uses Windows message/keyboard APIs.
- The analyzed executable contains 5 initialized code segments and 8 data segments, with 959 discovered Ghidra functions including imports/thunks and runtime/framework code. The current architecture map is only Phase 1/2 and many `FUN_...` names remain unresolved.
- The current working environment uses **Ghidra 12.1.3** at:
  `/home/pmfa/Projects/ghidra_12.1.3_PUBLIC/`
- The Ghidra project is `ghidra/SpellingBlizzard.gpr` with its database in `ghidra/SpellingBlizzard.rep/`. The original executable is separate from the Ghidra project.

## Source-of-truth hierarchy

When investigating behavior, use this order:

1. `original/BLIZZARD.EXE` — immutable binary ground truth.
2. Ghidra's analysis of the original executable — primary reverse-engineering workspace.
3. Project documentation under `docs/` — accumulated verified findings and hypotheses.
4. Reconstructed source code — derived representation, never a source of truth when it conflicts with the executable.
5. General knowledge about Win16, Microsoft runtimes, Bright Star, or similar games — useful only as supporting context.

Never silently turn an inference into a fact. Preserve the distinction between **Fact**, **Strong inference**, and **Hypothesis** used by the existing architecture documentation.

## Non-negotiables

1. **Never modify `original/BLIZZARD.EXE`.**
   - Treat it as immutable.
   - Do not patch, strip, normalize, recompress, or otherwise rewrite it.
   - Any experiments involving binary modifications must operate on a copy outside `original/`.

2. **Do not overwrite Ghidra evidence casually.**
   - Preserve useful auto-analysis results, function boundaries, symbols, references, and comments.
   - Rename functions only when the proposed name is supported by evidence.
   - Prefer documenting uncertainty over making a speculative rename look authoritative.

3. **Do not invent source-level structure.**
   - Strings such as `theObject`, `theMissile`, `itsLayer[i]`, `itsGridCellArray`, `itsImageHandlerList`, etc. are valuable anchors, but their exact class/field ownership is not yet proven.
   - C++-style names and vtable-like structures are evidence of an object-oriented framework, not proof of the original source hierarchy.

4. **Separate application behavior from runtime/framework behavior.**
   - The executable contains substantial startup, C runtime, Win16 compatibility, framework, and error-handling code.
   - Do not assume every discovered function is game logic.
   - Startup at `1000:001a` is strongly believed to be generic Win16/compiler/runtime startup rather than the game main loop.

5. **Preserve the original Win16 execution model during the initial recompilation effort.**
   - Do not prematurely port the game to Win32, SDL, DOS, or another platform.
   - First reconstruct the original interfaces and semantics.
   - Platform modernization can be a later layer once behavior is understood.

6. **No unsupported compiler certainty.**
   - Current evidence strongly suggests a Microsoft 16-bit C/C++ runtime/framework environment, based on Win16 imports, segmented calling conventions, and Microsoft-style `R600x` diagnostics.
   - This remains a **medium-confidence toolchain hypothesis**, not a proven compiler identification.
   - Do not state that Microsoft C/C++ was definitively used until startup patterns, library fingerprints, or known-library comparison establishes it.

7. **Do not assume DOS graphics or sound.**
   - Current evidence does not support direct VGA BIOS graphics or direct Sound Blaster/AdLib/OPL programming.
   - Graphics are strongly represented by Win16 GDI.
   - Audio is strongly represented by Windows MMSYSTEM `waveOut`.

8. **Trace before implementing.**
   - Before replacing a function, identify callers, callees, imports, data references, relevant strings/resources, and observable side effects.
   - Prefer small, reversible discoveries over large speculative rewrites.

9. **Do not linear-read large documentation or Ghidra output without a reason.**
   - Search for exact strings, API names, addresses, or documented anchors first.
   - Read only the relevant ranges.
   - Keep findings focused and link them to concrete addresses/functions.

10. **Keep canonical evidence read-only.**
    - Scripts that analyze the binary should produce reports or derived artifacts rather than modifying the executable.
    - Generated reports should identify the input file and analysis method.

## Critical files

| File | Role |
| --- | --- |
| `original/BLIZZARD.EXE` | Canonical immutable executable. Primary behavioral and structural ground truth. |
| `ghidra/SpellingBlizzard.gpr` | Ghidra project for the executable. |
| `ghidra/SpellingBlizzard.rep/` | Ghidra database backing the project. |
| `docs/toolchain.md` | Current toolchain/runtime hypothesis and supporting evidence. |
| `docs/strings.md` | Initial string inventory and classification; useful address anchors. |
| `docs/original-executable.md` | Canonical executable identity, format, segments, resources, dependencies, and immutability notes. |
| `docs/architecture.md` | Current architecture map, startup path, function inventory, interfaces, uncertainties, and next steps. |
| `/home/pmfa/Projects/ghidra_12.1.3_PUBLIC/` | Ghidra 12.1.3 installation used for analysis. |
| `AGENTS.md` | Repository-wide working rules and reverse-engineering conventions. |

## Executable identity

The canonical binary currently being analyzed is:

- Filename: `BLIZZARD.EXE`
- Path: `original/BLIZZARD.EXE`
- Size: `229,472` bytes
- MD5: `9689e835fb4d553b062c87345c1fafa7`
- SHA-1: `74ced33706f70089f70a58b68ec03cc698f184cb`
- SHA-256: `ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2`
- VERSIONINFO product: `Spelling Blizzard`
- Version: `1.10.CD for Windows`
- Internal name: `BLIZZARD`
- Original filename: `BLIZZARD.EXE`
- Company: `Bright Star Technology, Inc., a wholly-owned subsidiary of Sierra On-Line, Inc.`
- Copyright: `1994 by Bright Star Technology, Inc.`

The file's filesystem timestamp is `1994-07-19 01:10:00 -0300`, but this is filesystem metadata in the workspace and must not be treated as definitive original release metadata.

## Executable format

The MZ stub only provides Windows compatibility messaging and DOS termination. It prints:

`This program requires Microsoft Windows.`

Do not mistake this stub for the application's main execution path.

## Naming conventions

During the exploratory phase:

- Keep Ghidra-generated names such as `FUN_1000_0346` when semantics are uncertain.
- Use names that describe a verified role, not an assumed implementation.
- Prefer names such as `win16_startup_environment` only after the evidence supports that role.
- Preserve original address information in comments when renaming.
- For reconstructed source, keep a mapping from source functions to original segment:offset addresses.

Suggested comment format:

```c
/* Original: 1000:0346
 * Evidence: GETMODULEFILENAME + command-line parsing.
 * Confidence: medium.
 */
```

Do not use names such as `game_main`, `render_scene`, `load_level`, or `play_sound` solely because they seem plausible.

## Reverse-engineering workflow

For a new investigation:

1. **State the question precisely.**
   - Example: “Where is the Win16 message pump?”
   - Avoid vague tasks such as “understand the main loop.”

2. **Search the existing documentation first.**
   - Search `docs/architecture.md`, `docs/strings.md`, `docs/toolchain.md`, and other relevant project notes.
   - Reuse established terminology and addresses.

3. **Search Ghidra strategically.**
   - Find imported API references.
   - Find references to relevant strings.
   - Inspect callers and callees.
   - Inspect data references.
   - Compare neighboring functions only when useful.

4. **Record evidence before interpretation.**
   - Address/function.
   - Relevant instructions or decompiler behavior.
   - Imports/strings/resources touched.
   - Callers/callees.
   - Proposed role.
   - Confidence.

5. **Update documentation.**
   - Add facts and hypotheses to the appropriate document.
   - Do not erase uncertainty merely to make the architecture look cleaner.

6. **Only then implement/reconstruct code.**
   - Keep the implementation traceable back to the binary.
   - Avoid speculative abstractions until the underlying behavior is understood.

## Build/recompilation direction

The project does not yet have an established source build pipeline in the available evidence. Do not invent a build architecture in this document.

The intended progression is:

```text
Original BLIZZARD.EXE
        |
        v
Ghidra analysis + binary evidence
        |
        v
Verified function / data / API mapping
        |
        v
Reconstructed C/C++ source
        |
        v
Native recompilation
        |
        v
Behavioral comparison against original
```

Initially, prioritize **behavioral fidelity and traceability** over portability or modernization.

A later compatibility layer may reproduce Win16 APIs on a modern host, but that should be a deliberate architectural decision after the original API usage is mapped.

## Verification ritual

After any meaningful reverse-engineering or reconstruction change:

1. Confirm the original executable hash has not changed.
2. Confirm the change is supported by binary evidence.
3. Check all references/callers of any renamed or reconstructed function.
4. Check structure offsets against actual memory accesses.
5. Check segmented pointer assumptions and far-call conventions.
6. Check imported API signatures and calling conventions.
7. Compare relevant runtime behavior against the original executable where possible.
8. Record unresolved assumptions explicitly.

For binary-analysis scripts, record:

- input filename;
- input hash when practical;
- tool/version;
- command or analysis method;
- output artifact;
- interpretation and confidence.

## Evidence discipline

Use these labels consistently:

### Fact

Directly established by the executable, Ghidra analysis, or a reproducible measurement.

Examples:

- `BLIZZARD.EXE` is an NE executable.
- VERSIONINFO says `Spelling Blizzard 1.10.CD for Windows`.
- `WAVEOUTOPEN` is imported.
- `1000:001a` is the NE entry point.

### Strong inference

A conclusion supported by multiple direct observations but not encoded explicitly in the binary.

Examples:

- The program is a Win16 message-driven application.
- A function is likely runtime startup.
- A group of strings likely belongs to an object-oriented framework.

### Hypothesis

A plausible interpretation requiring further confirmation.

Examples:

- `SB_CDRES.DLL` supplies CD content.
- A specific function is the game's main loop.
- A particular compiler/version produced the binary.
- A particular string corresponds to a specific source-level class.

When new evidence contradicts a hypothesis, update the hypothesis rather than forcing the evidence to fit it.

## Important principle

The purpose of this project is to recover what the original program **actually does**, not what a modern developer might expect a spelling game to do.

When evidence is incomplete:

- trace more;
- measure more;
- preserve uncertainty;
- avoid convenient assumptions;
- keep the original executable untouched.

The executable is the specification.
