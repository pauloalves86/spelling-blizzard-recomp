import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.Reference;

public class Phase10StateMachineTrace extends GhidraScript {
    private Address address(String value) {
        return currentProgram.getAddressFactory().getAddress(value);
    }

    private void reportFunction(DecompInterface decompiler, String value) {
        Function function = currentProgram.getFunctionManager().getFunctionAt(address(value));
        println("===== " + value + " =====");
        if (function == null) {
            println("NO FUNCTION");
            return;
        }
        println("name=" + function.getName() + " signature=" + function.getSignature() +
            " purge=" + function.getStackPurgeSize());
        println("callers:");
        for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(function.getEntryPoint())) {
            Function caller = currentProgram.getFunctionManager().getFunctionContaining(reference.getFromAddress());
            println("  " + reference.getFromAddress() + " " + reference.getReferenceType() + " " +
                (caller == null ? "<none>" : caller.getEntryPoint() + " " + caller.getName()));
        }
        println("instructions:");
        Listing listing = currentProgram.getListing();
        for (Instruction instruction : listing.getInstructions(function.getBody(), true)) {
            println("  " + instruction.getAddress() + " " + instruction);
        }
        DecompileResults result = decompiler.decompileFunction(function, 90, monitor);
        println("decompile=" + result.decompileCompleted() + " error=" + result.getErrorMessage());
        if (result.getDecompiledFunction() != null) {
            println(result.getDecompiledFunction().getC());
        }
    }

    @Override
    public void run() throws Exception {
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        String[] targets = {"1018:267e", "1018:ae3a", "1018:509c", "1018:adb8", "1018:aede", "1018:af08", "1020:4e8e"};
        for (String target : targets) {
            reportFunction(decompiler, target);
        }
        println("===== LITERAL 0x2714 INSTRUCTIONS =====");
        for (Instruction instruction : currentProgram.getListing().getInstructions(true)) {
            if (instruction.toString().contains("0x2714")) {
                Function function = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
                println(instruction.getAddress() + " " + instruction + " in " +
                    (function == null ? "<none>" : function.getEntryPoint() + " " + function.getName()));
            }
        }
    }
}
