
package ru.dz.plc.compiler.binode;
/*
import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;
import ru.dz.soot.SootMain;
*/
/**
 * <p>Constructor call node.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */

import ru.dz.plc.compiler.Method;

/*
public class ConstructNode extends Node {

	public ConstructNode(Node args) {
		super(args);		
	}

	@Override
	public
	void find_out_my_type() throws PlcException 
	{
		type = _l.getType(); // Exactly
	}
	
	
	@Override
	public String toString() {	
		return "constructor()";
	}
	
	@Override
	public boolean is_on_int_stack() { return false; }

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		generate_my_code(c,s);
	}

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException 
	{
		if(context != null)
		{
			c.emitComment("Line "+context.getLineNumber());
			c.recordLineNumberToIPMapping(context.getLineNumber());
			c.emitComment("; call constructor");
		}
		
		// new object
//		_l.generate_code(c, s);
//		move_between_stacks(c, _l.is_on_int_stack());		
//		c.emitOsDup(); // dup new object ptr

		// args - TODO are we sure they're on obj stack?
		if( _l != null ) {
			_l.generate_code(c, s);
			//move_between_stacks(c, _l.is_on_int_stack());
		}

		//int method_ordinal = method.get_ordinal(obj_type);

		int n_param = 0;

		// bug - wrong count of args?
		for( Node i = _l; i != null; i = ((BiNode)i).getRight() )      n_param++;

		if( n_param > 0 )
			throw new PlcException(context.get_context(), "can generate just argless c'tors, sorry" );
		
		
		int method_ordinal = 0; // .internal.object constructor
		
		PhantomClass pclass = _l.getType().get_class();
		if( pclass != null )
		{
			Method cm = pclass.findMethod(Method.CONSTRUCTOR_M_NAME);
			if( cm != null ) method_ordinal = cm.getOrdinal();
		}
		else
			SootMain.warning("Can't call c'tor for "+_l.getType());
		
		
		//c.emitCall(method_ordinal,n_param);
		c.emitStaticCall(method_ordinal, n_param);
		
		c.emitOsDrop(); // c'tor is void
		
	}	
	
}
*/