// Read-only survey of seed addresses in Ghidra.
// Args: <seeds-file>
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.nio.file.*;
import java.util.List;

public class CheckSeeds extends GhidraScript {
  public void run() throws Exception {
    String seedsFile = getScriptArgs()[0];
    List<String> lines = Files.readAllLines(Paths.get(seedsFile));
    int total = 0;
    int hasFunctionAt = 0;
    int insideOtherFunction = 0;
    int hasInstructionNoFunction = 0;
    int noInstructionNoFunction = 0;

    for (String s : lines) {
      s = s.trim();
      if (s.isEmpty()) continue;
      total++;
      Address ad = currentProgram.getAddressFactory().getAddress(s);
      Function fAt = currentProgram.getFunctionManager().getFunctionAt(ad);
      if (fAt != null) {
        hasFunctionAt++;
        continue;
      }
      Function fContaining = currentProgram.getFunctionManager().getFunctionContaining(ad);
      if (fContaining != null) {
        insideOtherFunction++;
        continue;
      }
      Instruction ins = currentProgram.getListing().getInstructionAt(ad);
      if (ins != null) {
        hasInstructionNoFunction++;
      } else {
        noInstructionNoFunction++;
      }
    }

    println(String.format("Total seeds: %d", total));
    println(String.format("  Function exists at entry: %d", hasFunctionAt));
    println(String.format("  Inside another function body: %d", insideOtherFunction));
    println(String.format("  Has instruction, but no function: %d", hasInstructionNoFunction));
    println(String.format("  No instruction, no function (undefined): %d", noInstructionNoFunction));
    println(String.format("Total functions in program: %d", currentProgram.getFunctionManager().getFunctionCount()));
  }
}
