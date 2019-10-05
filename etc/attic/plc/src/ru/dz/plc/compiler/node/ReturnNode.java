package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.util.PlcException;

/**
 * <p>Return node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class ReturnNode extends Node {
	
	public ReturnNode( Node expr ) {
		super(expr);
	}
	public boolean args_on_int_stack() { return false; }
	public String toString()  {    return "return";  }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		c.emitRet();
	}
	public void find_out_my_type()  {    type = new PhTypeVoid();  }

}
