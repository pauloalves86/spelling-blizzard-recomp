// Step 14: read-only trace of the indirect caller of 1008:ac80.
// Input: BLIZZARD.EXE in ghidra/SpellingBlizzard.gpr. Run with -readOnly -noanalysis.
// Args: addresses; for each prints containing function, callers, and decompilation.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.symbol.*;
public class Step14Trace extends GhidraScript {
  public void run() throws Exception {
    DecompInterface di = new DecompInterface(); di.openProgram(currentProgram);
    FunctionManager fm = currentProgram.getFunctionManager();
    for (String s : getScriptArgs()) {
      boolean decomp = s.startsWith("+"); if (decomp) s = s.substring(1);
      Address a = currentProgram.getAddressFactory().getAddress(s);
      Function f = fm.getFunctionContaining(a);
      println("=== " + s + " in " + (f == null ? "<no function>" : f.getEntryPoint() + " " + f.getName() + " " + f.getCallingConventionName() + " purge=" + f.getStackPurgeSize()));
      if (f == null) continue;
      for (Reference r : currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint())) {
        Function c = fm.getFunctionContaining(r.getFromAddress());
        println("  XREF " + r.getFromAddress() + " " + r.getReferenceType() + " in " + (c == null ? "<none>" : c.getEntryPoint() + " " + c.getName()));
      }
      if (decomp) {
        DecompileResults res = di.decompileFunction(f, 120, monitor);
        println(res.decompileCompleted() ? res.getDecompiledFunction().getC() : "  <decompile failed>");
      }
    }
  }
}
