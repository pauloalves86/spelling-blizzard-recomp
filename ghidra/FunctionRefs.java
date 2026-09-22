import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.symbol.*;
public class FunctionRefs extends GhidraScript {
 public void run() throws Exception { String[] a={"1018:509c","1018:a9a0","1018:ae3a","1018:aeaa","1008:ac80","1018:5496","1010:08bc","1008:ef2c","1008:2bf4","1010:d0a6"}; for(String s:a){Address ad=currentProgram.getAddressFactory().getAddress(s); Function f=currentProgram.getFunctionManager().getFunctionAt(ad); println("TARGET "+s+" "+(f==null?"<none>":f.getName())); for(Reference r:currentProgram.getReferenceManager().getReferencesTo(ad)){Function c=currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress()); println(" XREF "+r.getFromAddress()+" in "+(c==null?"<none>":c.getEntryPoint()+" "+c.getName()));} if(f!=null){println(" CALLS"); for(Function c:f.getCalledFunctions(monitor)) println("  "+c.getEntryPoint()+" "+c.getName());}}
 }
}
