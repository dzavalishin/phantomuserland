package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.util.PlcException;

/**
 * Bitwise not.
 */
public class OpNotNode extends Node 
{
	public OpNotNode(Node l) {    super(l);  }
	public String toString()  {    return "~";  }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	public boolean is_on_int_stack() { return true; }
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException 
	{
		c.emitNumericPrefix(getType());
		c.emit_inot();
	}
	
	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException
	{
		cgen.put("!");
		_l.generate_C_code(cgen, s);
	}
}


