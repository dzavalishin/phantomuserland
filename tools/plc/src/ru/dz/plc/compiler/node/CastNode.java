package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

public class CastNode extends Node {

	private PhantomType pt;

	public CastNode(Node l, PhantomType pt) throws PlcException {    
		super(l);
		this.pt = pt;
		setType(pt);
	}

	public String toString()  {    return "(cast)";  }

	public void preprocess_me( ParseState s ) throws PlcException  {  }

	public boolean is_on_int_stack() { return _l.is_on_int_stack(); }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException 
	{
		if(!is_on_int_stack())
		{
			generate_object_cast(c);
			return;
		}

		if(!pt.is_on_int_stack())
		{
			c.emitNumericPrefix(pt);
			c.emit_i2o();
			generate_object_cast(c);
		}
		else
			c.emitNumericCast(_l.getType(), pt);
	}



	private void generate_object_cast(Codegen c) throws IOException 
	{
		// TODO must check type compatibility
		c.emitSummonByName(pt.get_main_class_name());
		c.emitCast();
	}

	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException
	{
		cgen.put("( ("+pt.to_C_Type()+") (");
		_l.generate_C_code(cgen, s);
		cgen.put("))");
	}

}
