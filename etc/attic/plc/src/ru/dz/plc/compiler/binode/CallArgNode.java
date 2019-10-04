package ru.dz.plc.compiler.binode;

import java.io.IOException;
import java.io.PrintStream;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

 
/**
 * <p>Function call argument Node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class CallArgNode extends BiNode {
  public CallArgNode( Node l, Node r) {    super(l,r);  }
  public String toString()  {    return "arg ";  }
  public boolean args_on_int_stack() { return false; } // I need them on object stack
  protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {} // empty
  // Args are pushed from 1 to last, no reverse order
  protected void print_children(PrintStream ps, int level, int start_level) throws PlcException  {
    if( _l != null )     _l.print(ps, level+1, start_level+1 );
    if( _r != null )     _r.print(ps, level, start_level );
  }
}