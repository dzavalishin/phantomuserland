package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * 
 * Value assignment. TODO implement?
 * @author dz
 *
 */
public class OpVassignNode extends BiNode {
	public OpVassignNode(Node l, Node r) {    super(l,r);  }
	public String toString()  {    return ".=";  }

	@Override
	protected void generate_my_code(Codegen c, CodeGeneratorState s)
	throws IOException, PlcException {
		throw new PlcException("OpVassignNode", "not implemented");
	}

}
