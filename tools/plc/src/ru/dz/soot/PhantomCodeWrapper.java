package ru.dz.soot;

import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import soot.Value;
import soot.jimple.internal.JimpleLocal;

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
		//String vClass = v.getClass().toString();
		//System.err.print(" ?? expr class = "+vClass);
		
		SootExpressionTranslator et = new SootExpressionTranslator( v );
		return et.process();
	}


	public static PhantomCodeWrapper getReturnValueNode(PhantomCodeWrapper v) {
		return new PhantomCodeWrapper(new ru.dz.plc.compiler.node.ReturnNode(v.n));
	}




	public static PhantomCodeWrapper getAssign(Value assignTo,
			PhantomCodeWrapper expression) {
		
		if( expression == null )
		{
			SootMain.say("null expr!");
			expression = new PhantomCodeWrapper(new NullNode());
		}
		
		// TODO implement
		Node dest = null;
		
		if(assignTo instanceof JimpleLocal)
		{
			JimpleLocal jl = (JimpleLocal)assignTo;
			dest = new IdentNode(jl.getName());
		}

		if(dest == null)
		{
			dest = new NullNode();
			String vClass = assignTo.getClass().toString();
			System.err.print(" ?? assignable class = "+vClass);			
		}
		
		OpAssignNode node = new OpAssignNode(dest, expression.n);
		return new PhantomCodeWrapper(node);
	}




	public static PhantomCodeWrapper getReadLocal(String varName) {
		return new PhantomCodeWrapper(new IdentNode(varName));
	}

	
}
