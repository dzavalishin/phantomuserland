package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.util.PlcException;

/**
 * Calc expr and do a dup for a later DupDestNode to pick up
 * @author dz
 *
 */

public class DupSourceNode extends Node {
	public DupSourceNode(Node expr) {    super(expr);  }
	public String toString()  {    return "dup src";  }
	public boolean is_on_int_stack() { return false; }
	
	@Override
	public void preprocess_me(ParseState s) throws PlcException {
		// Nothing to do, oh, baby, stay in bed
	}
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		if(getType().is_on_int_stack()) 
			throw new PlcException("Codegen", "op - no int stack dup yet");
		else
			c.emitOsDup();
	}
	
	// for llvm and C we need to keep stack in CodeGeneratorState, assign a temp variable
	// and put var name to stack, pick it up (pop) in dup dest node and use as an expression
	
	
}
