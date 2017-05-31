package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.util.PlcException;

 /**
 * Jump target Node. Generates label.
 */

public class JumpTargetNode extends Node {
	private final int labelNo;
	private final String labelString;

	//public String get_name() { return ident; }

	public JumpTargetNode( int labelNo ) {
		super(null);
		this.labelString = null;
		this.labelNo = labelNo;
	}
	public JumpTargetNode(String labelString) {
		super(null);
		this.labelString = labelString;
		labelNo = -1;
	}
	
	public String toString()  {    return "label "+ ((labelNo < 0) ? labelString : ""+labelNo );  }
	public void find_out_my_type() throws PlcException { if(type == null) type = new PhTypeUnknown(); }
	public boolean is_const() { return true; }

	public void preprocess_me( ParseState s ) throws PlcException {
	}

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException 
	{
		if(labelString!=null)
			c.markLabel(labelString);
		else
			c.markLabel("javaLabel"+labelNo);
	}

}
