import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.Reference;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

/** Read-only ownership/initializer checks for the Step 13 table candidates. */
public class Step13Verify extends GhidraScript {
    private static final String[] BASES = {
        "1018:3230", "1008:97c2", "1008:7f80"
    };
    private static final int[] BASE_OFFSETS = {0x3230, 0x97c2, 0x7f80};
    private static final int[] BASE_SEGMENTS = {0x1018, 0x1008, 0x1008};
    private static final String[] ENTRY_TARGETS = {
        "1018:509c", "1020:4e8e", "1018:267e", "1020:2534"
    };
    private static final String[] ENTRY_CELLS = {
        "1018:3248", "1018:335c", "1008:9802", "1008:7fbc"
    };
    private static final int[] ENTRY_OFFSETS = {0x18, 0x12c, 0x40, 0x3c};
    private static final int[] RUN_SIZES = {0x140, 0x140, 0x80};

    private Address addr(String value) {
        return currentProgram.getAddressFactory().getAddress(value);
    }

    private String functionDescription(Address target) {
        Function function = currentProgram.getFunctionManager().getFunctionAt(target);
        if (function == null) {
            return "<no function boundary>";
        }
        return function.getName() + " signature=" + function.getSignature() +
            " purge=" + function.getStackPurgeSize();
    }

    private void refs(String label, String value) {
        Address target = addr(value);
        int count = 0;
        println("REFS " + label + " " + value);
        for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(target)) {
            Function owner = currentProgram.getFunctionManager().getFunctionContaining(
                reference.getFromAddress());
            Instruction instruction = currentProgram.getListing().getInstructionContaining(
                reference.getFromAddress());
            println("  " + reference.getFromAddress() + " " + reference.getReferenceType() +
                " owner=" + (owner == null ? "<none>" : owner.getEntryPoint()) +
                " instruction=" + (instruction == null ? "<none>" : instruction));
            count++;
        }
        println("  count=" + count);
    }

    private void reportFunction(String value) {
        Address target = addr(value);
        Function function = currentProgram.getFunctionManager().getFunctionAt(target);
        println("FUNCTION " + value + " " + functionDescription(target));
        refs("function-entry", value);
    }

    private void dumpRawRun(String base, int length) throws Exception {
        Address start = addr(base);
        MemoryBlock block = currentProgram.getMemory().getBlock(start);
        println("RAW-RUN " + base + " window=0x" + Integer.toHexString(length) +
            " block=" + (block == null ? "<none>" : block.getName()));
        if (block == null || !block.isInitialized()) {
            return;
        }
        for (int offset = 0; offset + 3 < length; offset += 4) {
            byte[] bytes = new byte[4];
            currentProgram.getMemory().getBytes(start.add(offset), bytes);
            int targetOffset = (bytes[0] & 0xff) | ((bytes[1] & 0xff) << 8);
            int targetSegment = (bytes[2] & 0xff) | ((bytes[3] & 0xff) << 8);
            Address target = currentProgram.getAddressFactory().getAddress(
                String.format("%04x:%04x", targetSegment, targetOffset));
            Address cell = start.add(offset);
            Data data = currentProgram.getListing().getDataAt(cell);
            println(String.format("  +%04x cell=%s raw=%02x %02x %02x %02x target=%04x:%04x %s data=%s",
                offset, cell, bytes[0] & 0xff, bytes[1] & 0xff,
                bytes[2] & 0xff, bytes[3] & 0xff, targetSegment, targetOffset,
                functionDescription(target), data == null ? "<undefined>" : data.toString()));
        }
    }

    private void scanPattern(byte[] pattern, String label) throws Exception {
        println("SCAN-EXACT " + label + " bytes=" + hex(pattern));
        int count = 0;
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized() || block.getSize() < pattern.length ||
                block.getSize() > Integer.MAX_VALUE) {
                continue;
            }
            byte[] data = new byte[(int) block.getSize()];
            int read = currentProgram.getMemory().getBytes(block.getStart(), data);
            for (int i = 0; i + pattern.length <= read; i++) {
                boolean match = true;
                for (int j = 0; j < pattern.length; j++) {
                    if (data[i + j] != pattern[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    println("  " + block.getStart().add(i) + " block=" + block.getName());
                    count++;
                }
            }
        }
        println("  count=" + count);
    }

    private void scanWord(int value) throws Exception {
        byte low = (byte) (value & 0xff);
        byte high = (byte) ((value >>> 8) & 0xff);
        println(String.format("SCAN-WORD 0x%04x little-endian", value));
        int count = 0;
        int displayed = 0;
        for (MemoryBlock block : currentProgram.getMemory().getBlocks()) {
            if (!block.isInitialized() || block.getSize() < 2 ||
                block.getSize() > Integer.MAX_VALUE) {
                continue;
            }
            byte[] data = new byte[(int) block.getSize()];
            int read = currentProgram.getMemory().getBytes(block.getStart(), data);
            for (int i = 0; i + 1 < read; i++) {
                if (data[i] != low || data[i + 1] != high) {
                    continue;
                }
                Address hit = block.getStart().add(i);
                count++;
                if (displayed < 10) {
                    Instruction instruction = currentProgram.getListing().getInstructionContaining(hit);
                    Function function = currentProgram.getFunctionManager().getFunctionContaining(hit);
                    println("  " + hit + " block=" + block.getName() +
                        " instruction=" + (instruction == null ? "<none>" : instruction) +
                        " function=" + (function == null ? "<none>" : function.getEntryPoint()));
                    displayed++;
                }
            }
        }
        println("  count=" + count + " displayed=" + displayed);
    }

    private boolean hasScalar16(Instruction instruction, int wanted) {
        for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
            for (Object object : instruction.getOpObjects(operand)) {
                if (object instanceof Scalar && ((((Scalar) object).getValue() & 0xffffL) ==
                    (wanted & 0xffffL))) {
                    return true;
                }
            }
        }
        return false;
    }

    private boolean hasAnyScalar(Instruction instruction) {
        for (int operand = 0; operand < instruction.getNumOperands(); operand++) {
            for (Object object : instruction.getOpObjects(operand)) {
                if (object instanceof Scalar) {
                    return true;
                }
            }
        }
        return false;
    }

    private void scanCandidateLiterals() {
        Listing listing = currentProgram.getListing();
        Map<String, Set<String>> functionsWithOffsetLiteral = new LinkedHashMap<>();
        Map<String, List<String>> offsetSites = new LinkedHashMap<>();
        for (String base : BASES) {
            functionsWithOffsetLiteral.put(base, new LinkedHashSet<String>());
            offsetSites.put(base, new ArrayList<String>());
        }

        InstructionIterator iterator = listing.getInstructions(true);
        while (iterator.hasNext()) {
            Instruction instruction = iterator.next();
            Function function = currentProgram.getFunctionManager().getFunctionContaining(
                instruction.getAddress());
            String owner = function == null ? "<none>" : function.getEntryPoint().toString();
            for (int i = 0; i < BASES.length; i++) {
                if (hasScalar16(instruction, BASE_OFFSETS[i])) {
                    functionsWithOffsetLiteral.get(BASES[i]).add(owner);
                    offsetSites.get(BASES[i]).add(instruction.getAddress() + " owner=" + owner +
                        " :: " + instruction);
                }
            }
        }

        println("CANDIDATE-OFFSET IMMEDIATE SCAN (instruction scalar operands, low 16 bits)");
        for (int i = 0; i < BASES.length; i++) {
            List<String> sites = offsetSites.get(BASES[i]);
            println("  base=" + BASES[i] + " offset=0x" +
                Integer.toHexString(BASE_OFFSETS[i]) + " count=" + sites.size());
            for (String site : sites) {
                println("    " + site);
            }
        }

        for (int i = 0; i < BASES.length; i++) {
            Set<String> owners = functionsWithOffsetLiteral.get(BASES[i]);
            int alternateSegment = BASE_SEGMENTS[i] == 0x1018 ? 0x1008 : 0x1018;
            println("SAME-FUNCTION SEGMENT-LITERAL CONTEXT (co-occurrence only; not a pointer-pair proof) " +
                "base=" + BASES[i] + " expected-segment=0x" +
                Integer.toHexString(BASE_SEGMENTS[i]) + " alternate-segment=0x" +
                Integer.toHexString(alternateSegment) + " offset-owner-count=" + owners.size());
            if (owners.isEmpty()) {
                println("  no instruction function contained an offset literal");
                continue;
            }
            InstructionIterator pairedIterator = listing.getInstructions(true);
            int count = 0;
            while (pairedIterator.hasNext()) {
                Instruction instruction = pairedIterator.next();
                Function function = currentProgram.getFunctionManager().getFunctionContaining(
                    instruction.getAddress());
                String owner = function == null ? "<none>" : function.getEntryPoint().toString();
                if (!owners.contains(owner)) {
                    continue;
                }
                if (hasScalar16(instruction, BASE_OFFSETS[i]) ||
                    hasScalar16(instruction, BASE_SEGMENTS[i]) ||
                    hasScalar16(instruction, alternateSegment)) {
                    println("  " + instruction.getAddress() + " owner=" + owner +
                        " :: " + instruction);
                    count++;
                }
            }
            println("  same-function-context-instruction-count=" + count);
        }
    }

    private void dumpWindow(String first, String last) {
        Address start = addr(first);
        Address end = addr(last);
        Listing listing = currentProgram.getListing();
        Instruction instruction = listing.getInstructionContaining(start);
        if (instruction == null) instruction = listing.getInstructionAfter(start);
        println("INSTRUCTION-WINDOW " + first + ".." + last);
        while (instruction != null && instruction.getAddress().compareTo(end) <= 0) {
            println("  " + instruction.getAddress() + " " + instruction);
            instruction = listing.getInstructionAfter(instruction.getAddress());
        }
    }

    private void reportData(String value) {
        Address location = addr(value);
        Data data = currentProgram.getListing().getDataAt(location);
        MemoryBlock block = currentProgram.getMemory().getBlock(location);
        println("DATA-ANCHOR " + value + " block=" +
            (block == null ? "<none>" : block.getName()) + " data=" +
            (data == null ? "<undefined>" : data.toString()));
        refs("data-anchor", value);
    }

    private void dumpBytes(String value, int length) throws Exception {
        Address location = addr(value);
        MemoryBlock block = currentProgram.getMemory().getBlock(location);
        println("MEMORY-BYTES " + value + " length=0x" + Integer.toHexString(length) +
            " block=" + (block == null ? "<none>" : block.getName()));
        if (block == null || !block.isInitialized()) return;
        byte[] bytes = new byte[length];
        int count = currentProgram.getMemory().getBytes(location, bytes);
        StringBuilder result = new StringBuilder("  ");
        for (int i = 0; i < count; i++) {
            if (i != 0) result.append(' ');
            result.append(String.format("%02x", bytes[i] & 0xff));
        }
        println(result.toString());
    }

    private void dumpConstructor(String value, int maxLines) {
        Address entry = addr(value);
        Function function = currentProgram.getFunctionManager().getFunctionAt(entry);
        println("CONSTRUCTOR-CANDIDATE " + value + " " +
            (function == null ? "<no function>" : function.getSignature() +
                " purge=" + function.getStackPurgeSize()));
        refs("constructor-entry", value);
        if (function == null) {
            return;
        }
        InstructionIterator iterator = currentProgram.getListing().getInstructions(
            function.getBody(), true);
        int lines = 0;
        int calls = 0;
        int immediateMoves = 0;
        while (iterator.hasNext()) {
            Instruction instruction = iterator.next();
            String mnemonic = instruction.getMnemonicString().toUpperCase();
            if (mnemonic.startsWith("CALL")) {
                calls++;
                println("  CALL " + instruction.getAddress() + " " + instruction);
                lines++;
            } else if (mnemonic.startsWith("MOV") && hasAnyScalar(instruction)) {
                immediateMoves++;
                println("  MOV-IMMEDIATE " + instruction.getAddress() + " " + instruction);
                lines++;
            }
            if (lines >= maxLines) {
                println("  output capped at " + maxLines + " selected instructions");
                break;
            }
        }
        println("  selected-calls=" + calls + " immediate-moves=" + immediateMoves);
    }

    private void scanIndirectCallsByOffset() {
        int[] offsets = {0x18, 0x1c, 0x3c, 0x40, 0x12c};
        Set<String> focusedOwners = new LinkedHashSet<String>();
        focusedOwners.add("1018:267e");
        focusedOwners.add("1018:509c");
        focusedOwners.add("1020:4e8e");
        focusedOwners.add("1020:2534");
        focusedOwners.add("1010:3d56");
        Pattern[] patterns = new Pattern[offsets.length];
        for (int i = 0; i < offsets.length; i++) {
            patterns[i] = Pattern.compile("\\+\\s*0x0*" +
                Integer.toHexString(offsets[i]) + "\\b", Pattern.CASE_INSENSITIVE);
        }
        int[] counts = new int[offsets.length];
        println("INDIRECT-CALL DISPLACEMENT INVENTORY (candidate slots only; not owner proof)");
        InstructionIterator iterator = currentProgram.getListing().getInstructions(true);
        while (iterator.hasNext()) {
            Instruction instruction = iterator.next();
            if (!instruction.getMnemonicString().toUpperCase().startsWith("CALL") ||
                !instruction.toString().contains("[")) {
                continue;
            }
            String text = instruction.toString();
            for (int i = 0; i < patterns.length; i++) {
                if (patterns[i].matcher(text).find()) {
                    Function owner = currentProgram.getFunctionManager().getFunctionContaining(
                        instruction.getAddress());
                    String ownerEntry = owner == null ? "<none>" : owner.getEntryPoint().toString();
                    if (offsets[i] == 0x12c || focusedOwners.contains(ownerEntry)) {
                        println(String.format("  slot=0x%x %s owner=%s :: %s", offsets[i],
                            instruction.getAddress(), ownerEntry, instruction));
                    }
                    counts[i]++;
                }
            }
        }
        for (int i = 0; i < offsets.length; i++) {
            println(String.format("  slot=0x%x indirect-call-count=%d", offsets[i], counts[i]));
        }
    }

    private String hex(byte[] bytes) {
        StringBuilder result = new StringBuilder();
        for (byte value : bytes) {
            if (result.length() != 0) result.append(' ');
            result.append(String.format("%02x", value & 0xff));
        }
        return result.toString();
    }

    @Override
    public void run() throws Exception {
        println("STEP13-VERIFY program=" + currentProgram.getName() +
            " language=" + currentProgram.getLanguageID());

        for (int i = 0; i < BASES.length; i++) {
            println("CANDIDATE-BASE " + BASES[i] + " expected-offset=0x" +
                Integer.toHexString(BASE_OFFSETS[i]) + " expected-segment=0x" +
                Integer.toHexString(BASE_SEGMENTS[i]));
            refs("table-base", BASES[i]);
        }
        for (int i = 0; i < ENTRY_CELLS.length; i++) {
            println("CANDIDATE-ENTRY cell=" + ENTRY_CELLS[i] + " base=" +
                BASES[i == 0 || i == 1 ? 0 : i == 2 ? 1 : 2] + " slot=0x" +
                Integer.toHexString(ENTRY_OFFSETS[i]) + " target=" + ENTRY_TARGETS[i] +
                " target-function=" + functionDescription(addr(ENTRY_TARGETS[i])));
            refs("entry-cell", ENTRY_CELLS[i]);
            reportFunction(ENTRY_TARGETS[i]);
        }

        dumpRawRun("1018:3230", RUN_SIZES[0]);
        dumpRawRun("1008:97c2", RUN_SIZES[1]);
        dumpRawRun("1008:7f80", RUN_SIZES[2]);

        scanPattern(new byte[] {0x30, 0x32, 0x18, 0x10}, "far pointer to 1018:3230");
        scanPattern(new byte[] {(byte) 0xc2, (byte) 0x97, 0x08, 0x10},
            "far pointer to 1008:97c2");
        scanPattern(new byte[] {(byte) 0x80, 0x7f, 0x08, 0x10},
            "far pointer to 1008:7f80");
        scanPattern(new byte[] {(byte) 0x9c, 0x50, 0x18, 0x10},
            "far pointer to 1018:509c");
        scanPattern(new byte[] {(byte) 0x8e, 0x4e, 0x20, 0x10},
            "far pointer to 1020:4e8e");
        scanPattern(new byte[] {0x7e, 0x26, 0x18, 0x10},
            "far pointer to 1018:267e");
        scanPattern(new byte[] {0x34, 0x25, 0x20, 0x10},
            "far pointer to 1020:2534");

        scanWord(0x3230);
        scanWord(0x97c2);
        scanWord(0x7f80);
        dumpBytes("1060:06d4", 0x20);
        dumpBytes("1060:0727", 0x20);
        dumpBytes("1060:075e", 0x20);
        scanCandidateLiterals();
        reportData("1018:7f80");
        dumpWindow("1018:74d8", "1018:74f8");

        reportFunction("1008:2bf4");
        reportFunction("1008:ac80");
        reportFunction("1018:649a");
        reportFunction("1008:0eba");
        dumpConstructor("1008:2bf4", 100);
        dumpConstructor("1008:ac80", 120);
        dumpConstructor("1018:649a", 100);
        dumpConstructor("1008:0eba", 60);

        scanIndirectCallsByOffset();
        reportFunction("1018:509c");
        reportFunction("1020:4e8e");
        reportFunction("1018:267e");
        reportFunction("1020:2534");
        reportFunction("1010:3d56");
    }
}
