// Batch export all functions in BLIZZARD.EXE with decompiled C, callers, callees, and data refs.
// Usage: analyzeHeadless ghidra/local SpellingBlizzard -process BLIZZARD.EXE -readOnly -noanalysis -scriptPath ghidra -postScript ExportAllDecomp.java build/decomp

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.*;
import java.io.*;
import java.util.*;

public class ExportAllDecomp extends GhidraScript {

  private static String jsonEscape(String s) {
    if (s == null) return "";
    StringBuilder sb = new StringBuilder(s.length() + 16);
    for (int i = 0; i < s.length(); i++) {
      char c = s.charAt(i);
      switch (c) {
        case '"': sb.append("\\\""); break;
        case '\\': sb.append("\\\\"); break;
        case '\b': sb.append("\\b"); break;
        case '\f': sb.append("\\f"); break;
        case '\n': sb.append("\\n"); break;
        case '\r': sb.append("\\r"); break;
        case '\t': sb.append("\\t"); break;
        default:
          if (c < 0x20 || c > 0x7e) {
            sb.append(String.format("\\u%04x", (int) c));
          } else {
            sb.append(c);
          }
      }
    }
    return sb.toString();
  }

  public void run() throws Exception {
    String[] args = getScriptArgs();
    String outDirPath = args.length > 0 ? args[0] : "build/decomp";
    File outDir = new File(outDirPath);
    if (!outDir.exists()) {
      outDir.mkdirs();
    }

    FunctionManager fm = currentProgram.getFunctionManager();
    ReferenceManager rm = currentProgram.getReferenceManager();
    Listing listing = currentProgram.getListing();
    Memory mem = currentProgram.getMemory();

    DecompInterface decomp = new DecompInterface();
    decomp.openProgram(currentProgram);

    Map<String, PrintWriter> segWriters = new LinkedHashMap<>();
    String[] codeSegs = new String[] { "1000", "1008", "1010", "1018", "1020" };
    for (String seg : codeSegs) {
      PrintWriter pw = new PrintWriter(new FileWriter(new File(outDir, "seg_" + seg + ".c")));
      pw.println("/* Auto-exported Ghidra decompilation for segment " + seg + " of original/BLIZZARD.EXE */");
      pw.println();
      segWriters.put(seg, pw);
    }
    PrintWriter otherWriter = new PrintWriter(new FileWriter(new File(outDir, "seg_imports.c")));
    otherWriter.println("/* Auto-exported Ghidra decompilation for import/thunk functions */");
    otherWriter.println();

    PrintWriter jsonOut = new PrintWriter(new BufferedWriter(new FileWriter(new File(outDir, "decomp_index.json"))));
    jsonOut.println("{");
    jsonOut.println("  \"functions\": [");

    FunctionIterator fit = fm.getFunctions(true);
    boolean firstFunc = true;
    int count = 0;

    while (fit.hasNext() && !monitor.isCancelled()) {
      Function f = fit.next();
      Address ep = f.getEntryPoint();
      String epStr = ep.toString();
      String segPrefix = epStr.contains(":") ? epStr.substring(0, epStr.indexOf(':')) : "other";
      String name = f.getName();
      int bodySize = (int) f.getBody().getNumAddresses();
      int purge = f.getStackPurgeSize();
      String conv = f.getCallingConventionName();
      String sig = f.getPrototypeString(false, false);

      // Collect callers
      Set<String> callers = new TreeSet<>();
      for (Reference r : rm.getReferencesTo(ep)) {
        Function caller = fm.getFunctionContaining(r.getFromAddress());
        if (caller != null) {
          callers.add(caller.getEntryPoint().toString());
        } else {
          callers.add("raw:" + r.getFromAddress().toString());
        }
      }

      // Collect callees, imports, and data refs + immediates
      Set<String> callees = new TreeSet<>();
      Set<String> imports = new TreeSet<>();
      Set<String> dataRefs = new TreeSet<>();
      List<String> pushSeq = new ArrayList<>();

      InstructionIterator iit = listing.getInstructions(f.getBody(), true);
      while (iit.hasNext()) {
        Instruction ins = iit.next();
        for (Reference r : ins.getReferencesFrom()) {
          Address to = r.getToAddress();
          String toStr = to.toString();
          Function target = fm.getFunctionAt(to);
          if (target != null) {
            String tEntry = target.getEntryPoint().toString();
            callees.add(tEntry);
            if (tEntry.startsWith("10a0:") || target.isThunk() || target.isExternal()) {
              imports.add(target.getName());
            }
          } else if (r.getReferenceType().isCall() || r.getReferenceType().isJump()) {
            Function cont = fm.getFunctionContaining(to);
            if (cont != null && !cont.equals(f)) {
              callees.add(cont.getEntryPoint().toString());
            }
          } else if (r.getReferenceType().isData()) {
            dataRefs.add(toStr);
          }
        }
      }

      // Decompile
      String cCode = "";
      String errMsg = "";
      DecompileResults res = decomp.decompileFunction(f, 30, monitor);
      if (res != null && res.getDecompiledFunction() != null) {
        cCode = res.getDecompiledFunction().getC();
      } else if (res != null && res.getErrorMessage() != null) {
        errMsg = res.getErrorMessage();
      }

      // Write to segment C file
      PrintWriter segPw = segWriters.getOrDefault(segPrefix, otherWriter);
      segPw.println("/* ==========================================================================");
      segPw.println(" * Address:   " + epStr);
      segPw.println(" * Name:      " + name);
      segPw.println(" * Size:      " + bodySize + " bytes | Purge: " + purge + " | Conv: " + conv);
      segPw.println(" * Signature: " + sig);
      segPw.println(" * Callers:   " + String.join(", ", callers));
      segPw.println(" * Callees:   " + String.join(", ", callees));
      if (!imports.isEmpty()) {
        segPw.println(" * Imports:   " + String.join(", ", imports));
      }
      if (!dataRefs.isEmpty()) {
        segPw.println(" * DataRefs:  " + String.join(", ", dataRefs));
      }
      segPw.println(" * ========================================================================== */");
      if (cCode != null && !cCode.isEmpty()) {
        segPw.println(cCode.trim());
      } else {
        segPw.println("/* Decompilation unavailable: " + errMsg + " */");
      }
      segPw.println();

      // Write to JSON index
      if (!firstFunc) {
        jsonOut.println(",");
      }
      firstFunc = false;

      jsonOut.print("    {");
      jsonOut.print("\"entry\": \"" + jsonEscape(epStr) + "\", ");
      jsonOut.print("\"segment\": \"" + jsonEscape(segPrefix) + "\", ");
      jsonOut.print("\"name\": \"" + jsonEscape(name) + "\", ");
      jsonOut.print("\"size\": " + bodySize + ", ");
      jsonOut.print("\"purge\": " + purge + ", ");
      jsonOut.print("\"convention\": \"" + jsonEscape(conv) + "\", ");
      jsonOut.print("\"signature\": \"" + jsonEscape(sig) + "\", ");

      jsonOut.print("\"callers\": [");
      int idx = 0;
      for (String c : callers) {
        if (idx++ > 0) jsonOut.print(", ");
        jsonOut.print("\"" + jsonEscape(c) + "\"");
      }
      jsonOut.print("], ");

      jsonOut.print("\"callees\": [");
      idx = 0;
      for (String c : callees) {
        if (idx++ > 0) jsonOut.print(", ");
        jsonOut.print("\"" + jsonEscape(c) + "\"");
      }
      jsonOut.print("], ");

      jsonOut.print("\"imports\": [");
      idx = 0;
      for (String c : imports) {
        if (idx++ > 0) jsonOut.print(", ");
        jsonOut.print("\"" + jsonEscape(c) + "\"");
      }
      jsonOut.print("], ");

      jsonOut.print("\"data_refs\": [");
      idx = 0;
      for (String c : dataRefs) {
        if (idx++ > 0) jsonOut.print(", ");
        jsonOut.print("\"" + jsonEscape(c) + "\"");
      }
      jsonOut.print("], ");

      jsonOut.print("\"c_code\": \"" + jsonEscape(cCode) + "\"");
      jsonOut.print("}");

      count++;
      if (count % 200 == 0) {
        println("Exported " + count + " functions...");
      }
    }

    jsonOut.println();
    jsonOut.println("  ]");
    jsonOut.println("}");
    jsonOut.close();

    for (PrintWriter pw : segWriters.values()) {
      pw.close();
    }
    otherWriter.close();
    decomp.dispose();

    println("Completed exporting " + count + " functions to " + outDir.getAbsolutePath());
  }
}
