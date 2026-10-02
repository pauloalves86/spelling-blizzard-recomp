// Create missing functions from seed addresses in Ghidra.
// Usage: analyzeHeadless <project_dir> <project_name> -process BLIZZARD.EXE -noanalysis -scriptPath ghidra -postScript CreateSeeds.java <seeds-file>

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.nio.file.*;
import java.util.List;

public class CreateSeeds extends GhidraScript {
  public void run() throws Exception {
    String seedsFile = getScriptArgs()[0];
    List<String> lines = Files.readAllLines(Paths.get(seedsFile));

    int total = 0;
    int alreadyExists = 0;
    int insideOther = 0;
    int disasmFailed = 0;
    int createdCount = 0;
    int createFailed = 0;

    for (String s : lines) {
      s = s.trim();
      if (s.isEmpty()) continue;
      total++;

      Address ad = currentProgram.getAddressFactory().getAddress(s);
      if (ad == null) {
        println("ERROR: Invalid address string: " + s);
        createFailed++;
        continue;
      }

      Function fAt = currentProgram.getFunctionManager().getFunctionAt(ad);
      if (fAt != null) {
        alreadyExists++;
        continue;
      }

      Function fContaining = currentProgram.getFunctionManager().getFunctionContaining(ad);
      if (fContaining != null) {
        println(String.format("WARNING: Seed %s is inside existing function %s (%s)",
          s, fContaining.getName(), fContaining.getEntryPoint()));
        insideOther++;
        continue;
      }

      Instruction ins = currentProgram.getListing().getInstructionAt(ad);
      if (ins == null) {
        boolean ok = disassemble(ad);
        if (!ok) {
          println("ERROR: Disassembly failed at " + s);
          disasmFailed++;
          continue;
        }
      }

      Function newFunc = createFunction(ad, null);
      if (newFunc != null) {
        createdCount++;
      } else {
        println("ERROR: createFunction failed at " + s);
        createFailed++;
      }
    }

    println(String.format("=== Seed Function Creation Summary ==="));
    println(String.format("Total seeds processed:      %d", total));
    println(String.format("Already had function:       %d", alreadyExists));
    println(String.format("Inside another function:    %d", insideOther));
    println(String.format("Disassembly failed:         %d", disasmFailed));
    println(String.format("Successfully created:       %d", createdCount));
    println(String.format("Failed to create function:  %d", createFailed));
  }
}
