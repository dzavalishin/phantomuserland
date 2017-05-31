package ru.dz.plc.compiler.binode;

import java.io.PrintStream;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/** 
 * <p>Just order of execution.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class SequenceNode extends BiNode {
	  public SequenceNode( Node l, Node r) {    super(l,r);  }
	  public String toString()  {    return ", ";  }
	  protected void generate_my_code(Codegen c, CodeGeneratorState s) {} // really nothing :)
	  protected void print_children(PrintStream ps, int level, int start_level) throws PlcException {
	    if( _l != null )     _l.print(ps, level+1, start_level+1 );
	    if( _r != null )     _r.print(ps, level+1, start_level+1 );
	  }
	  protected void print_me() {
	    /*System.out.print(toString());
	    if( is_const()   ) System.out.print(" : const");
	    System.out.println();*/
	  }
	  public void preprocess_me(ParseState ps) throws PlcException {}
	}
