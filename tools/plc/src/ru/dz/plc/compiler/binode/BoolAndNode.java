package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Boolean AND node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class BoolAndNode extends BinaryOpNode 
{
	public BoolAndNode( Node l, Node r) {    super(l,r);  }
	public String toString()  {    return "bool and";  }
	public boolean is_on_int_stack() { return true; }


	@Override
	String getLlvmOpName() { return "bool_and"; }


	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		//if(getType().is_int()) c.emitLogAnd();		else throw new PlcException("Codegen", "op && does not exist for this type");
		generateIntegerStackOp(c, () -> c.emitLogAnd() );
	}
}
