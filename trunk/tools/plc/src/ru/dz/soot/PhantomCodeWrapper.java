package ru.dz.soot;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.PhTypeInt;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.PhantomVariable;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpDynamicMethodCallNode;
import ru.dz.plc.compiler.binode.OpSubscriptNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.EmptyNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.JumpNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.trinode.OpMethodCallNode;
import ru.dz.plc.util.PlcException;
import soot.SootMethodRef;
import soot.Type;
import soot.Value;
import soot.jimple.DynamicInvokeExpr;
import soot.jimple.InterfaceInvokeExpr;
import soot.jimple.InvokeExpr;
import soot.jimple.SpecialInvokeExpr;
import soot.jimple.StaticInvokeExpr;
import soot.jimple.VirtualInvokeExpr;
import soot.jimple.internal.JArrayRef;
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
			PhantomCodeWrapper expression, Method m, PhantomClass pc) throws PlcException {
		
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
			
			try {
				PhantomType ptype = SootExpressionTranslator.convertType(type);
				m.svars.add_stack_var(new PhantomVariable(jl.getName(), ptype));
			} catch (PlcException e) {
				dest = null;
				SootMain.error(e);
			}
			

		}

		if (assignTo instanceof JArrayRef) {
			JArrayRef ar = (JArrayRef) assignTo;
			
			Node base   = getExpression(ar.getBase(),  m, pc).getNode();			
			Node subscr = getExpression(ar.getIndex(), m, pc).getNode();

			dest = new OpSubscriptNode(base, subscr);			
		}
		
		if(dest == null)
		{
			dest = new NullNode();
			String vClass = assignTo.getClass().toString();
			SootMain.error(" ?? assignable class = "+vClass);			
		}
		
		OpAssignNode node = new OpAssignNode(dest, expression.n);
		return new PhantomCodeWrapper(node);
	}


	public static PhantomCodeWrapper getInvoke(InvokeExpr expr, Method m, PhantomClass phantomClass ) throws PlcException
	{
		SootMethodRef methodRef = expr.getMethodRef();
		String name = methodRef.name();
		SootMain.say("      ."+name);
		// TODO make invoke

		Node object = null;
		//Node method = null;
		Node args   = null;

		args = makeArgs(expr, m, phantomClass);
		
		boolean done = false;
		
		if (expr instanceof VirtualInvokeExpr) {
			VirtualInvokeExpr ie = (VirtualInvokeExpr) expr;

			SootMain.say("      invoke virt "+ie);
			
			
			object = PhantomCodeWrapper.getExpression(ie.getBase(), m, phantomClass).getNode();
			done = true;
		}

		if (expr instanceof StaticInvokeExpr) {
			StaticInvokeExpr ie = (StaticInvokeExpr) expr;

			SootMain.say("      invoke static "+ie);
			object = new NullNode();
			
			done = true;
		}

		if (expr instanceof SpecialInvokeExpr) {
			SpecialInvokeExpr ie = (SpecialInvokeExpr) expr;
			
			SootMain.say("      invoke special "+ie);
			object = PhantomCodeWrapper.getExpression(ie.getBase(), m, phantomClass).getNode();
			done = true;
		}
		
		
		if (expr instanceof InterfaceInvokeExpr) {
			InterfaceInvokeExpr ie = (InterfaceInvokeExpr) expr;
			
			SootMain.say("      invoke iface "+ie);
			// Do it like dynamic
			object = PhantomCodeWrapper.getExpression(ie.getBase(), m, phantomClass).getNode();
			done = true;
		}
		
		if (expr instanceof DynamicInvokeExpr) {
			DynamicInvokeExpr ie = (DynamicInvokeExpr) expr;
			
			SootMain.say("      invoke dynamic "+ie);
			object = new NullNode();
			done = true;
		}
		
		
		if(!done)
		{
			SootMain.error("Unknown invoke "+expr+" class "+expr.getClass());
			return new PhantomCodeWrapper(new NullNode());
		}

		OpDynamicMethodCallNode node = new OpDynamicMethodCallNode(object, name, args); 	
		node.setType(SootExpressionTranslator.convertType(expr.getType()));
		return new PhantomCodeWrapper(node);
	}

	private static Node makeArgs(InvokeExpr expr, Method m, PhantomClass pc) throws PlcException {
		int argCount = expr.getArgCount();		
		return makeArg( expr, argCount, m, pc );
	}

	private static Node makeArg(InvokeExpr expr, int argNo, Method m, PhantomClass phantomClass) throws PlcException {
		argNo--;
		
		if( argNo < 0 ) return null; // codegen stops on null
		
		Node node = PhantomCodeWrapper.getExpression(expr.getArg(argNo), m, phantomClass).getNode();;
		
		return new SequenceNode(node, makeArg(expr, argNo, m, phantomClass));
	}

	
	
}
