import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.app.decompiler.*;
public class GameDecomp extends GhidraScript {
 public void run() throws Exception { DecompInterface d=new DecompInterface(); d.openProgram(currentProgram); String[] a={"1018:509c","1018:b90c","1018:b218","1020:067a","1020:10d8","1018:708a","1018:7f32"}; for(String s:a){Function f=currentProgram.getListing().getFunctionAt(currentProgram.getAddressFactory().getAddress(s)); println("--- "+s+" "+(f==null?"<none>":f.getName())+" ---"); if(f!=null){DecompileResults r=d.decompileFunction(f,60,monitor); println(r.getDecompiledFunction()==null?"<none>":r.getDecompiledFunction().getC());}} }
}
