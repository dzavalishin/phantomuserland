package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;


/**
 * <p>Summon (bring to stack) class object.</p>
 * <p>Copyright: Copyright (c) 2004-2013 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class SummonClassNode extends Node {
	public SummonClassNode(PhantomType static_type) throws PlcException
	{
		super(null);
		setType( static_type );
	}

	@Override
	public
	void find_out_my_type() throws PlcException {
		super.find_out_my_type();
	}

	public String toString()  {    return "summon class " + type.toString();  }

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		generate_my_code(c,s);
	}
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		if( type == null)
			throw new PlcException( toString(), "no type known" );

		type.emit_get_class_object(c,s);
	}
}
