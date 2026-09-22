import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;

public class Phase6FunctionDump extends GhidraScript {
    public void run() throws Exception {
        String[] names = {"1008:ac80", "1018:5496"};
        for (String name : names) {
            Address address = currentProgram.getAddressFactory().getAddress(name);
            Function function = currentProgram.getFunctionManager().getFunctionAt(address);
            println("FUNCTION " + name + " " + function.getBody());
            for (Instruction instruction : currentProgram.getListing().getInstructions(function.getBody(), true))
                println(instruction.getAddress() + " " + instruction.toString());
        }
    }
}
