package ru.dz.soot;

import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.NullNode;
import soot.Value;

public class PhantomCodeWrapper {
	
	private ru.dz.plc.compiler.node.Node n;

	private PhantomCodeWrapper(ru.dz.plc.compiler.node.Node node) {
		n = node;
	}

	
	
	
	public static PhantomCodeWrapper getNullNode() {
		return new PhantomCodeWrapper(new NullNode());
	}

	public static PhantomCodeWrapper getReturnNode() {
		return new PhantomCodeWrapper(new ru.dz.plc.compiler.node.ReturnNode(new NullNode()));
	}

	
	public static PhantomCodeWrapper getJumpNode(String label) {
		
		return new PhantomCodeWrapper( new ru.dz.plc.compiler.node.JumpNode(label) );
	}




	public static PhantomCodeWrapper getExpression(Value v) {
		String vClass = v.getClass().toString();
		System.err.print(" ?? expr class = "+vClass);
		
		// TODO implement me
		return new PhantomCodeWrapper( new NullNode() );
	}


	public static PhantomCodeWrapper getReturnValueNode(PhantomCodeWrapper v) {
		return new PhantomCodeWrapper(new ru.dz.plc.compiler.node.ReturnNode(v.n));
	}

	
}
