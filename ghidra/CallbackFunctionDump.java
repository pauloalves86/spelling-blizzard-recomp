import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

public class CallbackFunctionDump extends GhidraScript {
    public void run() throws Exception {
        String[] targets = {"1008:e8b6", "1008:cf6c", "1008:cfc2", "1008:d0f2", "1008:e838"};
        for (String target : targets) {
            Address a = currentProgram.getAddressFactory().getAddress(target);
            Function f = currentProgram.getFunctionManager().getFunctionContaining(a);
            println("TARGET " + target + " FUNCTION " + (f == null ? "<none>" : f.getEntryPoint() + " " + f.getName()));
            if (f != null) for (Instruction i : currentProgram.getListing().getInstructions(f.getBody(), true)) println("  " + i.getAddress() + " " + i.toString());
        }
    }
}
