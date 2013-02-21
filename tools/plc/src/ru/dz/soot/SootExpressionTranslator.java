package ru.dz.soot;

import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.PhTypeInt;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.PhantomVariable;
import ru.dz.plc.compiler.binode.OpMinusNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.binode.OpSubscriptNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.node.OpArrayLength;
import ru.dz.plc.compiler.node.StringConstNode;
import ru.dz.plc.util.PlcException;
import soot.SootMethodRef;
import soot.Type;
import soot.Value;
import soot.jimple.IntConstant;
import soot.jimple.StringConstant;
import soot.jimple.internal.AbstractBinopExpr;
import soot.jimple.internal.JAddExpr;
import soot.jimple.internal.JArrayRef;
import soot.jimple.internal.JLengthExpr;
import soot.jimple.internal.JStaticInvokeExpr;
import soot.jimple.internal.JSubExpr;
import soot.jimple.internal.JVirtualInvokeExpr;
import soot.jimple.internal.JimpleLocal;
import soot.util.Switch;

public class SootExpressionTranslator {

	private Value root;
	private Method m;

	public SootExpressionTranslator(Value v, Method m) {
		this.root = v;
		this.m = m;
	}

	public PhantomCodeWrapper process() throws PlcException
	{
		PhantomCodeWrapper ret = doValue(root);
		return ret;
	}

	public static PhantomType convertType( Type t ) throws PlcException
	{
		String tn = t.toString();
		
		if( tn.equals(" java.lang.Object")) tn = ".internal.object";
		if( tn.equals("int")) tn = ".internal.int";
		
		boolean err = false;
		
		if( tn.equals("long")) { tn = ".internal.long"; err = true; }
		if( tn.equals("float")) { tn = ".internal.float"; err = true; }
		if( tn.equals("double")) { tn = ".internal.double"; err = true; }
		
		if( err ) SootMain.error("no type"+tn);
		
		PhantomClass pc = new PhantomClass(tn);
		/*if(pc == null)
		{
			SootMain.error("type not found"+tn);
			return null;
		}*/
		
		PhantomType pt = new PhantomType(pc);
		return pt;
	}
	
	private PhantomCodeWrapper doValue(Value v) throws PlcException 
	{
		if( v instanceof StringConstant )
			return doStringConst((StringConstant)v);

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
	

	private PhantomCodeWrapper doStringConst(StringConstant v) {
		return new PhantomCodeWrapper(new StringConstNode(v.value));
	}

	private PhantomCodeWrapper doLength(JLengthExpr v) throws PlcException {
		Value array = v.getOp();
		
		return new PhantomCodeWrapper(new OpArrayLength(PhantomCodeWrapper.getExpression(array, m).getNode()));
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

	private PhantomCodeWrapper doArrayRef(JArrayRef v) throws PlcException {
		Value base = v.getBase();
		Value index = v.getIndex();
		
		Node array = doValue(base).getNode();
		Node subscr = doValue(index).getNode();
		
		OpSubscriptNode subscriptNode = new OpSubscriptNode(array,subscr);
		return new PhantomCodeWrapper(subscriptNode);
	}

	private PhantomCodeWrapper doAdd(JAddExpr v) throws PlcException {
		Type t = v.getType();
		assertInt(t);

		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		OpPlusNode node = new OpPlusNode(e1n,e2n);
		node.setType(convertType(t));
		
		return new PhantomCodeWrapper( node );
	}
	
	private PhantomCodeWrapper doSub(JSubExpr v) throws PlcException {
		Type t = v.getType();
		assertInt(t);
		
		Value e1 = v.getOp1();
		Value e2 = v.getOp2();
		
		Node e1n = doValue(e1).getNode();
		Node e2n = doValue(e2).getNode();
		
		OpMinusNode node = new OpMinusNode(e1n,e2n);
		node.setType(convertType(t));
		return new PhantomCodeWrapper( node);
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
