package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Reference equality comparison node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class RefEqNode extends BiNode 
{
	public RefEqNode( Node l, Node r) {    super(l,r);  }
	public boolean is_on_int_stack() { return true; }
	public boolean args_on_int_stack() { return false; }
	public String toString()  {    return ":==";  }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		c.emit_ptr_eq();
	}
}
