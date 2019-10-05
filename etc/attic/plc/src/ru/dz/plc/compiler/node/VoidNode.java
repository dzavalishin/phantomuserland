package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

 /**
 *  This Node. Loads this on stack.
 */

public class VoidNode extends Node {

	public VoidNode(Node expr) {
		super(expr);
	}
	
	
	public String toString()  {    return "void ";  }
	public void find_out_my_type() throws PlcException { if( type == null ) type = new PhTypeVoid(); }
	public boolean is_const() { return false; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	@Override
	public boolean args_on_int_stack() {
		return _l.is_on_int_stack();
	}
	
	/**
	 * Just drop what they produced.
	 */
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		if(_l.is_on_int_stack())
			c.emitIsDrop();
		else
			c.emitOsDrop();
	}

}
