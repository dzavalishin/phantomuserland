package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Value != node.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */


public class ValNeqNode extends EqNeqNode {
	
	public ValNeqNode( Node l, Node r) 
	{    
		super(l,r);
		presetType(PhantomType.getInt());
	}

	public boolean is_on_int_stack() { return true; }
	public String toString()  {    return "!=";  }
	
	// todo - must redefine generate_code to be able to process
	// objects without conversion to int
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		//if(getType().is_int())
		if(!go_to_object_stack())
		{
			//if(!(getType().is_int())) throw new PlcException("val_eq_node","not an int on int stack");

			if(!(getType().is_on_int_stack())) throw new PlcException("val_eq_node","not a numeric on int stack");

			//assert(_l.getType().equals(type) );
			//assert(_r.getType().equals(type) );

			c.emitNumericPrefix(common_type);			
			c.emitISubLU();
			
			if(!common_type.is_int())
				c.emitNumericCast(common_type, PhantomType.getInt());
		}
		else
		{
			// BUG. What goes on if types are different?
			// especially when A is child of B or vice versa?
			/*
	      if( ! (_l.getType().equals(_r.getType())) )
	        throw new PlcException("Codegen", "can't != values of different types", _l.getType().toString() + " and " + _r.getType().toString() );
			 */
			c.emitCall(4,1); // Method number 4 is ==
			// Here we have to move value to int stack
			// because of logical not operation
			c.emit_o2i();
			c.emitLogNot();
		}
		//else throw new PlcException("Codegen", "op != does not exist for this type");
	}

	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) 
			throws PlcException {
		generate_cmp_C_code(cgen, s, "Neq");
	}

}

