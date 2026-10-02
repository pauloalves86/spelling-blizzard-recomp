// Classify code coverage and analyze uncovered gaps in BLIZZARD.EXE.
// Usage: analyzeHeadless ghidra SpellingBlizzard -process BLIZZARD.EXE -readOnly -noanalysis -scriptPath ghidra -postScript ClassifyCoverage.java

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ClassifyCoverage extends GhidraScript {
  static final byte CAT_UNCLASSIFIED = 0;
  static final byte CAT_COVERED_FUNC = 1;
  static final byte CAT_VTABLE = 2;
  static final byte CAT_PADDING = 3;
  static final byte CAT_JUMP_TABLE = 4;
  static final byte CAT_STRINGS = 5;
  static final byte CAT_CANDIDATE_CODE = 6;
  static final byte CAT_OTHER_DATA = 7;

  public void run() throws Exception {
    String[] segNames = new String[] { "1000", "1008", "1010", "1018", "1020" };
    int[] segLens = new int[] { 9191, 61402, 61418, 61307, 28650 };

    // Load vtable ranges from build/vtable_ranges.txt
    List<String> vtLines = Files.readAllLines(Paths.get("build/vtable_ranges.txt"));
    Map<String, List<int[]>> vtablesBySeg = new HashMap<String, List<int[]>>();
    for (String seg : segNames) {
      vtablesBySeg.put(seg, new ArrayList<int[]>());
    }
    for (String l : vtLines) {
      String[] parts = l.trim().split("\\s+");
      if (parts.length >= 3) {
        String seg = parts[0];
        int start = Integer.parseInt(parts[1]);
        int end = Integer.parseInt(parts[2]);
        if (vtablesBySeg.containsKey(seg)) {
          vtablesBySeg.get(seg).add(new int[] { start, end });
        }
      }
    }

    AddressFactory af = currentProgram.getAddressFactory();
    FunctionManager fm = currentProgram.getFunctionManager();
    Listing listing = currentProgram.getListing();
    Memory mem = currentProgram.getMemory();

    int totalProgramBytes = 0;
    int totalCoveredFunc = 0;
    int totalVtable = 0;
    int totalPadding = 0;
    int totalJumpTable = 0;
    int totalStrings = 0;
    int totalCandidateCode = 0;
    int totalOtherData = 0;

    PrintWriter jsonOut = new PrintWriter(new FileWriter("build/coverage_classification.json"));
    jsonOut.println("{");
    jsonOut.println("  \"segments\": {");

    for (int s = 0; s < segNames.length; s++) {
      String seg = segNames[s];
      int segLen = segLens[s];
      totalProgramBytes += segLen;

      byte[] cat = new byte[segLen];
      byte[] memBytes = new byte[segLen];

      Address startAd = af.getAddress(seg + ":0000");
      Address endAd = af.getAddress(String.format("%s:%04x", seg, segLen - 1));
      AddressSet segSet = new AddressSet(startAd, endAd);

      // Read raw memory bytes for this segment
      try {
        mem.getBytes(startAd, memBytes);
      } catch (Exception e) {
        println("ERROR reading memory for seg " + seg + ": " + e.getMessage());
      }

      // Step 1: Mark Covered Functions
      FunctionIterator fit = fm.getFunctions(segSet, true);
      while (fit.hasNext()) {
        Function f = fit.next();
        AddressSetView body = f.getBody().intersect(segSet);
        for (AddressRange r : body.getAddressRanges()) {
          int rStart = (int) (r.getMinAddress().getOffset() & 0xffff);
          int rEnd = (int) (r.getMaxAddress().getOffset() & 0xffff);
          for (int off = rStart; off <= rEnd && off < segLen; off++) {
            cat[off] = CAT_COVERED_FUNC;
          }
        }
      }

      // Step 2: Mark Vtables
      for (int[] vr : vtablesBySeg.get(seg)) {
        for (int off = vr[0]; off <= vr[1] && off < segLen; off++) {
          if (cat[off] == CAT_UNCLASSIFIED) {
            cat[off] = CAT_VTABLE;
          }
        }
      }

      // Step 3: Mark Jump Tables (runs of >= 3 16-bit words pointing within segment)
      int off = 0;
      while (off < segLen - 5) {
        if (cat[off] != CAT_UNCLASSIFIED) {
          off++;
          continue;
        }
        int k = 0;
        while (off + 2 * k + 1 < segLen && cat[off + 2 * k] == CAT_UNCLASSIFIED && cat[off + 2 * k + 1] == CAT_UNCLASSIFIED) {
          int bLo = memBytes[off + 2 * k] & 0xff;
          int bHi = memBytes[off + 2 * k + 1] & 0xff;
          int target = bLo | (bHi << 8);
          // Check if target is a plausible code offset in this segment within +/- 0x1800
          if (target >= 0 && target < segLen && Math.abs(target - off) < 0x1800) {
            k++;
          } else {
            break;
          }
        }
        if (k >= 3) {
          for (int b = 0; b < 2 * k; b++) {
            cat[off + b] = CAT_JUMP_TABLE;
          }
          off += 2 * k;
        } else {
          off++;
        }
      }

      // Step 4: Mark Strings (printable ASCII >= 4 chars)
      off = 0;
      while (off < segLen) {
        if (cat[off] != CAT_UNCLASSIFIED) {
          off++;
          continue;
        }
        int k = 0;
        while (off + k < segLen && cat[off + k] == CAT_UNCLASSIFIED) {
          int b = memBytes[off + k] & 0xff;
          if ((b >= 32 && b <= 126) || b == 10 || b == 13 || b == 9) {
            k++;
          } else {
            break;
          }
        }
        if (k >= 4) {
          // If followed by 0x00, include the null terminator
          int strLen = k;
          if (off + k < segLen && memBytes[off + k] == 0 && cat[off + k] == CAT_UNCLASSIFIED) {
            strLen++;
          }
          for (int b = 0; b < strLen; b++) {
            cat[off + b] = CAT_STRINGS;
          }
          off += strLen;
        } else {
          off++;
        }
      }

      // Step 5: Mark Padding (runs of 0x00, 0x90, 0xcc)
      off = 0;
      while (off < segLen) {
        if (cat[off] != CAT_UNCLASSIFIED) {
          off++;
          continue;
        }
        int k = 0;
        while (off + k < segLen && cat[off + k] == CAT_UNCLASSIFIED) {
          int b = memBytes[off + k] & 0xff;
          if (b == 0x00 || b == 0x90 || b == 0xcc) {
            k++;
          } else {
            break;
          }
        }
        if (k > 0) {
          // Check if this run is isolated padding or between code/tables
          for (int b = 0; b < k; b++) {
            cat[off + b] = CAT_PADDING;
          }
          off += k;
        } else {
          off++;
        }
      }

      // Step 6: Mark Candidate Code (disassemble in gaps)
      off = 0;
      while (off < segLen) {
        if (cat[off] != CAT_UNCLASSIFIED) {
          off++;
          continue;
        }
        Address curAd = af.getAddress(String.format("%s:%04x", seg, off));
        Instruction ins = listing.getInstructionAt(curAd);
        if (ins == null) {
          boolean disOk = disassemble(curAd);
          if (disOk) {
            ins = listing.getInstructionAt(curAd);
          }
        }

        if (ins != null) {
          // Trace instructions in gap until return or non-code
          Address insAd = curAd;
          while (insAd != null && insAd.compareTo(endAd) <= 0) {
            int insOff = (int) (insAd.getOffset() & 0xffff);
            if (insOff >= segLen || cat[insOff] != CAT_UNCLASSIFIED) {
              break;
            }
            Instruction curIns = listing.getInstructionAt(insAd);
            if (curIns == null) break;

            int len = curIns.getLength();
            for (int b = 0; b < len && insOff + b < segLen; b++) {
              if (cat[insOff + b] == CAT_UNCLASSIFIED) {
                cat[insOff + b] = CAT_CANDIDATE_CODE;
              }
            }

            String mnem = curIns.getMnemonicString();
            if (mnem.startsWith("RET") || mnem.equals("JMP") || mnem.equals("JMPF")) {
              break; // End of basic block / function
            }
            insAd = insAd.add(len);
          }
        }
        off++;
      }

      // Step 7: Mark any remaining as Other Data
      for (int i = 0; i < segLen; i++) {
        if (cat[i] == CAT_UNCLASSIFIED) {
          cat[i] = CAT_OTHER_DATA;
        }
      }

      // Tally categories for this segment
      int cntCovered = 0, cntVt = 0, cntPad = 0, cntJt = 0, cntStr = 0, cntCand = 0, cntOther = 0;
      for (int i = 0; i < segLen; i++) {
        switch (cat[i]) {
          case CAT_COVERED_FUNC:   cntCovered++; break;
          case CAT_VTABLE:         cntVt++;      break;
          case CAT_PADDING:        cntPad++;     break;
          case CAT_JUMP_TABLE:     cntJt++;      break;
          case CAT_STRINGS:        cntStr++;     break;
          case CAT_CANDIDATE_CODE: cntCand++;    break;
          case CAT_OTHER_DATA:     cntOther++;   break;
        }
      }

      totalCoveredFunc += cntCovered;
      totalVtable += cntVt;
      totalPadding += cntPad;
      totalJumpTable += cntJt;
      totalStrings += cntStr;
      totalCandidateCode += cntCand;
      totalOtherData += cntOther;

      double trueCoverage = (cntCovered + cntCand) > 0 ? ((double) cntCovered / (cntCovered + cntCand)) * 100.0 : 100.0;
      double rawRatio = ((double) cntCovered / segLen) * 100.0;

      println(String.format("Seg %s (len %d):", seg, segLen));
      println(String.format("  Covered Functions: %5d (%5.2f%%)", cntCovered, rawRatio));
      println(String.format("  Candidate Code:    %5d (%5.2f%%)", cntCand, ((double)cntCand/segLen)*100.0));
      println(String.format("  Vtables:           %5d (%5.2f%%)", cntVt, ((double)cntVt/segLen)*100.0));
      println(String.format("  Jump Tables:       %5d (%5.2f%%)", cntJt, ((double)cntJt/segLen)*100.0));
      println(String.format("  Strings:           %5d (%5.2f%%)", cntStr, ((double)cntStr/segLen)*100.0));
      println(String.format("  Padding:           %5d (%5.2f%%)", cntPad, ((double)cntPad/segLen)*100.0));
      println(String.format("  Other Data:        %5d (%5.2f%%)", cntOther, ((double)cntOther/segLen)*100.0));
      println(String.format("  => True Code Coverage: %5.2f%% [covered / (covered + candidate)]", trueCoverage));

      jsonOut.println(String.format("    \"%s\": {", seg));
      jsonOut.println(String.format("      \"length\": %d,", segLen));
      jsonOut.println(String.format("      \"covered_functions\": %d,", cntCovered));
      jsonOut.println(String.format("      \"candidate_code\": %d,", cntCand));
      jsonOut.println(String.format("      \"vtables\": %d,", cntVt));
      jsonOut.println(String.format("      \"jump_tables\": %d,", cntJt));
      jsonOut.println(String.format("      \"strings\": %d,", cntStr));
      jsonOut.println(String.format("      \"padding\": %d,", cntPad));
      jsonOut.println(String.format("      \"other_data\": %d,", cntOther));
      jsonOut.println(String.format("      \"true_coverage_pct\": %.2f,", trueCoverage));
      jsonOut.println(String.format("      \"raw_ratio_pct\": %.2f", rawRatio));
      if (s < segNames.length - 1) {
        jsonOut.println("    },");
      } else {
        jsonOut.println("    }");
      }
    }

    double overallTrueCov = ((double) totalCoveredFunc / (totalCoveredFunc + totalCandidateCode)) * 100.0;
    double overallRawRatio = ((double) totalCoveredFunc / totalProgramBytes) * 100.0;

    println("================================================================");
    println(String.format("OVERALL TOTALS across all %d code segments (%d bytes):", segNames.length, totalProgramBytes));
    println(String.format("  Covered Functions: %6d (%5.2f%%)", totalCoveredFunc, overallRawRatio));
    println(String.format("  Candidate Code:    %6d (%5.2f%%)", totalCandidateCode, ((double)totalCandidateCode/totalProgramBytes)*100.0));
    println(String.format("  Vtables:           %6d (%5.2f%%)", totalVtable, ((double)totalVtable/totalProgramBytes)*100.0));
    println(String.format("  Jump Tables:       %6d (%5.2f%%)", totalJumpTable, ((double)totalJumpTable/totalProgramBytes)*100.0));
    println(String.format("  Strings:           %6d (%5.2f%%)", totalStrings, ((double)totalStrings/totalProgramBytes)*100.0));
    println(String.format("  Padding:           %6d (%5.2f%%)", totalPadding, ((double)totalPadding/totalProgramBytes)*100.0));
    println(String.format("  Other Data:        %6d (%5.2f%%)", totalOtherData, ((double)totalOtherData/totalProgramBytes)*100.0));
    println(String.format("  => OVERALL TRUE CODE COVERAGE: %5.2f%%", overallTrueCov));
    println("================================================================");

    jsonOut.println("  },");
    jsonOut.println("  \"totals\": {");
    jsonOut.println(String.format("    \"length\": %d,", totalProgramBytes));
    jsonOut.println(String.format("    \"covered_functions\": %d,", totalCoveredFunc));
    jsonOut.println(String.format("    \"candidate_code\": %d,", totalCandidateCode));
    jsonOut.println(String.format("    \"vtables\": %d,", totalVtable));
    jsonOut.println(String.format("    \"jump_tables\": %d,", totalJumpTable));
    jsonOut.println(String.format("    \"strings\": %d,", totalStrings));
    jsonOut.println(String.format("    \"padding\": %d,", totalPadding));
    jsonOut.println(String.format("    \"other_data\": %d,", totalOtherData));
    jsonOut.println(String.format("    \"true_coverage_pct\": %.2f,", overallTrueCov));
    jsonOut.println(String.format("    \"raw_ratio_pct\": %.2f", overallRawRatio));
    jsonOut.println("  }");
    jsonOut.println("}");
    jsonOut.close();
  }
}
