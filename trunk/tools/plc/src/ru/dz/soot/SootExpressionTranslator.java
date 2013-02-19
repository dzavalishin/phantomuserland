package ru.dz.soot;

import ru.dz.plc.compiler.binode.BiNode;
import ru.dz.plc.compiler.binode.OpMinusNode;
import ru.dz.plc.compiler.binode.OpPlusNode;
import ru.dz.plc.compiler.node.BinaryConstNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.IntConstNode;
import ru.dz.plc.compiler.node.Node;
import soot.Type;
import soot.Value;
import soot.jimple.IntConstant;
import soot.jimple.internal.AbstractBinopExpr;
import soot.jimple.internal.JAddExpr;
import soot.jimple.internal.JSubExpr;
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
		
		say(" ?? "+v.getClass().getName());
		say("    "+v.toString());

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
