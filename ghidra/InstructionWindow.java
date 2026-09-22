import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
public class InstructionWindow extends GhidraScript {
 public void run() throws Exception { String[] ss={"1018:ae3a","1018:aeaa","1018:3c70","1008:ac80","1010:2a7e","1008:e8b6","1008:cf6c","1008:cfc2","1008:d0f2","1008:e838","1008:e94a"}; for(String s:ss){Address a=currentProgram.getAddressFactory().getAddress(s); println("AT "+s); Instruction i=currentProgram.getListing().getInstructionContaining(a); if(i==null)i=currentProgram.getListing().getInstructionBefore(a); for(int n=0;n<5&&i!=null;n++,i=currentProgram.getListing().getInstructionBefore(i.getAddress())){} i=currentProgram.getListing().getInstructionBefore(a); for(int n=0;n<10&&i!=null;n++,i=currentProgram.getListing().getInstructionAfter(i.getAddress())) println(" "+i.getAddress()+" "+i.toString());}}
}
