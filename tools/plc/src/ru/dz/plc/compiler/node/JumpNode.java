package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * Jump Node. Generates jump to label.
 */

public class JumpNode extends Node {
	private final int labelNo;
	private final String label;

	//public String get_name() { return ident; }

	public JumpNode( int labelNo ) {
		super(null);
		this.labelNo = labelNo;
		this.label = null;
	}
	public JumpNode(String label) {
		super(null);
		this.label = label;
		this.labelNo = -1;
	}
	
	//public String toString()  {    return "jmp "+labelNo;  }
	public String toString()  {    return "jump "+ ((labelNo < 0) ? label: ""+labelNo );  }

	public PhantomType find_out_my_type() throws PlcException { return PhantomType.getVoid(); }
	public boolean is_const() { return true; }

	public void preprocess_me( ParseState s ) throws PlcException {
	}

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException 
	{
		if(label != null )
			c.emitJmp(label);
		else
			c.emitJmp("javaLabel"+labelNo);
	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		if(label != null )
			llc.putln("br label %"+label+" ;");
		else
			llc.putln("br label %javaLabel"+labelNo+" ;");
	}
	
}
