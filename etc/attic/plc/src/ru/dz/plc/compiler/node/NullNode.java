package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeObject;
import ru.dz.plc.util.PlcException;

/** Just does nothing. */
public class NullNode extends Node {

	public NullNode() {    super(null);  }
	public String toString()  {    return "null"; /*+ident;*/  }
	public void find_out_my_type() throws PlcException { if( type == null ) type = new PhTypeObject(); }
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException 
	{ 
		c.emitSummonNull();
	}

}
