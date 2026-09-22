import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;

public class Phase8ByteWindows extends GhidraScript {
    public void run() throws Exception {
        String[] starts = {"1008:e8b0", "1008:e830", "1008:cfc0", "1008:d0f0", "1020:5f30"};
        for (String text : starts) {
            Address address = currentProgram.getAddressFactory().getAddress(text);
            StringBuilder line = new StringBuilder("BYTES " + text + ":");
            for (int index = 0; index < 48; index++) {
                line.append(String.format(" %02x", getByte(address.add(index)) & 0xff));
            }
            println(line.toString());
        }
    }
}
