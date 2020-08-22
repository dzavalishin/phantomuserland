package ru.dz.plc.compiler.binode;

import java.io.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.*;

/**
 * <p>Title: Two children abstract base node.</p>
 * <p>Description: none.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

abstract public class BiNode extends Node {
  protected Node _r;

  public BiNode(Node l, Node r) {
    super(l);
    this._r = r;
  }

  public Node getRight() { return _r; }
  
  public void print( PrintStream ps, int level, int start_level ) throws PlcException
  {
    //if(type == null) find_out_my_type();
    print_offset( ps, level, start_level );
    //System.out.println(toString());
    print_me(ps);
    print_children(ps, level, start_level);
  }

  protected void print_children(PrintStream ps, int level, int start_level) throws PlcException  {
    if( _l != null )     _l.print(ps, level+1, _r != null ? start_level : start_level+1 );
    if( _r != null )     _r.print(ps, level+1, start_level+1 );
  }

  public boolean is_const()
  {
    return _l != null && _l.is_const() && _r != null && _r.is_const();
  }

  public void find_out_my_type() throws PlcException
  {
    if( type != null && !(type.is_unknown())) return;
    PhantomType l_type = null, r_type = null;

    if( _l != null ) l_type = _l.getType();
    if( _r != null ) r_type = _r.getType();

    if( l_type != null && l_type.equals( r_type ) ) type = l_type;
    else type = new PhTypeUnknown();
  }

  public void preprocess( ParseState s ) throws PlcException
  {
    if(_l != null) _l.preprocess(s);
    if(_r != null) _r.preprocess(s);
    preprocess_me(s);
  }

  public void preprocess_me( ParseState s ) throws PlcException {}

  public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
  {
    if( _l != null ) { _l.generate_code(c,s); move_between_stacks(c, _l.is_on_int_stack()); }
    if( _r != null ) { _r.generate_code(c,s); move_between_stacks(c, _r.is_on_int_stack()); }

    log.fine("Node "+this+" codegen");
    
    if(context != null)			c.emitComment("Line "+context.getLineNumber());
    generate_my_code(c,s);
  }



}


// -------------------------------- real nodes ------------------------------





/**
 * This node can generate code to any stack, depending on children results.
 * Result will be on object stack if any of children is returning value on object
 * stack.
 */
abstract class BiBistackNode extends BiNode {
  //boolean go_to_object_stack = false;
  public boolean args_on_int_stack() { return !go_to_object_stack(); }

  public BiBistackNode( Node l, Node r)
  {
    super(l,r);
    
  }

  public boolean go_to_object_stack() 
  {
	  return (!_l.is_on_int_stack()) || (!_r.is_on_int_stack()); 
  }
  
  public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
  {
    _l.generate_code(c, s);
    if (go_to_object_stack() && _l.is_on_int_stack()) c.emit_i2o();
    _r.generate_code(c, s);
    if (go_to_object_stack() && _r.is_on_int_stack()) c.emit_i2o();

    log.fine("Node "+this+" codegen");
    generate_my_code(c,s);
  }

}






// ------------------------------------------------------------------------

/* TODO       check_assignment_types("container element", type, _r.getType()); */



