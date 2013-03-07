package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

public abstract class BinaryOpNode extends BiNode {

	public BinaryOpNode(Node l, Node r) {
		super(l, r);
	}

	abstract String getLlvmOpName();

	/**
	 * Default LLVM code generator for binary ops.
	 */
	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		String on = getLlvmOpName();
		String lltype = getLlvmType(); 

		llc.putln(String.format("%s = %s %s %s, %s", llvmTempName, on, lltype, _l.getLlvmTempName(), _r.getLlvmTempName() ));
	}

	// TODO generalized binop codegen for bytecode 

	/*
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException 
	{
		if(getType().is_int()) c.emitISubLU();
		else throw new PlcException("Codegen", "op - does not exist for this type");
	}
	*/
}
