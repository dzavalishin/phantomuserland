package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * Value comparison. Method 4 is called for non-integers.
 * @author dz
 *
 */
public class ValEqNode extends EqNeqNode {

	public ValEqNode( Node l, Node r) throws PlcException {    
		super(l,r);  
		presetType(PhantomType.getInt());
	}

	public String toString()  {    return "==";  }
	public boolean is_on_int_stack() { return true; }

	// TODO - must redefine generate_code to be able to process
	// objects without conversion to int

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {


		//if(getType().is_int())
		if(!go_to_object_stack())
		{
			//if(!(getType().is_int())) throw new PlcException("val_eq_node","not an int in int stack");

			if(!(getType().is_on_int_stack())) throw new PlcException("val_eq_node","not a numeric on int stack");

			//assert(_l.getType().equals(type) );
			//assert(_r.getType().equals(type) );

			c.emitNumericPrefix(common_type);			
			c.emitISubLU();
			
			if(!common_type.is_int())
				c.emitNumericCast(common_type, PhantomType.getInt());

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

	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) 
			throws PlcException {
		generate_cmp_C_code(cgen, s, "Eq");
	}

	
}
