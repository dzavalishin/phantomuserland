package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

 /**
 * Value comparison. Method 4 is called for non-integers.
 * @author dz
 *
 */
public class ValEqNode extends BiBistackNode {
  public ValEqNode( Node l, Node r) {    super(l,r);  }
  public String toString()  {    return "==";  }
  public boolean is_on_int_stack() { return true; }
  // todo - must redefine generate_code to be able to process
  // objects without conversion to int

  protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {


    //if(getType().is_int())
    if(!go_to_object_stack())
    {
      if(!(getType().is_int())) throw new PlcException("val_eq_node","not an int in int stack");
      c.emitISubLU();
      c.emitLogNot();
    }
    else
    {
      // BUG. What goes on if types are different?
      // especially when A is child of B or vice versa?
      /*
      if( ! (_l.getType().equals(_r.getType())) )
        throw new PlcException("Codegen", "can't == values of different types", _l.getType().toString() + " and " + _r.getType().toString() );
      */
      c.emitCall(4,1); // Method number 4 is ==
      // BUG. It is better to leave this stuff on o stack
      // and let caller decide whether to move it, but then we'll need to
      // rewrite is_on_int_stack() in a more clever way that it is
      c.emit_o2i();
    }
  }
}
