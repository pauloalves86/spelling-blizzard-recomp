import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

public class Field06Trace extends GhidraScript {
    public void run() throws Exception {
        for (Instruction ins : currentProgram.getListing().getInstructions(true)) {
            String text = ins.toString();
            if (text.contains("+ 0x6]") || text.contains("+ 0x6 ") || text.contains("+ 0x06")) {
                Function f = currentProgram.getFunctionManager().getFunctionContaining(ins.getAddress());
                println(ins.getAddress() + " " + text + " in " +
                    (f == null ? "<none>" : f.getEntryPoint() + " " + f.getName()));
            }
        }
    }
}
