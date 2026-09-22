import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

public class NestedObjectTrace extends GhidraScript {
    public void run() throws Exception {
        String[] needles = {"0x250", "0x252", "0x254", "0x256"};
        for (Instruction ins : currentProgram.getListing().getInstructions(true)) {
            String text = ins.toString();
            boolean hit = false;
            for (String needle : needles) if (text.contains(needle)) hit = true;
            if (hit) {
                Function f = currentProgram.getFunctionManager().getFunctionContaining(ins.getAddress());
                println(ins.getAddress() + " " + text + " in " +
                    (f == null ? "<none>" : f.getEntryPoint() + " " + f.getName()));
            }
        }
    }
}
