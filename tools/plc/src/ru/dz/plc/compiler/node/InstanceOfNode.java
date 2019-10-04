package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;


/**
 * <p>Nonzero if given object is instance of given class.</p>
 * <p>Copyright: Copyright (c) 2004-2013 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class InstanceOfNode extends Node {
	private PhantomType checkType;

	public InstanceOfNode(Node expr, PhantomType type) throws PlcException
	{
		super(expr);
		checkType = type; 
		if( checkType == null)
			throw new PlcException( toString(), "no type known" );
	}

	@Override
	public String toString()  {    return "instance of " + (checkType != null ? checkType.toString() : "?");  }

	@Override
	public boolean is_on_int_stack() { return false; }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		checkType.emit_get_class_object(c,s);
		c.emitCall(14, 1); // Class object, 1 arg - object to check type of
		// Result is returned
	}
	
	
	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s)
			throws PlcException {
/*
		checkType.emit_get_class_object(c,s);
		
		emitMethodCall( Node new_this, 14, null, s );
*/
		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"IsInstanceOf( ");
		_l.generate_C_code(cgen, s);
		cgen.put(", ");
		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"GetClass( \"");
		cgen.put(checkType.get_main_class_name());
		cgen.put("\" ) ");
	}
}
