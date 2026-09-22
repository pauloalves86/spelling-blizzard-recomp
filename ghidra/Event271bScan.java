import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

public class Event271bScan extends GhidraScript {
    public void run() throws Exception {
        for (Instruction ins : currentProgram.getListing().getInstructions(true)) {
            if (ins.toString().contains("0x271b")) {
                Function f = currentProgram.getFunctionManager().getFunctionContaining(ins.getAddress());
                println(ins.getAddress() + " " + ins.toString() + " in " +
                    (f == null ? "<none>" : f.getEntryPoint() + " " + f.getName()));
            }
        }
    }
}
