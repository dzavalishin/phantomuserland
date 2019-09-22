package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/** Just does nothing. */
public class EmptyNode extends Node {

	public EmptyNode() {    super(null);  }
	public String toString()  {    return "empty"; /*+ident;*/  }
	public PhantomType find_out_my_type() { return PhantomType.getVoid(); }
	public boolean is_const() { return false; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	@Override
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException { }

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException { }
	
}
