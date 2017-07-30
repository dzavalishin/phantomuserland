package ru.dz.jpc.python;

import ru.dz.plc.compiler.node.MultiuseNode;

// This one is for compiling Python register-based internal representation
// Not yet finished
public class RegisterNodeWrapper 
{
	private final ru.dz.plc.compiler.node.Node plcNode;
	private int useCount = 0;

	public RegisterNodeWrapper(ru.dz.plc.compiler.node.Node plcNode ) {
		this.plcNode = new MultiuseNode( plcNode, this );		
	}
	
	public ru.dz.plc.compiler.node.Node use()
	{
		useCount++;
		return plcNode;		
	}

	public int getUseCount() {
		return useCount;
	}
}

