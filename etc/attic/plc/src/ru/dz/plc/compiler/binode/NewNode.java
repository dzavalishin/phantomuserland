package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;


/**
 * <p>New (create object) node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class NewNode extends BiNode {
	PhantomType static_type = null;
	public NewNode(PhantomType static_type, Node dynamic_type, Node args)
	{
		super(dynamic_type,args);
		this.static_type = static_type;
	}

	@Override
	public
	void find_out_my_type() throws PlcException {

		if(static_type != null)
		{
			type = static_type;
			return;
		}

		super.find_out_my_type();
	}

	public String toString()  {    return "new " + ((static_type == null) ? "(dynamic type)" : static_type.toString());  }

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		//if( _l != null ) { _l.generate_code(c,s); move_between_stacks(c, _l.is_on_int_stack()); }
		//if( _r != null ) { _r.generate_code(c,s); move_between_stacks(c, _r.is_on_int_stack()); }
		generate_my_code(c,s);
	}
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {

		if( static_type == null && _l == null)
			throw new PlcException( "new Node", "no type known" );

		if( _l != null )
		{
			_l.generate_code(c,s);
			if(_l.is_on_int_stack())
				throw new PlcException( "new Node", "type expression can't be int" );

		}
		else
		{
			static_type.emit_get_class_object(c,s);
		}

		c.emitNew();

		if( _r != null )
		{
			_r.generate_code(c,s);
			throw new PlcException( "new Node", "no constructor parameters yet" );
		}

	}
}
