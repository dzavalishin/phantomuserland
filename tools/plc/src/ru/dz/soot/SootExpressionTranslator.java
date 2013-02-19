package ru.dz.soot;

import ru.dz.plc.compiler.binode.OpMinusNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import soot.SootMethodRef;
import soot.Type;
import soot.Value;
import soot.jimple.IntConstant;
import soot.jimple.internal.AbstractBinopExpr;
import soot.jimple.internal.JAddExpr;
import soot.jimple.internal.JArrayRef;
import soot.jimple.internal.JStaticInvokeExpr;
import soot.jimple.internal.JSubExpr;
import soot.jimple.internal.JVirtualInvokeExpr;
import soot.jimple.internal.JimpleLocal;

public class SootExpressionTranslator {

	private Value root;

	public SootExpressionTranslator(Value v) {
		this.root = v;
	}

	public PhantomCodeWrapper process()
	{
		PhantomCodeWrapper ret = doValue(root);
		return ret;
	}

	private PhantomCodeWrapper doValue(Value v) 
	{

		if( v instanceof JimpleLocal )
			return doReadLocal((JimpleLocal)v);

		if( v instanceof JAddExpr )
			return doAdd((JAddExpr)v);
		
		if( v instanceof JSubExpr )
			return doSub((JSubExpr)v);
		
		if( v instanceof IntConstant )
			return new PhantomCodeWrapper( new IntConstNode(((IntConstant)v).value));
		
		if( v instanceof JArrayRef )
			return doArrayRef((JArrayRef)v);
		

		if( v instanceof JVirtualInvokeExpr )
			return doVirtualInvoke((JVirtualInvokeExpr)v);
		
		if( v instanceof JStaticInvokeExpr )
			return doStaticInvoke((JStaticInvokeExpr)v);
		
		say("e ?? "+v.getClass().getName());
		say("e    "+v.toString());

		return PhantomCodeWrapper.getNullNode();
	}

	/*
	private <T extends BiNode> PhantomCodeWrapper doBinOp(AbstractBinopExpr v) {
		Type t = v.getType();
		// TODO type?
		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		//return new PhantomCodeWrapper( new T(e1n,e2n) );
	}
    */
	

	private PhantomCodeWrapper doStaticInvoke(JStaticInvokeExpr v) {
		SootMethodRef mr = v.getMethodRef();
		String mName = mr.name();
		mr.declaringClass();
		say("Static call "+mName);
		// TODO Auto-generated method stub
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doVirtualInvoke(JVirtualInvokeExpr v) {
		SootMethodRef mr = v.getMethodRef();
		String mName = mr.name();
		say("Virtual call "+mName);
		// TODO Auto-generated method stub
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doArrayRef(JArrayRef v) {
		// TODO impl
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doAdd(JAddExpr v) {
		Type t = v.getType();
		// TODO type?
		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		return new PhantomCodeWrapper( new OpPlusNode(e1n,e2n));
	}
	
	private PhantomCodeWrapper doSub(JSubExpr v) {
		Type t = v.getType();
		// TODO type?
		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		return new PhantomCodeWrapper( new OpMinusNode(e1n,e2n));
	}

	private PhantomCodeWrapper doReadLocal(JimpleLocal v) {
		String varName = v.getName();
		return PhantomCodeWrapper.getReadLocal( varName );
	}

	private void say(String string) {
		System.err.println(string);
	}
	
}
