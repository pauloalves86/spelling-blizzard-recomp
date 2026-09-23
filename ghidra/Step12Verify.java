import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;

public class Step12Verify extends GhidraScript {
    private Address addr(String value) {
        return currentProgram.getAddressFactory().getAddress(value);
    }

    private void refs(String value) {
        Address target = addr(value);
        println("REFS TO " + value);
        int count = 0;
        for (Reference ref : currentProgram.getReferenceManager().getReferencesTo(target)) {
            Function owner = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
            println("  " + ref.getFromAddress() + " " + ref.getReferenceType() + " owner=" +
                (owner == null ? "<none>" : owner.getEntryPoint() + " " + owner.getName()));
            count++;
        }
        println("  count=" + count);
    }

    private void dumpWindow(String first, String last) {
        Address start = addr(first);
        Address end = addr(last);
        Listing listing = currentProgram.getListing();
        Instruction instruction = listing.getInstructionContaining(start);
        if (instruction == null) instruction = listing.getInstructionAfter(start);
        println("WINDOW " + first + ".." + last);
        while (instruction != null && instruction.getAddress().compareTo(end) <= 0) {
            println("  " + instruction.getAddress() + " " + instruction);
            instruction = listing.getInstructionAfter(instruction.getAddress());
        }
    }

    private void inspectBytes(String value, int length) throws Exception {
        Address location = addr(value);
        MemoryBlock block = currentProgram.getMemory().getBlock(location);
        Function function = currentProgram.getFunctionManager().getFunctionContaining(location);
        Instruction containing = currentProgram.getListing().getInstructionContaining(location);
        Instruction before = currentProgram.getListing().getInstructionBefore(location);
        Instruction after = currentProgram.getListing().getInstructionAfter(location);
        println("ADDRESS " + value + " block=" + (block == null ? "<none>" : block.getName()) +
            " function=" + (function == null ? "<none>" : function.getEntryPoint()) +
            " containing=" + (containing == null ? "<none>" : containing.getAddress()) +
            " before=" + (before == null ? "<none>" : before.getAddress()) +
            " after=" + (after == null ? "<none>" : after.getAddress()));
        if (block == null) return;
        byte[] bytes = new byte[length];
        int count = currentProgram.getMemory().getBytes(location, bytes);
        StringBuilder text = new StringBuilder("  BYTES");
        for (int i = 0; i < count; i++) text.append(String.format(" %02x", bytes[i] & 0xff));
        println(text.toString());
    }

    private void dumpFarRun(String startText, int bytes) throws Exception {
        Address start = addr(startText);
        println("FAR RUN " + startText + " bytes=" + bytes);
        for (int offset = 0; offset + 3 < bytes; offset += 4) {
            byte[] b = new byte[4];
            currentProgram.getMemory().getBytes(start.add(offset), b);
            int off = (b[0] & 0xff) | ((b[1] & 0xff) << 8);
            int seg = (b[2] & 0xff) | ((b[3] & 0xff) << 8);
            Address target = currentProgram.getAddressFactory().getAddress(
                String.format("%04x:%04x", seg, off));
            Function f = currentProgram.getFunctionManager().getFunctionAt(target);
            println(String.format("  +%04x %02x%02x:%02x%02x => %04x:%04x %s",
                offset, b[1] & 0xff, b[0] & 0xff, b[3] & 0xff, b[2] & 0xff,
                seg, off, f == null ? "<no function boundary>" : f.getName()));
        }
    }

    private void scanPointer(byte[] pattern) throws Exception {
        println(String.format("SCAN %02x %02x %02x %02x", pattern[0] & 0xff,
            pattern[1] & 0xff, pattern[2] & 0xff, pattern[3] & 0xff));
        int hits = 0;
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized() || block.getSize() < pattern.length) continue;
            byte[] data = new byte[(int)block.getSize()];
            int got = currentProgram.getMemory().getBytes(block.getStart(), data);
            for (int i = 0; i + 3 < got; i++) {
                if (data[i] == pattern[0] && data[i+1] == pattern[1] &&
                    data[i+2] == pattern[2] && data[i+3] == pattern[3]) {
                    println("  " + block.getStart().add(i));
                    hits++;
                }
            }
        }
        println("  hits=" + hits);
    }

    private void reportFunction(String value) {
        Address location = addr(value);
        Function f = currentProgram.getFunctionManager().getFunctionAt(location);
        println("FUNCTION " + value + " = " + (f == null ? "<none>" : f.getSignature() +
            " purge=" + f.getStackPurgeSize()));
        refs(value);
    }

    private void refsWithInstructions(String value) {
        Address target = addr(value);
        println("DATA REFS TO " + value);
        int count = 0;
        for (Reference ref : currentProgram.getReferenceManager().getReferencesTo(target)) {
            Function owner = currentProgram.getFunctionManager().getFunctionContaining(ref.getFromAddress());
            Instruction instruction = currentProgram.getListing().getInstructionContaining(ref.getFromAddress());
            println("  " + ref.getFromAddress() + " " + ref.getReferenceType() + " " +
                (instruction == null ? "<no instruction>" : instruction.toString()) + " owner=" +
                (owner == null ? "<none>" : owner.getEntryPoint() + " " + owner.getName()));
            count++;
        }
        println("  count=" + count);
    }

    @Override
    public void run() throws Exception {
        reportFunction("1018:267e");
        reportFunction("1018:509c");
        reportFunction("1010:3d56");
        reportFunction("1020:4e8e");
        reportFunction("1020:2534");
        refs("1008:97c2");
        refs("1008:9802");
        refs("1008:97e6");
        refs("1018:3230");
        refs("1018:335c");
        refs("1008:7f80");
        refs("1008:7fbc");
        refsWithInstructions("1060:01ec");
        refsWithInstructions("1060:01f0");
        dumpFarRun("1008:97c2", 0x80);
        dumpFarRun("1018:3230", 0x140);
        dumpFarRun("1008:7f80", 0x80);
        scanPointer(new byte[] {0x7e, 0x26, 0x18, 0x10});
        scanPointer(new byte[] {(byte)0x8e, 0x4e, 0x20, 0x10});
        scanPointer(new byte[] {(byte)0x9c, 0x50, 0x18, 0x10});
        scanPointer(new byte[] {0x34, 0x25, 0x20, 0x10});
        scanPointer(new byte[] {(byte)0xc2, (byte)0x97, 0x08, 0x10});
        scanPointer(new byte[] {(byte)0xe6, (byte)0x97, 0x08, 0x10});
        scanPointer(new byte[] {0x30, 0x32, 0x18, 0x10});
        scanPointer(new byte[] {(byte)0x80, 0x7f, 0x08, 0x10});
        scanPointer(new byte[] {0x66, (byte)0xa3, (byte)0xec, 0x01});
        scanPointer(new byte[] {0x66, (byte)0xc7, 0x06, (byte)0xec});
        dumpWindow("1018:5104", "1018:5138");
        dumpWindow("1018:5149", "1018:5196");
        dumpWindow("1018:51a3", "1018:51cd");
        dumpWindow("1018:5399", "1018:53c4");
        dumpWindow("1018:532b", "1018:534a");
        dumpWindow("1018:3230", "1018:3270");
        dumpWindow("1008:7fbc", "1008:7fd0");
        dumpWindow("1010:3d56", "1010:3d90");
        dumpWindow("1010:3e8d", "1010:3ecb");
        dumpWindow("1010:3f23", "1010:3f52");
        dumpWindow("1020:2534", "1020:254e");
        dumpWindow("1020:4e8e", "1020:4f14");
        dumpWindow("1020:5f29", "1020:5f58");
        inspectBytes("1020:5f20", 0x60);
        inspectBytes("1020:5f29", 0x40);
        inspectBytes("1008:7f80", 0x80);
        dumpWindow("1020:678c", "1020:6935");
    }
}
