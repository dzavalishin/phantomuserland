package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeString;
import ru.dz.plc.util.PlcException;

public class StringConstNode extends Node {
	private String val;
	public StringConstNode(String val) {
		super(null);
		this.val = val;
	}
	public String toString()  {    return "string const \""+val+"\"";  }
	public void find_out_my_type() { type = new PhTypeString(); }
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {
		c.emitString(val);
	}
}
