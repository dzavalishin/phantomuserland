package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.trinode.OpMethodCallNode;
import ru.dz.plc.util.PlcException;

 /**
 *  This Node. Loads this on stack.
 */

public class VoidNode extends Node {

	public VoidNode(Node expr) {
		super(expr);
	}
	
	
	public String toString()  {    return "void ";  }
	public PhantomType find_out_my_type() throws PlcException { return PhantomType.getVoid(); }
	public boolean is_const() { return false; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	@Override
	public boolean args_on_int_stack() {
		return _l.is_on_int_stack();
	}

	public void propagateVoidParents()
	{
		_l.setParentIsVoid();
		_l.propagateVoidParents();
	}
	
	/**
	 * Just drop what they produced.
	 */
	protected void generate_my_code(Codegen c, CodeGeneratorState s) 
			throws IOException,	PlcException 
	{
		// Code in _l knows that its parent is void and does not return value at all
		if(_l.getType().is_void() && !(_l instanceof OpMethodCallNode ) )
			return;
		
		// For OpMethodCallNode we must drop anyway for even void methods push return value.
		
		if(_l.is_on_int_stack())
		{
			c.emitNumericPrefix(_l.getType());
			c.emitIsDrop();
		}
		else
			c.emitOsDrop();
	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		// Just empty
	}
	
	@Override
	protected void generateMy_C_Code(C_codegen cgen) throws PlcException {
		// Empty
	}
}
