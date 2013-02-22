package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.MethodNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.trinode.TriNode;
import ru.dz.plc.util.PlcException;

/**
 * <p>Method call node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class OpDynamicMethodCallNode extends BiNode {
	PhantomType obj_type = null; // type of object, which is used as 'this' in call
	private String methodName;

	public OpDynamicMethodCallNode(Node object, String methodName, Node args) { super(object, args);
	this.methodName = methodName; }
	public String toString()  {    return ".dynamic.";  }

	@Override
	public boolean args_on_int_stack() {
		return super.args_on_int_stack();
	}

	public void preprocess_me( ParseState s ) throws PlcException
	{
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		generate_my_code(c,s);
	}

	public void find_out_my_type() throws PlcException {
		if( type == null ) throw new PlcException("Method call Node","return type is not set");
		//type = new ph_type_unknown(); // BUG! Wrong!
	}

	public void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{

		int n_param = 0;
		// bug - wrong count of args?
		for( Node i = _r; i != null; i = ((BiNode)i).getRight() )      
			n_param++;

		if( _r != null ) _r.generate_code(c,s); // calc args
		c.emitIConst_32bit(n_param); // n args
		
		_l.generate_code(c,s); // get object
		move_between_stacks(c, _l.is_on_int_stack());

		c.emitString(methodName);
		
		c.emitDynamicCall();
	}

}
