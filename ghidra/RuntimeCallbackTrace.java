import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;

public class RuntimeCallbackTrace extends GhidraScript {
    public void run() throws Exception {
        String[] addresses = {"1020:5f20", "1008:e8b6", "1008:e838", "1008:cfc2", "1008:d0f2"};
        for (String text : addresses) {
            Address start = currentProgram.getAddressFactory().getAddress(text);
            println("AT " + text);
            Instruction instruction = currentProgram.getListing().getInstructionContaining(start);
            if (instruction == null) instruction = currentProgram.getListing().getInstructionAfter(start);
            for (int i = 0; instruction != null && i < 48; i++) {
                println("  " + instruction.getAddress() + " " + instruction);
                instruction = currentProgram.getListing().getInstructionAfter(instruction.getAddress());
            }
        }
    }
}
