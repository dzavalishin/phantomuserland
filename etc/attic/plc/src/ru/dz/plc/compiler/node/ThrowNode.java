package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.util.PlcException;

/**
 * Throw Node. Throws exception.
 */

public class ThrowNode extends Node {

	public ThrowNode( Node expr ) {
		super(expr);
	}
	public String toString()  {    return "throw "; /*+ident;*/  }
	public void find_out_my_type() { if( type == null ) type = new PhTypeVoid(); }
	public boolean is_const() { return true; }

	// load this
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		c.emitThrow();
	}

	public void preprocess_me( ParseState s ) throws PlcException
	{
	}
}
