import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Reference;

public class StateVariableTrace extends GhidraScript {
    public void run() throws Exception {
        String[] names = {"1060:01de", "1060:0a36", "1060:01e4", "1060:01ec"};
        for (String name : names) {
            Address a = currentProgram.getAddressFactory().getAddress(name);
            println("DATA " + name + " " + a);
            for (Reference ref : currentProgram.getReferenceManager().getReferencesTo(a)) {
                Function f = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
                println("  " + ref.getFromAddress() + " " + ref.getReferenceType() +
                    " in " + (f == null ? "<none>" : f.getEntryPoint() + " " + f.getName()));
            }
            try {
                byte[] bytes = getBytes(a, 8);
                StringBuilder b = new StringBuilder("  BYTES");
                for (byte value : bytes) b.append(String.format(" %02x", value & 0xff));
                println(b.toString());
            } catch (Exception e) {
                println("  BYTES unavailable: " + e.getMessage());
            }
        }
    }
}
