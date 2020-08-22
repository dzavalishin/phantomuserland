package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeInt;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Integer constant node. Also float constant.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class IntConstNode extends Node {
	private int val;

	public IntConstNode(int val) {
		super(null);
		this.val = val;
	}

	public IntConstNode(float val) {
		super(null);
		this.val = Float.floatToIntBits(val);
	}

	public String toString()  {    return "int const \""+Integer.toString(val)+"\"";  }
	public PhantomType find_out_my_type() throws PlcException 
	{ 
		return PhantomType.getInt(); 
	}
	public boolean is_on_int_stack() { return true; }
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {
		c.emitIConst_32bit(val); // BUG! We have shorter constants too, use them in Codegen
	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		llc.putln(getLlvmTempName()+" = add i32 0, "+Integer.toString(val));
	}

	@Override
	protected void generateMy_C_Code(C_codegen cgen) throws PlcException {
		cgen.put(Integer.toString(val));
	}
}

