package ru.dz.plc.compiler.trinode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.MethodSignature;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.binode.BiNode;
import ru.dz.plc.compiler.node.MethodNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Method call node.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */


public class OpMethodCallNode extends TriNode 
{
	PhantomType obj_type = null; // type of object, which is used as 'this' in call

	public OpMethodCallNode(Node object, Node method, Node args) 
	{ 
		super(object, method, args); 
	}
	
	public String toString()  {    return ".";  }

	@Override
	public boolean args_on_int_stack() {
		return super.args_on_int_stack();
	}

	public void preprocess_me( ParseState s ) throws PlcException
	{
		obj_type = _l.getType();

		MethodNode method = (MethodNode)_m;

		MethodSignature sig = new MethodSignature(method.getIdent(), _r);
		method.setSignature(sig);

		presetType(method.get_return_type(obj_type));
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		// NB! Reverse order - _l, then _r - this opcode is brain damaged,
		// it needs args pushed AFTER object to call
		_l.generate_code(c,s); // get object
		move_between_stacks(c, _l);
		
		if( _r != null ) _r.generate_code(c,s); // calc args
		
	    log.fine("Node "+this+" codegen");

		generate_my_code(c,s);
	}

	public PhantomType find_out_my_type() throws PlcException {
		print_error("return type is not set");
		throw new PlcException("Method call Node","return type is not set");
	}

	public void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		MethodNode method = (MethodNode)_m;

		//System.out.println("calling Method "+Method.ident+" of "+obj_type.toString());

		int method_ordinal = method.get_ordinal(obj_type);

		int n_param = 0;

		// bug - wrong count of args?
		for( Node i = _r; i != null; i = ((BiNode)i).getRight() )      n_param++;

		c.emitCall(method_ordinal,n_param);
	}

	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException 
	{
		MethodNode method = (MethodNode)_m;

		int method_ordinal = method.get_ordinal(obj_type);
		
		cgen.putln("// Call method "+method.getIdent());

		cgen.emitMethodCall( _l,  method_ordinal, _r, s );

	}
	
}
