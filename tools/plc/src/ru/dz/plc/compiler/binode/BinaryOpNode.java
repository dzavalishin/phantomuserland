package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.CastNode;
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
		if(!bop_preprocessed) throw new PlcException("bin op","not preprocessed");

		String on = getLlvmOpName();
		String lltype = getLlvmType(); 

		llc.putln(String.format("%s = %s %s %s, %s", llvmTempName, on, lltype, _l.getLlvmTempName(), _r.getLlvmTempName() ));
	}


	// Supposed to be working for all the children of this class
	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException 
	{
		if(!bop_preprocessed) throw new PlcException("bin op","not preprocessed");

		// JIT_op_plus(left,right)
		String oname = getLlvmOpName();
		cgen.put("JIT_op_"+oname+"(");
		_l.generate_C_code(cgen,s);
		cgen.put(",");
		_r.generate_C_code(cgen,s);
		cgen.put(")");		
	}

	private boolean bop_preprocessed = false;

	@Override
	public PhantomType find_out_my_type() throws PlcException 
	{
		PhantomType t =  PhantomType.findCompatibleType(_l.getType(),_r.getType());
		if( t == null )
		{
			print_error(String.format("types %s and %s are incompatible", 
					_l.getType().toString(),
					_r.getType().toString()
					));
			throw new PlcException("bin op find type "+context.get_position(),"types are not compatible");
		}
		return t;
	}

	@Override
	public void preprocess_me(ParseState s) throws PlcException {
		super.preprocess_me(s);

		find_out_my_type();

		if(!getType().is_on_int_stack())
			throw new PlcException("bin op preprocess", "not numeric type");

		if( !_l.getType().equals(getType()) ) 
			_l = new CastNode(_l, getType());

		if( !_r.getType().equals(getType()) ) 
			_r = new CastNode(_r, getType());

		bop_preprocessed = true;
	}


	protected void generateIntegerStackOp( Codegen c, RunBinaryOp op  ) throws PlcException
	{
		if(!bop_preprocessed) throw new PlcException("bin op","not preprocessed");

		try {
			c.emitNumericPrefix(getType());
			op.run(); // generate actual op
		} catch (IOException e) {
			throw new PlcException("io error", e);
		}

	}

}
