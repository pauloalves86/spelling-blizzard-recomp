import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.*;
import java.util.*;

public class InitialReport extends GhidraScript {
  public void run() throws Exception {
    Listing l=currentProgram.getListing();
    println("PROGRAM="+currentProgram.getName());
    println("LANGUAGE="+currentProgram.getLanguageID());
    println("COMPILER="+currentProgram.getCompilerSpec().getCompilerSpecID());
    int functionCount=0; for (Function ignored : l.getFunctions(true)) functionCount++; println("FUNCTION_COUNT="+functionCount);
    println("MEMORY_BLOCKS");
    for (MemoryBlock b : currentProgram.getMemory().getBlocks()) println(String.format("%s %s-%s len=%d r=%s w=%s x=%s init=%s", b.getName(),b.getStart(),b.getEnd(),b.getSize(),b.isRead(),b.isWrite(),b.isExecute(),b.isInitialized()));
    println("ENTRY_POINTS");
    for (Address a : currentProgram.getSymbolTable().getExternalEntryPointIterator()) println("EXTERNAL_ENTRY "+a);
    for (Function f : l.getFunctions(true)) {
      if (f.isThunk() || f.getEntryPoint().getOffset()<0x100) {}
      println("FUNC "+f.getEntryPoint()+" "+f.getName()+" body="+f.getBody().getNumAddresses()+" calls="+f.getCalledFunctions(monitor).size()+" callers="+f.getCallingFunctions(monitor).size());
    }
    println("STRINGS");
    DataIterator di=l.getDefinedData(true);
    while(di.hasNext()) { Data d=di.next(); if(d.hasStringValue()) { String s=d.getDefaultValueRepresentation(); if(s.length()>=4) println("STRING "+d.getAddress()+" "+s); } }
    println("EXTERNALS");
    SymbolIterator si=currentProgram.getSymbolTable().getExternalSymbols();
    while(si.hasNext()) { Symbol s=si.next(); println("EXT "+s.getAddress()+" "+s.getName()+" parent="+s.getParentNamespace().getName()); }
  }
}
