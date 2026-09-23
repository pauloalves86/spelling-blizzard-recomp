import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.Reference;

public class Step12Trace extends GhidraScript {
    private Address addr(String val) {
        return currentProgram.getAddressFactory().getAddress(val);
    }

    private void dumpFunction(DecompInterface decomp, String val) {
        println("==================================================");
        println("FUNCTION: " + val);
        println("==================================================");
        Function f = currentProgram.getFunctionManager().getFunctionAt(addr(val));
        if (f == null) {
            println("Function not found at " + val);
            return;
        }
        println("Name: " + f.getName() + " Signature: " + f.getSignature());
        println("Purge: " + f.getStackPurgeSize());
        println("References TO entry point:");
        for (Reference ref : currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint())) {
            Function caller = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
            println("  From: " + ref.getFromAddress() + " Type: " + ref.getReferenceType() +
                    " Caller: " + (caller != null ? caller.getEntryPoint() + " " + caller.getName() : "none"));
        }

        println("--- INSTRUCTIONS ---");
        Listing listing = currentProgram.getListing();
        for (Instruction instr : listing.getInstructions(f.getBody(), true)) {
            println("  " + instr.getAddress() + " : " + instr);
        }

        println("--- DECOMPILATION ---");
        DecompileResults res = decomp.decompileFunction(f, 60, monitor);
        if (res != null && res.getDecompiledFunction() != null) {
            println(res.getDecompiledFunction().getC());
        } else {
            println("Decompile failed: " + (res != null ? res.getErrorMessage() : "null"));
        }
    }

    @Override
    public void run() throws Exception {
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);

        dumpFunction(decomp, "1018:509c");
        dumpFunction(decomp, "1010:3d56");

        // Check references to 1010:3d56 callers
        Function f3d56 = currentProgram.getFunctionManager().getFunctionAt(addr("1010:3d56"));
        if (f3d56 != null) {
            println("=== CALLERS of 1010:3d56 ===");
            for (Reference ref : currentProgram.getReferenceManager().getReferencesTo(f3d56.getEntryPoint())) {
                Function caller = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
                println("Caller: " + ref.getFromAddress() + " in " + (caller != null ? caller.getName() : "none"));
                if (caller != null) {
                    dumpFunction(decomp, caller.getEntryPoint().toString());
                }
            }
        }
    }
}
