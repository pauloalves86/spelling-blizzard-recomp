// Read-only focused function dossier: entry, callers/callees, and decompilation.
// Usage: ... -postScript InspectFunction.java <segment:offset>
import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.Reference;

public class InspectFunction extends GhidraScript {
  public void run() throws Exception {
    Address entry = currentProgram.getAddressFactory().getAddress(getScriptArgs()[0]);
    Function f = currentProgram.getFunctionManager().getFunctionAt(entry);
    if (f == null) throw new IllegalArgumentException("No function at " + entry);
    println("FUNCTION " + f.getEntryPoint() + " " + f.getName() +
      " body_bytes=" + f.getBody().getNumAddresses() +
      " purge=" + f.getStackPurgeSize());

    println("CALLERS");
    for (Reference r : currentProgram.getReferenceManager().getReferencesTo(entry)) {
      Function caller = currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress());
      println("  " + r.getFromAddress() + " " + r.getReferenceType() + " in " +
        (caller == null ? "<no function>" : caller.getEntryPoint() + " " + caller.getName()));
    }
    println("OUTGOING REFERENCES");
    InstructionIterator it = currentProgram.getListing().getInstructions(f.getBody(), true);
    while (it.hasNext()) {
      Instruction ins = it.next();
      for (Reference r : ins.getReferencesFrom()) {
        Function target = currentProgram.getFunctionManager().getFunctionAt(r.getToAddress());
        if (target != null || r.getReferenceType().isCall())
          println("  " + ins.getAddress() + " -> " + r.getToAddress() + " " + r.getReferenceType() +
            (target == null ? "" : " " + target.getName()));
      }
    }

    DecompInterface decomp = new DecompInterface();
    decomp.openProgram(currentProgram);
    DecompileResults result = decomp.decompileFunction(f, 60, monitor);
    println("DECOMPILED");
    println(result.getDecompiledFunction() == null ? "<unavailable>" : result.getDecompiledFunction().getC());
    decomp.dispose();
  }
}
