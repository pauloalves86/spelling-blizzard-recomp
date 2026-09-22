import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.app.decompiler.*;
import java.util.*;

public class ApplicationReport extends GhidraScript {
  private void refsTo(String module, String name) {
    SymbolIterator it=currentProgram.getSymbolTable().getSymbols(name);
    while(it.hasNext()) { Symbol s=it.next(); if(!s.getParentNamespace().getName().equals(module)) continue; println("API "+module+"!"+name+" symbol="+s.getAddress());
      for(Reference r: currentProgram.getReferenceManager().getReferencesTo(s.getAddress())) { Function f=currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress()); println("  XREF "+r.getFromAddress()+" in "+(f==null?"<none>":f.getEntryPoint()+" "+f.getName())); }
    }
    Function thunk=currentProgram.getFunctionManager().getFunctions(true).hasNext()?null:null;
    FunctionIterator fi=currentProgram.getFunctionManager().getFunctions(true);
    while(fi.hasNext()) { Function f=fi.next(); if(f.getName().equals(name)) { println("LOCAL_API "+module+"!"+name+" entry="+f.getEntryPoint()); for(Reference r:currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint())) { Function caller=currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress()); println("  LOCAL_XREF "+r.getFromAddress()+" in "+(caller==null?"<none>":caller.getEntryPoint()+" "+caller.getName())); } } }
  }
  private void stringRefs(String needle) {
    println("STRING_SEARCH "+needle);
    Listing l=currentProgram.getListing();
    DataIterator di=l.getDefinedData(true);
    while(di.hasNext()) { Data d=di.next(); String s=d.getDefaultValueRepresentation(); if(s!=null && s.toLowerCase().contains(needle.toLowerCase())) { println("STRING "+d.getAddress()+" "+s); for(Reference r:currentProgram.getReferenceManager().getReferencesTo(d.getAddress())) {Function f=currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress()); println("  XREF "+r.getFromAddress()+" in "+(f==null?"<none>":f.getEntryPoint()+" "+f.getName()));}} }
  }
  public void run() throws Exception {
    String[] apis={"PEEKMESSAGE","TRANSLATEMESSAGE","DISPATCHMESSAGE","POSTMESSAGE","INVALIDATERECT","VALIDATERECT","GETASYNCKEYSTATE","GETKEYSTATE","REGISTERCLASS","CREATEWINDOW","SHOWWINDOW","GETDC","BITBLT","WAVEOUTWRITE","WAVEOUTOPEN","WAVEOUTPREPAREHEADER","TIMEGETTIME","FINDRESOURCE","LOADRESOURCE","LOADLIBRARY","GETMODULEFILENAME","GETPRIVATEPROFILESTRING","OPENFILE"};
    println("API_XREFS"); for(String a:apis) { String mod=a.startsWith("WAVE")||a.equals("TIMEGETTIME")?"MMSYSTEM":(a.equals("BITBLT")?"GDI":(a.equals("FINDRESOURCE")||a.equals("LOADRESOURCE")||a.equals("LOADLIBRARY")||a.equals("GETMODULEFILENAME")||a.equals("GETPRIVATEPROFILESTRING")||a.equals("OPENFILE")?"KERNEL":"USER")); refsTo(mod,a); }
    for(String s:new String[]{"SpellingDemonsWindow","SpellingDemonsDesktop","CEditText","CAMEditText","WinClass","CWindow","CDesktop","theMatch","HighScor","CHEAT","thePlacedLadder","itsSpellingDemonObjectGrid","itsLayeredOffscreen","itsLayeredObjectList","itsImageHandlerList","itsGridCellArray","itsForeRedrawList","itsBackRedrawList","itsOrdersList","itsRemovalList","itsAdditionList","itsDeletionList"}) stringRefs(s);
  }
}
