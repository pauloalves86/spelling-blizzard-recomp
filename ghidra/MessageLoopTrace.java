import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.Reference;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;

public class MessageLoopTrace extends GhidraScript {
    private Address address(String value) {
        return currentProgram.getAddressFactory().getAddress(value);
    }

    private Function function(String value) {
        return currentProgram.getFunctionManager().getFunctionAt(address(value));
    }

    private String ownerOf(Address from) {
        Function owner = currentProgram.getFunctionManager().getFunctionContaining(from);
        return owner == null ? "<none>" : owner.getEntryPoint().toString() + " " + owner.getName();
    }

    private List<Function> sortedFunctions(Set<Function> functions) {
        List<Function> result = new ArrayList<Function>(functions);
        Collections.sort(result, new Comparator<Function>() {
            @Override
            public int compare(Function left, Function right) {
                return left.getEntryPoint().compareTo(right.getEntryPoint());
            }
        });
        return result;
    }

    private void dumpWindow(Address center, int before, int after) {
        Listing listing = currentProgram.getListing();
        Instruction at = listing.getInstructionContaining(center);
        if (at == null) at = listing.getInstructionAt(center);
        if (at == null) {
            println("  WINDOW " + center + " <no instruction>");
            return;
        }
        List<Instruction> previous = new ArrayList<Instruction>();
        Instruction cursor = at;
        for (int i = 0; i < before; i++) {
            cursor = listing.getInstructionBefore(cursor.getAddress());
            if (cursor == null) break;
            previous.add(cursor);
        }
        Collections.reverse(previous);
        for (Instruction instruction : previous) println("    " + instruction);
        println("  > " + at);
        cursor = at;
        for (int i = 0; i < after; i++) {
            cursor = listing.getInstructionAfter(cursor.getAddress());
            if (cursor == null) break;
            println("    " + cursor);
        }
    }

    private void reportReferences(String label, Address target) {
        println("REFERENCES " + label + " target=" + target);
        int total = 0;
        for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(target)) {
            total++;
            println("  " + reference.getReferenceType() + " from=" + reference.getFromAddress() +
                " owner=" + ownerOf(reference.getFromAddress()));
            dumpWindow(reference.getFromAddress(), 3, 2);
        }
        println("  reference-count=" + total);
    }

    private void reportFunction(String value) {
        Function target = function(value);
        if (target == null) {
            println("FUNCTION " + value + " <no function boundary>");
            return;
        }
        println("FUNCTION " + target.getEntryPoint() + " " + target.getName() +
            " purge=" + target.getStackPurgeSize() + " signature=" + target.getSignature());
        List<Function> callees = sortedFunctions(target.getCalledFunctions(monitor));
        for (Function callee : callees) {
            println("  DIRECT-CALLEE " + callee.getEntryPoint() + " " + callee.getName());
        }
        TreeMap<String, TreeSet<String>> callers = new TreeMap<String, TreeSet<String>>();
        for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(target.getEntryPoint())) {
            if (!reference.getReferenceType().isCall()) continue;
            String owner = ownerOf(reference.getFromAddress());
            TreeSet<String> sites = callers.get(owner);
            if (sites == null) {
                sites = new TreeSet<String>();
                callers.put(owner, sites);
            }
            sites.add(reference.getFromAddress().toString());
        }
        for (Map.Entry<String, TreeSet<String>> entry : callers.entrySet()) {
            println("  DIRECT-CALLER " + entry.getKey() + " sites=" + entry.getValue());
        }
        println("  direct-caller-function-count=" + callers.size());
    }

    private void reportApi(String name, String entryText) {
        Address entry = address(entryText);
        println("USER-API " + name + " local-entry=" + entry);
        int count = 0;
        for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(entry)) {
            count++;
            println("  API-CALL " + reference.getFromAddress() + " owner=" + ownerOf(reference.getFromAddress()));
            dumpWindow(reference.getFromAddress(), 3, 2);
        }
        println("  api-callsite-count=" + count);
    }

    private int readU16(Address at) throws Exception {
        byte[] bytes = new byte[2];
        currentProgram.getMemory().getBytes(at, bytes);
        return (bytes[0] & 0xff) | ((bytes[1] & 0xff) << 8);
    }

    private void reportTableCell(String context, String base, int offset) throws Exception {
        Address cell = address(base).add(offset);
        int farOffset = readU16(cell);
        int segment = readU16(cell.add(2));
        Address target = address(String.format("%04x:%04x", segment, farOffset));
        Function targetFunction = currentProgram.getFunctionManager().getFunctionAt(target);
        Function containing = currentProgram.getFunctionManager().getFunctionContaining(target);
        String targetInfo = targetFunction == null ? "<no function boundary>" :
            targetFunction.getName() + " purge=" + targetFunction.getStackPurgeSize();
        if (targetFunction == null && containing != null) {
            targetInfo += " inside " + containing.getEntryPoint() + " " + containing.getName();
        }
        println(String.format("TABLE-CELL %s base=%s slot=+0x%x cell=%s bytes=%02x %02x %02x %02x target=%s %s",
            context, base, offset, cell, farOffset & 0xff, (farOffset >>> 8) & 0xff,
            segment & 0xff, (segment >>> 8) & 0xff, target, targetInfo));
    }

    private void dumpCallsInFunction(String value) {
        Function target = function(value);
        if (target == null) return;
        println("CALL-INSTRUCTIONS function=" + target.getEntryPoint() + " " + target.getName());
        InstructionIterator iterator = currentProgram.getListing().getInstructions(target.getBody(), true);
        while (iterator.hasNext()) {
            Instruction instruction = iterator.next();
            if (!instruction.getMnemonicString().toUpperCase().startsWith("CALL")) continue;
            println("  " + instruction + " owner=" + target.getEntryPoint());
            dumpWindow(instruction.getAddress(), 4, 2);
        }
    }

    private void dumpDecomp(String value, DecompInterface decompiler) {
        Function target = function(value);
        if (target == null) {
            println("DECOMPILE " + value + " <no function boundary>");
            return;
        }
        DecompileResults result = decompiler.decompileFunction(target, 60, monitor);
        println("DECOMPILE " + target.getEntryPoint() + " " + target.getName() +
            " completed=" + result.decompileCompleted());
        println(result.getDecompiledFunction() == null ? "<no C output>" :
            result.getDecompiledFunction().getC());
    }

    private void dumpBytes(String value, int length) throws Exception {
        Address start = address(value);
        byte[] bytes = new byte[length];
        currentProgram.getMemory().getBytes(start, bytes);
        StringBuilder text = new StringBuilder();
        for (int i = 0; i < bytes.length; i++) {
            if (i != 0) text.append(' ');
            text.append(String.format("%02x", bytes[i] & 0xff));
        }
        println("RAW-BYTES " + start + " length=" + length + " " + text);
    }

    private void findDirectPath(String startText, String targetText, int maxDepth) {
        Address targetAddress = address(targetText);
        String startKey = address(startText).toString();
        Queue<List<String>> queue = new ArrayDeque<List<String>>();
        List<String> initial = new ArrayList<String>();
        initial.add(startKey);
        queue.add(initial);
        Set<String> visited = new HashSet<String>();
        boolean found = false;
        while (!queue.isEmpty()) {
            List<String> path = queue.remove();
            String last = path.get(path.size() - 1);
            if (last.equals(targetAddress.toString())) {
                println("DIRECT-PATH " + path);
                found = true;
                continue;
            }
            if (path.size() > maxDepth || !visited.add(last)) continue;
            Function current = function(last);
            if (current == null) continue;
            for (Function callee : sortedFunctions(current.getCalledFunctions(monitor))) {
                String next = callee.getEntryPoint().toString();
                if (path.contains(next)) continue;
                List<String> nextPath = new ArrayList<String>(path);
                nextPath.add(next);
                queue.add(nextPath);
            }
        }
        if (!found) println("DIRECT-PATH " + startKey + " -> " + targetText + " <not found within depth " + maxDepth + ">");
    }

    private void reportCallersOneHop(String value) {
        Function target = function(value);
        if (target == null) return;
        println("CALLERS-AND-UPSTREAM-ONE-HOP target=" + target.getEntryPoint());
        TreeMap<String, TreeSet<String>> callerMap = new TreeMap<String, TreeSet<String>>();
        for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(target.getEntryPoint())) {
            if (!reference.getReferenceType().isCall()) continue;
            String callerKey = ownerOf(reference.getFromAddress());
            TreeSet<String> sites = callerMap.get(callerKey);
            if (sites == null) {
                sites = new TreeSet<String>();
                callerMap.put(callerKey, sites);
            }
            sites.add(reference.getFromAddress().toString());
        }
        for (String callerKey : callerMap.keySet()) {
            println("  CALLER " + callerKey + " sites=" + callerMap.get(callerKey));
            int split = callerKey.indexOf(' ');
            if (split <= 0) continue;
            Function caller = function(callerKey.substring(0, split));
            if (caller == null) continue;
            TreeSet<String> parents = new TreeSet<String>();
            for (Reference reference : currentProgram.getReferenceManager().getReferencesTo(caller.getEntryPoint())) {
                if (!reference.getReferenceType().isCall()) continue;
                parents.add(ownerOf(reference.getFromAddress()) + " @ " + reference.getFromAddress());
            }
            println("    UPSTREAM-DIRECT-CALLERS " + (parents.isEmpty() ? "<none>" : parents));
        }
    }

    @Override
    public void run() throws Exception {
        println("MESSAGE-LOOP-TRACE program=" + currentProgram.getName() +
            " language=" + currentProgram.getLanguageID());

        String[] functions = {
            "1000:001a", "1000:00d8", "1000:2026", "1008:2bf4", "1008:e412",
            "1008:ac80", "1008:0eba", "1008:6ef6", "1008:670a", "1008:3218",
            "1008:cdf0", "1008:6682", "1008:6792", "1008:76cc", "1010:29ea", "1010:07d2",
            "1008:6fa0", "1008:6dfc", "1008:783a", "1008:0596", "1008:7b9a", "1008:7bce",
            "1010:527c", "1010:553e", "1010:559e",
            "1010:5d62", "1010:5d92", "1010:3d32", "1010:3d52", "1010:530a",
            "1010:5348", "1010:57a6", "1010:5824",
            "1010:5bd8",
            "1010:5c9e", "1010:5f3c",
            "1010:07fa", "1010:08bc", "1010:3d56", "1010:1cde", "1010:519a",
            "1008:e286", "1008:9222", "1010:08e0", "1010:5d96", "1010:50ce",
            "1010:53c8", "1010:5722", "1010:596e", "1010:5b18", "1018:509c",
            "1018:5496", "1010:2a74", "1010:2a7e", "1010:5db2"
        };
        for (String item : functions) reportFunction(item);

        String[][] apis = {
            {"PEEKMESSAGE", "10a0:018c"}, {"TRANSLATEMESSAGE", "10a0:0194"},
            {"DISPATCHMESSAGE", "10a0:0198"}, {"POSTMESSAGE", "10a0:0190"},
            {"INVALIDATERECT", "10a0:019c"}, {"VALIDATERECT", "10a0:01a0"},
            {"DEFWINDOWPROC", "10a0:0188"}, {"REGISTERCLASS", "10a0:0154"},
            {"BEGINPAINT", "10a0:0140"}, {"ENDPAINT", "10a0:0144"},
            {"CREATEWINDOW", "10a0:0148"}, {"SHOWWINDOW", "10a0:014c"},
            {"POSTQUITMESSAGE", "10a0:0128"}, {"GETWINDOWLONG", "10a0:01a8"},
            {"SETWINDOWLONG", "10a0:01ac"}, {"GETDC", "10a0:0158"}
        };
        for (String[] api : apis) reportApi(api[0], api[1]);

        reportReferences("1008:e412 entry", address("1008:e412"));
        reportCallersOneHop("1008:e412");
        reportCallersOneHop("1008:2bf4");
        reportCallersOneHop("1010:596e");
        reportCallersOneHop("1010:5b18");
        reportCallersOneHop("1008:3218");
        reportCallersOneHop("1008:cdf0");
        reportCallersOneHop("1008:6682");
        reportCallersOneHop("1008:6792");
        reportCallersOneHop("1010:5d96");

        println("STARTUP-TO-TARGET DIRECT-CALL PATHS (indirect callbacks excluded)");
        String[] pathTargets = {"1008:2bf4", "1010:596e", "1010:5b18", "1008:e412",
            "1010:07fa", "1010:08bc", "1010:3d56", "1008:ac80"};
        for (String pathTarget : pathTargets) findDirectPath("1000:001a", pathTarget, 10);

        println("STARTUP-INITIALIZATION TABLE CELLS");
        reportTableCell("DAT_1060:0a14 first dispatch", "1008:2da6", 0x00);
        reportTableCell("DAT_1060:07c4 init slot", "1008:2e32", 0x58);
        reportTableCell("DAT_1060:07c4 init slot", "1008:2e32", 0x84);
        reportTableCell("DAT_1060:07c4 init slot", "1008:2e32", 0x98);
        reportTableCell("DAT_1060:07c4 paint/update dispatch", "1008:2e32", 0x48);
        reportTableCell("DAT_1060:07c4 paint/update dispatch", "1008:2e32", 0x4c);
        reportTableCell("DAT_1060:07c8 initialized at 1008:72b6", "1008:7edc", 0x00);
        reportTableCell("DAT_1060:07c8 initialized at 1008:72b6", "1008:7edc", 0x18);
        reportTableCell("DAT_1060:07c8 initialized at 1008:72b6", "1008:7edc", 0xd4);
        reportTableCell("DAT_1060:07c8 initialized at 1008:72b6", "1008:7edc", 0xdc);
        reportTableCell("DAT_1060:07c4 table", "1008:2e32", 0x78);
        reportTableCell("DAT_1060:07c4 table", "1008:2e32", 0x7c);
        println("WNDPROC MESSAGE AND PAINT VTABLE CELLS");
        reportTableCell("1008:0eba window table", "1008:0f80", 0x0c);
        reportTableCell("1008:0eba window table", "1008:0f80", 0x10);
        reportTableCell("1008:670a window table", "1008:681a", 0x0c);
        reportTableCell("1008:670a window table", "1008:681a", 0x10);
        reportTableCell("1008:6ef6 window table", "1008:705e", 0x0c);
        reportTableCell("1008:6ef6 window table", "1008:705e", 0x10);
        reportTableCell("1010:50ce window table", "1010:58ea", 0x0c);
        reportTableCell("1010:50ce window table", "1010:58ea", 0x10);
        reportTableCell("1010:53c8 window table", "1010:5942", 0x0c);
        reportTableCell("1010:53c8 window table", "1010:5942", 0x10);
        reportTableCell("1010:5722 window table", "1010:5916", 0x0c);
        reportTableCell("1010:5722 window table", "1010:5916", 0x10);
        reportTableCell("1010:596e temporary table", "1010:5fee", 0x0c);
        reportTableCell("1010:596e temporary table", "1010:5fee", 0x10);
        println("WINDOW-CLASS INITIALIZER AND POST-CREATE VTABLE CELLS");
        reportTableCell("1008:0eba final table", "1008:0f80", 0x14);
        reportTableCell("1008:0eba final table", "1008:0f80", 0x18);
        reportTableCell("1008:670a final table", "1008:681a", 0x14);
        reportTableCell("1008:670a final table", "1008:681a", 0x18);
        reportTableCell("1008:6ef6 final table", "1008:705e", 0x14);
        reportTableCell("1008:6ef6 final table", "1008:705e", 0x18);
        reportTableCell("1010:50ce table", "1010:58ea", 0x14);
        reportTableCell("1010:50ce table", "1010:58ea", 0x18);
        reportTableCell("1010:53c8 final table", "1010:5942", 0x14);
        reportTableCell("1010:53c8 final table", "1010:5942", 0x18);
        reportTableCell("1010:5722 final table", "1010:5916", 0x14);
        reportTableCell("1010:5722 final table", "1010:5916", 0x18);
        reportTableCell("1010:596e conditional class table", "1010:5fee", 0x14);
        reportTableCell("1010:596e conditional class table", "1010:5fee", 0x18);
        dumpWindow(address("1008:0694"), 3, 12);
        dumpWindow(address("1008:72b6"), 3, 12);
        dumpWindow(address("1008:0f34"), 3, 12);
        dumpWindow(address("1008:67de"), 3, 12);
        dumpWindow(address("1010:55fe"), 3, 12);
        dumpWindow(address("1010:5d66"), 3, 12);
        dumpWindow(address("1010:5db2"), 3, 12);
        dumpWindow(address("1010:6002"), 3, 12);
        dumpWindow(address("1010:6006"), 3, 12);
        dumpWindow(address("1010:527c"), 3, 12);
        dumpWindow(address("1010:553e"), 3, 12);
        dumpWindow(address("1010:5d62"), 3, 12);
        dumpWindow(address("1010:5d92"), 3, 12);
        dumpWindow(address("1010:3d32"), 3, 12);
        dumpWindow(address("1010:3d52"), 3, 12);
        dumpBytes("1008:0694", 128);
        dumpBytes("1008:72b6", 128);
        dumpBytes("1008:783a", 128);
        dumpBytes("1008:0596", 128);
        dumpBytes("1008:7b9a", 96);
        dumpBytes("1008:7bce", 64);
        dumpBytes("1008:0f34", 16);
        dumpBytes("1008:67de", 16);
        dumpBytes("1008:6fa0", 16);
        dumpBytes("1010:5348", 16);
        dumpBytes("1010:55fe", 16);
        dumpBytes("1010:5824", 16);
        dumpBytes("1010:5d66", 16);
        dumpBytes("1010:5db2", 0x18a);
        dumpBytes("1010:6002", 32);
        dumpBytes("1010:6006", 32);
        dumpBytes("1010:527c", 128);
        dumpBytes("1010:553e", 128);
        dumpBytes("1010:5d62", 32);
        dumpBytes("1010:5d92", 32);
        dumpBytes("1010:07d2", 64);
        dumpBytes("1010:3d32", 64);
        dumpBytes("1010:3d52", 64);

        dumpCallsInFunction("1008:2bf4");
        dumpCallsInFunction("1008:ac80");
        dumpCallsInFunction("1008:3218");
        dumpCallsInFunction("1008:cdf0");
        dumpCallsInFunction("1008:6682");
        dumpCallsInFunction("1008:6792");
        dumpCallsInFunction("1008:76cc");
        dumpCallsInFunction("1008:6dfc");
        dumpCallsInFunction("1010:559e");
        dumpCallsInFunction("1010:29ea");
        dumpCallsInFunction("1010:1cde");
        dumpCallsInFunction("1010:519a");
        dumpCallsInFunction("1010:5d96");
        dumpCallsInFunction("1010:5bd8");
        dumpCallsInFunction("1010:50ce");
        dumpCallsInFunction("1010:53c8");
        dumpCallsInFunction("1010:5722");
        dumpCallsInFunction("1010:596e");
        dumpCallsInFunction("1010:5b18");

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        String[] decompileAddresses = {"1008:2bf4", "1010:29ea", "1010:07d2", "1008:76cc",
            "1008:783a", "1008:0596", "1008:7bce",
            "1008:6fa0", "1008:6dfc", "1010:559e", "1010:5348", "1010:5824",
            "1010:5bd8", "1010:5c9e",
            "1010:5d96", "1010:5f3c", "1010:3d56", "1010:519a", "1010:1cde",
            "1018:5496"};
        for (String decompileAddress : decompileAddresses) {
            dumpDecomp(decompileAddress, decompiler);
        }
        decompiler.dispose();
    }
}
