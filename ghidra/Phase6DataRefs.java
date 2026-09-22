import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.symbol.Reference;

public class Phase6DataRefs extends GhidraScript {
    public void run() throws Exception {
        String[] names = {"1060:01d8", "1060:01de", "1060:01e2", "1060:01e4", "1060:01ec", "1060:0a36", "1060:0a38", "1060:0a46", "1060:0a3c"};
        for (String name : names) {
            Address address = currentProgram.getAddressFactory().getAddress(name);
            println("DATA " + name + " " + address);
            for (Reference ref : currentProgram.getReferenceManager().getReferencesTo(address)) {
                Function function = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
                Instruction instruction = currentProgram.getListing().getInstructionAt(ref.getFromAddress());
                println(" REF " + ref.getFromAddress() + " " + ref.getReferenceType()
                    + " in " + (function == null ? "<none>" : function.getEntryPoint() + " " + function.getName())
                    + " :: " + (instruction == null ? "<none>" : instruction.toString()));
            }
        }
    }
}
