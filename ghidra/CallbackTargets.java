import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

public class CallbackTargets extends GhidraScript {
    public void run() throws Exception {
        String[] tables = {"1008:0bc8", "1008:2810", "1008:2b04"};
        int[] slots = {0x10, 0x14, 0x18, 0x1c, 0x20, 0x2c, 0x44, 0x54, 0x68, 0x110, 0x114, 0x118, 0x11c, 0x120, 0x124, 0x128};
        for (String table : tables) {
            Address base = currentProgram.getAddressFactory().getAddress(table);
            println("TABLE " + table);
            for (int slot : slots) {
                Address a = base.add(slot);
                int offset = getByte(a) & 0xff | ((getByte(a.add(1)) & 0xff) << 8);
                int segment = getByte(a.add(2)) & 0xff | ((getByte(a.add(3)) & 0xff) << 8);
                Function f = currentProgram.getFunctionManager().getFunctionAt(
                    currentProgram.getAddressFactory().getAddress(String.format("%04x:%04x", segment, offset)));
                println(String.format("  +0x%03x %04x:%04x %s", slot, segment, offset,
                    f == null ? "<no function>" : f.getName()));
            }
        }
    }
}
