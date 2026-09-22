import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.lang.*;
import java.util.*;

public class CallbackScan extends GhidraScript {
  private void dumpFunctionRefs(String text) {
    Address a=currentProgram.getAddressFactory().getAddress(text);
    Function f=currentProgram.getFunctionManager().getFunctionAt(a);
    println("FUNCTION "+text+" "+(f==null?"<none>":f.getName()));
    if(f!=null) {
      for(Reference r:currentProgram.getReferenceManager().getReferencesTo(a)) {
        Function c=currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress());
        println("  XREF "+r.getFromAddress()+" type="+r.getReferenceType()+" in "+(c==null?"<none>":c.getEntryPoint()+" "+c.getName()));
      }
    }
  }
  private void scanWord(int value) throws Exception {
    println("WORD_SCAN 0x"+Integer.toHexString(value));
    Memory mem=currentProgram.getMemory();
    for(MemoryBlock b:mem.getBlocks()) {
      if(!b.isInitialized()) continue;
      Address p=b.getStart(); Address end=b.getEnd();
      while(p.compareTo(end.subtract(1))<=0) {
        int lo=mem.getByte(p)&0xff; int hi=mem.getByte(p.add(1))&0xff;
        if(((hi<<8)|lo)==value) {
          Data d=currentProgram.getListing().getDataAt(p);
          println(" HIT "+p+" block="+b.getName()+" data="+(d==null?"<none>":d.getDataType().getName()));
          for(Reference r:currentProgram.getReferenceManager().getReferencesFrom(p)) println("  FROM_REF "+r.getToAddress()+" "+r.getReferenceType());
        }
        p=p.add(1);
      }
    }
  }
  public void run() throws Exception {
    Address target=currentProgram.getAddressFactory().getAddress("1018:509c");
    println("TARGET "+target+" offset=0x"+Long.toHexString(target.getOffset())+" space="+target.getAddressSpace().getName());
    dumpFunctionRefs("1018:509c");
    dumpFunctionRefs("1010:596e");
    dumpFunctionRefs("1010:5b18");
    scanWord(0x509c);
    scanWord(0x1018);
    println("SEGMENTS");
    for(MemoryBlock b:currentProgram.getMemory().getBlocks()) println(b.getName()+" "+b.getStart()+"-"+b.getEnd()+" init="+b.isInitialized()+" write="+b.isWrite());
  }
}
