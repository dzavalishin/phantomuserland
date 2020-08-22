package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.util.PlcException;

/**
 * Jump Node. Generates jump to label.
 */

public class JzNode extends Node {
	private final int labelNo;
	private final String label;

	//public String get_name() { return ident; }

	public JzNode( int labelNo ) {
		super(null);
		this.label = null;
		this.labelNo = labelNo;
	}
	public JzNode( String label ) {
		super(null);
		this.label = label;
		this.labelNo = -1;
	}

	//public String toString()  {    return "jz "+labelNo;  }
	public String toString()  {    return "jz "+ ((labelNo < 0) ? label : ""+labelNo );  }
	
	public void find_out_my_type() throws PlcException { if(type == null) type = new PhTypeUnknown(); }
	public boolean is_const() { return true; }

	public void preprocess_me( ParseState s ) throws PlcException {
	}

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException 
	{
		if(this.label != null)
			c.emitJz(label);
		else
			c.emitJz("javaLabel"+labelNo);
	}

}
