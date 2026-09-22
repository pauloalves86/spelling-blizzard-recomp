import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;

public class CallbackLinearDump extends GhidraScript {
    public void run() throws Exception {
        String[] targets = {"1008:cfc2", "1008:d0f2", "1008:e838", "1008:e8b6"};
        for (String target : targets) {
            Address a = currentProgram.getAddressFactory().getAddress(target);
            println("TARGET " + target);
            Instruction i = currentProgram.getListing().getInstructionAt(a);
            for (int n = 0; i != null && n < 80; n++) {
                println("  " + i.getAddress() + " " + i.toString());
                if (i.toString().startsWith("RETF") || i.toString().startsWith("RET ")) break;
                i = currentProgram.getListing().getInstructionAfter(i.getAddress());
            }
        }
    }
}
