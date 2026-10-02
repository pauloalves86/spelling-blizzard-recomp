// Dump function bodies and segment coverage data to JSON.
// Usage: analyzeHeadless <proj_dir> <proj_name> -process BLIZZARD.EXE -readOnly -noanalysis -scriptPath ghidra -postScript DumpCoverageData.java <output-json>

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import java.io.*;
import java.util.*;

public class DumpCoverageData extends GhidraScript {
  public void run() throws Exception {
    String outFile = getScriptArgs()[0];
    PrintWriter out = new PrintWriter(new FileWriter(outFile));

    String[] segNames = new String[] { "1000", "1008", "1010", "1018", "1020" };
    int[] segLens = new int[] { 9191, 61402, 61418, 61307, 28650 };

    out.println("{");
    out.println("  \"segments\": {");

    AddressFactory af = currentProgram.getAddressFactory();
    FunctionManager fm = currentProgram.getFunctionManager();
    Listing listing = currentProgram.getListing();

    for (int i = 0; i < segNames.length; i++) {
      String seg = segNames[i];
      int len = segLens[i];
      Address startAd = af.getAddress(seg + ":0000");
      Address endAd = af.getAddress(String.format("%s:%04x", seg, len - 1));
      AddressSet segSet = new AddressSet(startAd, endAd);

      AddressSet coveredSet = new AddressSet();
      FunctionIterator fit = fm.getFunctions(segSet, true);
      while (fit.hasNext()) {
        Function f = fit.next();
        AddressSetView body = f.getBody();
        // Intersect body with segSet so we only count bytes in this segment
        coveredSet.add(body.intersect(segSet));
      }

      AddressSet uncoveredSet = segSet.subtract(coveredSet);

      out.println(String.format("    \"%s\": {", seg));
      out.println(String.format("      \"length\": %d,", len));
      out.println(String.format("      \"covered_bytes\": %d,", coveredSet.getNumAddresses()));
      out.println(String.format("      \"uncovered_bytes\": %d,", uncoveredSet.getNumAddresses()));

      // covered ranges
      out.println("      \"covered_ranges\": [");
      boolean first = true;
      for (AddressRange r : coveredSet.getAddressRanges()) {
        if (!first) out.println(",");
        first = false;
        out.print(String.format("        [%d, %d]", r.getMinAddress().getOffset(), r.getMaxAddress().getOffset()));
      }
      out.println();
      out.println("      ],");

      // uncovered ranges
      out.println("      \"uncovered_ranges\": [");
      first = true;
      for (AddressRange r : uncoveredSet.getAddressRanges()) {
        if (!first) out.println(",");
        first = false;
        out.print(String.format("        [%d, %d]", r.getMinAddress().getOffset(), r.getMaxAddress().getOffset()));
      }
      out.println();
      out.println("      ]");

      if (i < segNames.length - 1) {
        out.println("    },");
      } else {
        out.println("    }");
      }
    }
    out.println("  },");

    // All functions list
    out.println("  \"functions\": [");
    FunctionIterator allFuncs = fm.getFunctions(true);
    boolean firstFunc = true;
    while (allFuncs.hasNext()) {
      Function f = allFuncs.next();
      if (!firstFunc) out.println(",");
      firstFunc = false;

      Address ep = f.getEntryPoint();
      String epStr = String.format("%04x:%04x", (int)ep.getAddressSpace().getSpaceID(), ep.getOffset());
      // Actually let's use the standard string representation
      String epFormatted = ep.toString();

      // Find ret instructions and ret operands
      List<String> rets = new ArrayList<String>();
      InstructionIterator iit = listing.getInstructions(f.getBody(), true);
      while (iit.hasNext()) {
        Instruction ins = iit.next();
        String mnem = ins.getMnemonicString();
        if (mnem.startsWith("RET")) {
          rets.add(ins.toString());
        }
      }

      out.print(String.format("    {\"name\": \"%s\", \"entry\": \"%s\", \"purge\": %d, \"body_size\": %d, \"rets\": [",
        f.getName(), epFormatted, f.getStackPurgeSize(), f.getBody().getNumAddresses()));
      for (int r = 0; r < rets.size(); r++) {
        if (r > 0) out.print(", ");
        out.print(String.format("\"%s\"", rets.get(r)));
      }
      out.print("]}");
    }
    out.println();
    out.println("  ]");
    out.println("}");
    out.close();
    println("Dump completed to " + outFile);
  }
}
