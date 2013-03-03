package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Array length node.</p>
 * <p>Copyright: Copyright (c) 2004-2012 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class OpArrayLength extends Node {

	public OpArrayLength(Node array) {    super(array);  }

	public String toString()  {    return ".length";  }

	@Override
	public void preprocess_me(ParseState s) throws PlcException {
		//super.preprocess_me(s);
	}

	public void find_out_my_type() throws PlcException
	{
		type = PhantomType.getInt();
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		Node atom = _l;

		if( !atom.getType().is_container() )
			throw new PlcException( toString(), "not a container" );

		log.fine("Node "+this+" codegen");

		// put array object to load from
		atom.generate_code(c,s);
		move_between_stacks(c, atom.is_on_int_stack());


		c.emitCall(12,0); // Method number 12, 0 parameters - size
	}
}