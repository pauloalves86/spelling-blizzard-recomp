// Step 15: read-only. For each address of a raw `9A` far-call instruction given as a
// script argument, read the relocated 16:16 target from Ghidra's memory image and
// print the symbol/function there. Works where Ghidra has not disassembled the code.
// Input: BLIZZARD.EXE in ghidra/SpellingBlizzard.gpr. Run with -readOnly -noanalysis.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.symbol.*;
import ghidra.program.model.listing.*;
public class Step15Refs extends GhidraScript {
  public void run() throws Exception {
    Memory m = currentProgram.getMemory();
    for (String s : getScriptArgs()) {
      Address a = currentProgram.getAddressFactory().getAddress(s);
      int op = m.getByte(a) & 0xff;
      int off = m.getShort(a.add(1)) & 0xffff, seg = m.getShort(a.add(3)) & 0xffff;
      Address t = currentProgram.getAddressFactory().getAddress(String.format("%04x:%04x", seg, off));
      Symbol sym = t == null ? null : currentProgram.getSymbolTable().getPrimarySymbol(t);
      Function f = t == null ? null : currentProgram.getFunctionManager().getFunctionAt(t);
      String thunk = (f != null && f.isThunk() && f.getThunkedFunction(true) != null) ? f.getThunkedFunction(true).getName(true) : "";
      println(String.format("CALL %s op=%02x -> %04x:%04x %s %s", s, op, seg, off, sym == null ? "<no symbol>" : sym.getName(true), thunk));
    }
  }
}
