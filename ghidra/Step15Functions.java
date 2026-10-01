// Step 15: read-only. For each address in the file given as the first script
// argument, print "<address> <containing function entry|-> <stack purge|->".
// Input: BLIZZARD.EXE in ghidra/SpellingBlizzard.gpr. Run with -readOnly -noanalysis.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.nio.file.*;
import java.io.PrintWriter;
public class Step15Functions extends GhidraScript {
  public void run() throws Exception {
    String[] a = getScriptArgs();
    try (PrintWriter out = new PrintWriter(a[1])) {
      for (String s : Files.readAllLines(Paths.get(a[0]))) {
        s = s.trim(); if (s.isEmpty()) continue;
        Address ad = currentProgram.getAddressFactory().getAddress(s);
        Function f = currentProgram.getFunctionManager().getFunctionContaining(ad);
        out.println(s + " " + (f == null ? "- -" : f.getEntryPoint() + " " + f.getStackPurgeSize()));
      }
    }
  }
}
