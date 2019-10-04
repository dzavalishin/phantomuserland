package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeObject;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/** Just does nothing. */
public class NullNode extends Node {

	public NullNode() {    super(null);  }
	
	public String toString()  {    return "null"; /*+ident;*/  }
	public PhantomType find_out_my_type() throws PlcException { return new PhTypeObject(); }
	
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException 
	{ 
		c.emitSummonNull();
	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		//llc.putln("%"+getLlvmTempName()+" = "+llc.getObjectType()+" 0");
		llc.putln(getLlvmTempName()+" = select i1 true, "+LlvmCodegen.getObjectType()+" <{ i8* null, i8* null }>, "+LlvmCodegen.getObjectType()+" <{ i8* null, i8* null }>");
	}
	
	@Override
	protected void generateMy_C_Code(C_codegen cgen) throws PlcException {
		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"GetNullConstant()");
	}

}
