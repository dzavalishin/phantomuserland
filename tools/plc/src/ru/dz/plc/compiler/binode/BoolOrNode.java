package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Boolean OR node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class BoolOrNode extends BinaryOpNode 
{
	public BoolOrNode( Node l, Node r) {    super(l,r);  }
	public String toString()  {    return "bool or";  }
	public boolean is_on_int_stack() { return true; }

	@Override
	String getLlvmOpName() { return "bool_and"; }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		generateIntegerStackOp(c, () -> c.emitLogOr() );
		/*
	    if(getType().is_on_int_stack())
	    {
	    	c.emitNumericPrefix(getType());
	    	c.emitLogOr();
	    }
	    else throw new PlcException("Codegen", "op || does not exist for this type");
		 */
	}
}
