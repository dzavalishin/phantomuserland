package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.util.PlcException;

/**
 * <p>Break node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class BreakNode extends Node {

	public BreakNode() {    super(null);  }
	public String toString()  {    return "break"; }
	public void find_out_my_type() { if( type == null ) type = new PhTypeVoid(); }
	public boolean is_const() { return false; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		if( s.break_label == null )
			throw new PlcException("break_node","nothing to break out from");
		c.emitJmp(s.break_label);
	}

}