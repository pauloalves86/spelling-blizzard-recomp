import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.app.decompiler.*;
public class FocusReport extends GhidraScript {
 public void run() throws Exception {
  Listing l=currentProgram.getListing();
  println("SPECIAL_INSTRUCTIONS");
  for (Instruction i:l.getInstructions(true)) { String m=i.getMnemonicString(); if(m.equals("INT")||m.equals("IN")||m.equals("OUT")) println(i.getAddress()+" "+i); }
  println("EXTERNAL_CALLERS");
  SymbolIterator ex=currentProgram.getSymbolTable().getExternalSymbols();
  while(ex.hasNext()) { Symbol s=ex.next(); Reference[] refs=s.getReferences(); int n=0; StringBuilder z=new StringBuilder(); for(Reference r:refs){ if(n>=20) break; z.append(r.getFromAddress()).append(" "); n++; } if(n>0) println(s.getParentNamespace().getName()+"!"+s.getName()+" <- "+z); }
  DecompInterface d=new DecompInterface(); d.openProgram(currentProgram);
  String[] addrs={"1000:001a","1000:00d8","1000:0104","1000:0346","1000:04f0","1000:05b4","1000:071a","1000:185c","1000:1a0a","1000:1b02","1008:82b8","1008:9222","1018:b90c","1020:10d8"};
  println("DECOMPILED_FOCUS");
  for(String as:addrs){Function f=l.getFunctionAt(currentProgram.getAddressFactory().getAddress(as)); if(f==null){println("NO "+as);continue;} DecompileResults r=d.decompileFunction(f,30,monitor); println("--- "+as+" "+f.getName()+" ---"); println(r.getDecompiledFunction()==null?"<none>":r.getDecompiledFunction().getC());}
 }
}
