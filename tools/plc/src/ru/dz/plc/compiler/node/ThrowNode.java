package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * Throw Node. Throws exception.
 */

public class ThrowNode extends Node {

	public ThrowNode( Node expr ) {
		super(expr);
	}
	public String toString()  {    return "throw "; /*+ident;*/  }
	public PhantomType find_out_my_type() { return PhantomType.getVoid(); }
	public boolean is_const() { return true; }

	// load this
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		c.emitThrow();
	}

	public void preprocess_me( ParseState s ) throws PlcException
	{
	}
	
	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException {
		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"Throw(");
		_l.generate_C_code(cgen, s);
		cgen.putln(")");
	}
}
