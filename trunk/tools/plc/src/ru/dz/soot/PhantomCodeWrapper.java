package ru.dz.soot;

import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import soot.Value;

public class PhantomCodeWrapper {
	
	private ru.dz.plc.compiler.node.Node n;

	public Node getNode() { return n; }
	
	public PhantomCodeWrapper(ru.dz.plc.compiler.node.Node node) {
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
		
		//new SootExpressionTranslator( )
		
		// TODO implement me
		return new PhantomCodeWrapper( new NullNode() );
	}


	public static PhantomCodeWrapper getReturnValueNode(PhantomCodeWrapper v) {
		return new PhantomCodeWrapper(new ru.dz.plc.compiler.node.ReturnNode(v.n));
	}




	public static PhantomCodeWrapper getAssign(Value assignTo,
			PhantomCodeWrapper expression) {
		
		String vClass = assignTo.getClass().toString();
		System.err.print(" ?? assignable class = "+vClass);
		
		// TODO implement
		Node dest = null;
		return new PhantomCodeWrapper(new ru.dz.plc.compiler.binode.OpAssignNode(dest, expression.n));
	}




	public static PhantomCodeWrapper getReadLocal(String varName) {
		return new PhantomCodeWrapper(new IdentNode(varName));
	}

	
}
