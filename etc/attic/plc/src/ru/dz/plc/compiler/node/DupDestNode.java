package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.util.PlcException;

/**
 * This node DOES NOT call down the chain codegens. It is assumed that it was already done in
 * DupSourceNode and result is DUPped on ostack.
 * @author dz
 *
 */

public class DupDestNode extends Node {

	public DupDestNode(Node expr) {    super(expr);  }
	public String toString()  {    return "dup dst";  }
	public boolean is_on_int_stack() { return false; }
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		if(getType().is_int()) 
			throw new PlcException("Codegen", "op - no int dup yet");
		else
			c.emitComment("dup result is used");
	}

	@Override
	public void preprocess_me(ParseState s) throws PlcException {
		// Nothing to do, oh, baby, stay in bed
	}
	
	@Override
	public void generate_code(Codegen c, CodeGeneratorState s)
			throws IOException, PlcException {		
		
		generate_my_code(c,s);
	}
}
