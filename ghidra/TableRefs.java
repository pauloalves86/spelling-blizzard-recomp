import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
public class TableRefs extends GhidraScript {
 public void run() throws Exception { String[] a={"1018:3230","1018:3232","1018:3248","1018:324a","1018:3260","1018:3be4","1018:509c"}; for(String s:a){Address x=currentProgram.getAddressFactory().getAddress(s); println("TARGET "+s); for(Reference r:currentProgram.getReferenceManager().getReferencesTo(x)){Function f=currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress()); println(" XREF "+r.getFromAddress()+" "+r.getReferenceType()+" in "+(f==null?"<none>":f.getEntryPoint()+" "+f.getName()));} Data d=currentProgram.getListing().getDataAt(x); println(" DATA "+(d==null?"<none>":d.toString())); }}
}
