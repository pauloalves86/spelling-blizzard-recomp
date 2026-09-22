import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.address.AddressSetView;

public class Phase6CommandScan extends GhidraScript {
    public void run() throws Exception {
        Listing listing = currentProgram.getListing();
        AddressSetView set = currentProgram.getMemory().getExecuteSet();
        for (Instruction instruction : listing.getInstructions(set, true)) {
            String text = instruction.toString();
            if (text.contains("0x2af9") || text.contains("0x2afa") || text.contains("0x2afb") ||
                text.contains("0x32c9") || text.contains("0x32ca")) {
                Function function = currentProgram.getFunctionManager().getFunctionContaining(instruction.getAddress());
                println(instruction.getAddress() + " " + text + " in " +
                    (function == null ? "<none>" : function.getEntryPoint() + " " + function.getName()));
            }
        }
    }
}
