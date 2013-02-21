package ru.dz.soot;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.PhTypeInt;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.PhantomVariable;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.util.PlcException;
import soot.Type;
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




	public static PhantomCodeWrapper getExpression(Value v, Method m, PhantomClass c) throws PlcException {
		//String vClass = v.getClass().toString();
		//System.err.print(" ?? expr class = "+vClass);
		
		SootExpressionTranslator et = new SootExpressionTranslator( v, m, c );
		return et.process();
	}


	public static PhantomCodeWrapper getReturnValueNode(PhantomCodeWrapper v) {
		return new PhantomCodeWrapper(new ru.dz.plc.compiler.node.ReturnNode(v.n));
	}




	public static PhantomCodeWrapper getAssign(Value assignTo,
			PhantomCodeWrapper expression, Method m) {
		
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
			
			Type type = jl.getType();
			
			PhantomType ptype;
			try {
				ptype = new PhTypeInt();
			} catch (PlcException e) {
				// TODO Auto-generated catch block
				ptype = PhantomType.t_void;
			}
			
			m.svars.add_stack_var(new PhantomVariable(jl.getName(), ptype));

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



	/*
	public static PhantomCodeWrapper getReadLocal(String varName) {
		return new PhantomCodeWrapper(new IdentNode(varName));
	}
	*/
	
}
