package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;


public 
class OpShiftRightUnsignedNode extends BinaryOpNode 
{
	public OpShiftRightUnsignedNode(Node l, Node r) {    super(l,r);  }
	public boolean is_on_int_stack() { return true; }
	public String toString()  {    return ">>u";  }
	 
    @Override
    String getLlvmOpName() { return "lshr"; }
    
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException 
	{
		//if(getType().is_int()) c.emitUShiftRight();
		//else throw new PlcException("Codegen", "op >> (unsigned) does not exist for this type");
		generateIntegerStackOp(c, () -> c.emitUShiftRight() );
	}
}
