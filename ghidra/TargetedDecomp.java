import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.app.decompiler.*;
public class TargetedDecomp extends GhidraScript {
 public void run() throws Exception {
  DecompInterface d=new DecompInterface(); d.openProgram(currentProgram);
  String[] addrs={"1008:e412","1010:07fa","1010:08bc","1010:3d56","1010:42ce","1010:446a","1010:596e","1010:5b18","1008:82b8","1010:0fe6","1010:286c","1010:3190","1010:3386","1010:34ea","1010:37e6","1008:6a28","1018:267e","1018:3c70","1018:509c","1018:649a","1018:6c72","1018:71da","1018:9684","1018:a9a0","1018:ae3a","1018:aeaa","1010:d0a6","1010:50ce","1010:53c8","1010:5722","1008:0eba","1008:670a","1008:6ef6","1008:ac80","1008:2bf4","1008:b7de","1010:d03c","1018:5496","1010:01d0","1008:8e46","1018:a704","1018:ae7e","1018:aec0","1018:284c","1018:438e","1018:3ee0","1020:1a9a","1020:678c","1018:2646","1018:5496","1010:2a74","1018:aa84","1018:ab1c","1018:ac66","1018:ad88","1018:adb8","1018:aede","1018:aec0","1010:035c","1010:01d0","1008:e812"};
  for(String as:addrs){Function f=currentProgram.getListing().getFunctionAt(currentProgram.getAddressFactory().getAddress(as)); println("--- "+as+" "+(f==null?"<none>":f.getName())+" ---"); if(f!=null){DecompileResults r=d.decompileFunction(f,60,monitor); println(r.getDecompiledFunction()==null?"<none>":r.getDecompiledFunction().getC());}}
 }
}
