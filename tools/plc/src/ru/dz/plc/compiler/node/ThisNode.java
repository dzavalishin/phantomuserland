package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

 /**
 *  This Node. Loads 'this' (self) on stack.
 */

public class ThisNode extends Node {
	private PhantomClass myClass;

	public ThisNode( PhantomClass c ) {
		super(null);
		myClass = c;
	}
	
	
	public String toString()  {    return "this ";  }
	public PhantomType find_out_my_type() throws PlcException { return new PhantomType(myClass); }
	public boolean is_const() { return false; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	/**
	 * Load this.
	 */
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		c.emitSummonThis();
	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		llc.putln( getLlvmTempName()+" = call "+LlvmCodegen.getObjectType()+" @PhantomVm_getThis();" );
	}
	
	@Override
	protected void generateMy_C_Code(C_codegen cgen) throws PlcException {
		cgen.put(" "+C_codegen.getThisVarName()+" ");
	}
	
}
