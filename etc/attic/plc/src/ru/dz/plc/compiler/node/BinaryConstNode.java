package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeString;
import ru.dz.plc.util.PlcException;

/**
 * <p>Binary constant node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class BinaryConstNode extends Node {
	private byte [] val;
	public BinaryConstNode(byte [] val) {
		super(null);
		this.val = val;
	}
	//public String toString()  {    return "binary const \""+val+"\"";  }
	public String toString()  {    return "binary const, \""+val.length+"\" bytes";  }
	
	public void find_out_my_type() { type = new PhTypeString(); }
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {
		c.emitBinary(val);
	}
}
