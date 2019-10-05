package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeInt;
import ru.dz.plc.util.PlcException;

/**
 * <p>Integer constant node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class IntConstNode extends Node {
	private int val;
	public IntConstNode(int val) {
		super(null);
		this.val = val;
	}
	public String toString()  {    return "int const \""+Integer.toString(val)+"\"";  }
	public void find_out_my_type() throws PlcException { type = new PhTypeInt(); }
	public boolean is_on_int_stack() { return true; }
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {
		c.emitIConst_32bit(val); // BUG! We have shorter constants too, use them in Codegen
	}
}

