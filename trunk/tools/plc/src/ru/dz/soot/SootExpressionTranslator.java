package ru.dz.soot;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.PhTypeInt;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.PhantomVariable;
import ru.dz.plc.compiler.binode.OpMinusNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.util.PlcException;
import soot.SootMethodRef;
import soot.Type;
import soot.Value;
import soot.jimple.IntConstant;
import soot.jimple.internal.AbstractBinopExpr;
import soot.jimple.internal.JAddExpr;
import soot.jimple.internal.JArrayRef;
import soot.jimple.internal.JLengthExpr;
import soot.jimple.internal.JStaticInvokeExpr;
import soot.jimple.internal.JSubExpr;
import soot.jimple.internal.JVirtualInvokeExpr;
import soot.jimple.internal.JimpleLocal;

public class SootExpressionTranslator {

	private Value root;
	private Method m;

	public SootExpressionTranslator(Value v, Method m) {
		this.root = v;
		this.m = m;
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
		
		
		if( v instanceof JLengthExpr )
			return doLength((JLengthExpr)v);
		

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
	

	private PhantomCodeWrapper doLength(JLengthExpr v) {
		// TODO array len op
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doStaticInvoke(JStaticInvokeExpr v) {
		SootMethodRef mr = v.getMethodRef();
		String mName = mr.name();
		mr.declaringClass();
		say("Static call "+mName);
		// TODO static call
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doVirtualInvoke(JVirtualInvokeExpr v) {
		SootMethodRef mr = v.getMethodRef();
		String mName = mr.name();
		say("Virtual call "+mName);
		// TODO virt call
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doArrayRef(JArrayRef v) {
		// TODO impl
		return new PhantomCodeWrapper(new NullNode());
	}

	private PhantomCodeWrapper doAdd(JAddExpr v) {
		Type t = v.getType();
		// TODO type?
		assertInt(t);

		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		return new PhantomCodeWrapper( new OpPlusNode(e1n,e2n));
	}
	
	private PhantomCodeWrapper doSub(JSubExpr v) {
		Type t = v.getType();
		// TODO type?
		assertInt(t);
		
		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		return new PhantomCodeWrapper( new OpMinusNode(e1n,e2n));
	}

	private void assertInt(Type t) {
		if( t.toString().equals("int"))
			return;
		//Type machineType = Type.toMachineType(t);
		say("unknown type "+t);
		//say("t1 "+machineType);
		throw new RuntimeException("Unknown type "+t);
	}

	private PhantomCodeWrapper doReadLocal(JimpleLocal v) {
		String varName = v.getName();
		/*
		Type type = v.getType();
		
		PhantomType ptype;
		try {
			ptype = new PhTypeInt();
		} catch (PlcException e) {
			// TODO Auto-generated catch block
			ptype = PhantomType.t_void;
		}
		
		m.svars.add_stack_var(new PhantomVariable(varName, ptype));
		*/
		IdentNode node = new IdentNode( varName );
		return new PhantomCodeWrapper( node );
	}

	private void say(String string) {
		System.err.println(string);
	}
	
}
