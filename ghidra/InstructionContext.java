import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
public class InstructionContext extends GhidraScript {
 public void run() throws Exception {
  String[] ss={"1018:3248","1018:8687","1018:86a1","1018:86bc","1018:86d7","1018:ae8d","1018:aeef","1010:59ef","1010:5b66"};
  for(String s:ss){Address a=currentProgram.getAddressFactory().getAddress(s); println("AT "+s); Function f=currentProgram.getFunctionManager().getFunctionContaining(a); println("FUNC "+(f==null?"<none>":f.getEntryPoint()+" "+f.getName())); Instruction i=currentProgram.getListing().getInstructionAt(a); if(i==null) i=currentProgram.getListing().getInstructionAfter(a); for(int n=0;n<8 && i!=null;n++,i=currentProgram.getListing().getInstructionAfter(i.getAddress())) println("  "+i.getAddress()+" "+i.toString()); }
 }
}
