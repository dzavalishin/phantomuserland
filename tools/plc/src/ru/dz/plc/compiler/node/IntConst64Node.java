package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Integer constant node - 64 bits. Also double constant.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class IntConst64Node extends Node {
	private long val;
	
	public IntConst64Node(long val) {
		super(null);
		this.val = val;
	}
	
	public IntConst64Node(double val) {
		super(null);
		this.val = Double.doubleToLongBits( val );
	}
	
	public String toString()  {    return "int64 const \""+Long.toString(val)+"\"";  }
	public PhantomType find_out_my_type() throws PlcException { return PhantomType.getLong(); }
	public boolean is_on_int_stack() { return true; }
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {
		c.emitIConst_64bit(val);
	}
	
	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		llc.putln(getLlvmTempName()+" = add i64 0, "+Long.toString(val));
	}
	
}

