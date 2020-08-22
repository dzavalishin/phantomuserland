package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.util.PlcException;

/**
 * <p>Continue operator node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ContinueNode extends Node {

	public ContinueNode() {    super(null);  }
	public String toString()  {    return "continue"; /*+ident;*/  }
	public void find_out_my_type() { if( type == null ) type = new PhTypeVoid(); }
	public boolean is_const() { return false; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		if( s.continue_label == null )
			throw new PlcException("continue_node","nothing to continue here");
		c.emitJmp(s.continue_label);
	}

}