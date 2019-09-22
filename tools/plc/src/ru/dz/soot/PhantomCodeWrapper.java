package ru.dz.soot;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.PhantomVariable;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpDynamicMethodCallNode;
import ru.dz.plc.compiler.binode.OpSubscriptNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.util.PlcException;
import soot.SootClass;
import soot.SootMethod;
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
import soot.jimple.internal.JInstanceFieldRef;
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




	public static PhantomCodeWrapper getExpression(Value v, Method m, PhantomClass c, ParseState ps) throws PlcException {
		//String vClass = v.getClass().toString();
		//System.err.print(" ?? expr class = "+vClass);
		
		SootExpressionTranslator et = new SootExpressionTranslator( v, m, c, ps );
		return et.process();
	}


	public static PhantomCodeWrapper getReturnValueNode(PhantomCodeWrapper v) {
		return new PhantomCodeWrapper(new ru.dz.plc.compiler.node.ReturnNode(v.n));
	}



// source type
	public static PhantomCodeWrapper getAssign(Value assignTo,
			PhantomCodeWrapper expression, Method m, PhantomClass pc, ParseState ps) throws PlcException {
		
		if( expression == null )
		{
			SootMain.say("null expr!");
			expression = new PhantomCodeWrapper(new NullNode());
		}
		
		Node dest = null;
		
		if(assignTo instanceof JimpleLocal)
		{
			JimpleLocal jl = (JimpleLocal)assignTo;
			dest = new IdentNode(jl.getName(), ps);
			
			Type type = jl.getType();
			
			try {
				PhantomType ptype = SootExpressionTranslator.convertType(type);
				//SootMain.say("var "+jl.getName()+" type "+ptype);
				m.svars.addStackVar(new PhantomVariable(jl.getName(), ptype));
			} catch (PlcException e) {
				dest = null;
				SootMain.error(e);
			}
			

		}

		if (assignTo instanceof JArrayRef) {
			JArrayRef ar = (JArrayRef) assignTo;
			
			Node base   = getExpression(ar.getBase(),  m, pc, ps).getNode();			
			Node subscr = getExpression(ar.getIndex(), m, pc, ps).getNode();

			dest = new OpSubscriptNode(base, subscr);			
		}
		
		if (assignTo instanceof JInstanceFieldRef) {
			JInstanceFieldRef fr = (JInstanceFieldRef) assignTo;
			
			String name = fr.getField().getName();
			dest = new IdentNode( name, ps ); 
			
			try {
				PhantomType ptype = SootExpressionTranslator.convertType(fr.getType());
				pc.addField(name, ptype);
			} catch (PlcException e) {
				dest = null;
				SootMain.error(e);
			}
			
			
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


	public static PhantomCodeWrapper getInvoke(InvokeExpr expr, Method m, PhantomClass phantomClass, ParseState ps ) throws PlcException
	{
		boolean dumpMe = true;
		
		SootMethodRef methodRef = expr.getMethodRef();
		String name = methodRef.name();
		
		if(dumpMe) SootMain.say("      ."+name);
		// TODO find out static cases, replace
		// TODO how do we know if class is dynamically loaded in
		// runtime and static call becomes dynamic again?

		Node object = null;
		//Node method = null;
		Node args   = null;

		args = makeArgs(expr, m, phantomClass, ps);
		
		boolean done = false;
		
		if (expr instanceof VirtualInvokeExpr) {
			VirtualInvokeExpr ie = (VirtualInvokeExpr) expr;

			if(dumpMe) SootMain.say("      invoke virt "+ie);
			
			
			object = PhantomCodeWrapper.getExpression(ie.getBase(), m, phantomClass, ps).getNode();
			done = true;
		}

		if (expr instanceof StaticInvokeExpr) {
			StaticInvokeExpr ie = (StaticInvokeExpr) expr;

			if(dumpMe) SootMain.say("      invoke static "+ie);
			object = new NullNode();
			// TODO is it right?
			done = true;
		}

		if (expr instanceof SpecialInvokeExpr) {
			SpecialInvokeExpr ie = (SpecialInvokeExpr) expr;
			// TODO is it right?
			
			if(dumpMe)
			{
				SootMain.say("      invoke special "+ie);
				SootMain.say("      invoke special type="+ie.getType());
				SootMain.say("      invoke special name='"+name+"'");
				//SootMain.say("      invoke special signature="+methodRef.getSignature());
				SootMethod method = ie.getMethod();
				SootClass declaringClass = method.getDeclaringClass();
				SootMain.say("      invoke special decl class="+declaringClass);
				PhantomType convertType = SootExpressionTranslator.convertType(declaringClass.getName());
				SootMain.say("      invoke special phantom type ="+convertType);

				if( (convertType.is_void() ||  convertType.get_main_class_name().equals(".internal.object")) && name.equals("<init>"))
				{
					// TODO hack skip calling init for void - or else we're in a loop of inits
					// TODO make real static invoke for constructors!
					// TODO call c'tor for void?
					// TODO remove fixed constructor method slot in VM?
					SootMain.warning("skip <init> for void in "+ie);
					return new PhantomCodeWrapper(new NullNode());
				}
	
			}
			object = PhantomCodeWrapper.getExpression(ie.getBase(), m, phantomClass, ps).getNode();
			done = true;
		}
		
		
		if (expr instanceof InterfaceInvokeExpr) {
			InterfaceInvokeExpr ie = (InterfaceInvokeExpr) expr;
			
			if(dumpMe) SootMain.say("      invoke iface "+ie);
			// Do it like dynamic
			object = PhantomCodeWrapper.getExpression(ie.getBase(), m, phantomClass, ps).getNode();
			done = true;
		}
		
		if (expr instanceof DynamicInvokeExpr) {
			DynamicInvokeExpr ie = (DynamicInvokeExpr) expr;
			
			if(dumpMe) SootMain.say("      invoke dynamic "+ie);
			object = new NullNode();
			done = true;
		}
		
		
		if(!done)
		{
			SootMain.error("Unknown invoke "+expr+" class "+expr.getClass());
			return new PhantomCodeWrapper(new NullNode());
		}
		/*
		// TODO hack!
		if( name.equals("<init>") )
		{
			// if we call init for class
			
		}
		*/
		
		OpDynamicMethodCallNode node = new OpDynamicMethodCallNode(object, name, args); 	
		node.presetType(SootExpressionTranslator.convertType(expr.getType()));
		return new PhantomCodeWrapper(node);
	}

	private static Node makeArgs(InvokeExpr expr, Method m, PhantomClass pc, ParseState ps) throws PlcException {
		//int argCount = expr.getArgCount();		
		//return makeArg( expr, argCount, m, pc );	
		return makeArg( expr, 0, m, pc, ps );
	}

	private static Node makeArg(InvokeExpr expr, int argNo, Method m, PhantomClass phantomClass, ParseState ps) throws PlcException {
		//argNo--;
		
		int argCount = expr.getArgCount();
		if( argNo >= argCount ) return null; // codegen stops on null
		
		Node node = PhantomCodeWrapper.getExpression(expr.getArg(argNo), m, phantomClass, ps).getNode();

		
		// wrong order?
		return new SequenceNode(node, makeArg(expr, argNo+1, m, phantomClass, ps));
		// try reverse - fails!
		//return new SequenceNode( makeArg(expr, argNo+1, m, phantomClass), node );
	}

	
	
}
