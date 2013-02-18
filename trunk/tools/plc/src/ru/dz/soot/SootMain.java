/**
 * 
 */
package ru.dz.soot;

import java.util.LinkedList;
import java.util.List;
import java.util.logging.Logger;

import soot.Body;
import soot.PatchingChain;
import soot.Scene;
import soot.SootClass;
import soot.SootMethod;
import soot.SootMethodRef;
import soot.Unit;
import soot.UnitBox;
import soot.ValueBox;
import soot.jimple.ArrayRef;
import soot.jimple.FieldRef;
import soot.jimple.InvokeExpr;
import soot.jimple.JimpleToBafContext;
import soot.jimple.internal.AbstractStmt;
import soot.jimple.internal.JAssignStmt;
import soot.jimple.internal.JGotoStmt;
import soot.jimple.internal.JIdentityStmt;
import soot.jimple.internal.JInvokeStmt;
import soot.jimple.internal.JReturnVoidStmt;
import soot.tagkit.Tag;

/**
 * @author dz
 *
 */
public class SootMain {
	Logger log = Logger.getLogger(SootMain.class.getName());

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub

		String cp = "bin;../bin;lib/rt_6.jar";
		
		//System.setProperty("soot.class.path", cp);
		
		Scene.v().setSootClassPath(cp);
		
		SootClass c = Scene.v().loadClassAndSupport("test.SootTestClass");
		if( c.isPhantom() )
		{
			die("Not loaded");
		}

		List<SootMethod> mlist = c.getMethods();
		
		for( SootMethod m : mlist )
		{
			doMethod(m);
		}
		
	}

	private static void doMethod(SootMethod m) {
		String mName = m.getName();
		say("Method "+mName);

		m.retrieveActiveBody();
		
		Body body = m.getActiveBody();
		
		PatchingChain<Unit> units = body.getUnits();
		
		for( Unit u : units )
		{
			doUnit(u);
		}
	}

	//static private JimpleToBafContext context = new JimpleToBafContext(0);

	private static void doUnit(Unit u) {
		String dump = u.toString();
		say("  "+dump);
		//say("  "+u.getClass().getName());
		
		//JReturnVoidStmt r;
		
		if( u instanceof AbstractStmt )
		{
			 AbstractStmt as = (AbstractStmt)u;
			 doStatement(as);
			 /*
			 if( as.containsInvokeExpr() )
			 {
				 InvokeExpr expr = as.getInvokeExpr();
				 say( "    = "+expr.toString() );
			 }
			 
			 if( as.containsFieldRef() )
			 {
				 FieldRef ref = as.getFieldRef();
				 say( "    < "+ref.toString() );
			 }
			 
			 if( as.containsArrayRef() )
			 {
				 ArrayRef ref = as.getArrayRef();
				 say( "    [] "+ref.toString() );
			 }
			 
			 / *
			List<Unit> out = new LinkedList<Unit>();
			as.convertToBaf(context, out);

			for( Unit uu : out )
			{
				String dump1 = uu.toString();
				say("  - "+dump1);
				say("  - "+uu.getClass().getName());
			}
			 */
		}
		
		List<Tag> tags = u.getTags();
		for( Tag t : tags )
		{
			String ts = t.toString();
			say("     "+ts);
		}

		
		List<UnitBox> boxes = u.getUnitBoxes();
		for( UnitBox b : boxes )
		{
			say("     "+b.toString());
		}
		
	}

	private static PhantomCodeWrapper doStatement(AbstractStmt as) {
	
		if( as instanceof JIdentityStmt )
			return doIdentity( (JIdentityStmt)as );
	
		if( as instanceof JReturnVoidStmt )
			return doRetVoid( (JReturnVoidStmt)as );
		
		if( as instanceof JInvokeStmt )
			return doInvoke( (JInvokeStmt)as );
		
		if( as instanceof JGotoStmt )
			return doGoto( (JGotoStmt)as );
		
		if( as instanceof JAssignStmt )
			return doAssign( (JAssignStmt)as );
		
		say(" ?? "+as.getClass().getName());

		return PhantomCodeWrapper.getNullNode();
	}

	
	
	
	
	
	
	
	private static PhantomCodeWrapper doAssign(JAssignStmt as) {
		ValueBox leftBox = as.leftBox;
		ValueBox rightBox = as.rightBox;
		
		String ls = leftBox.toString();
		String rs = rightBox.toString();
		
		say("      Assign '"+ls+"' = '"+rs+"'");

		return null;
	}

	private static PhantomCodeWrapper doGoto(JGotoStmt as) {
		UnitBox targetBox = as.getTargetBox();
		say("      go to "+targetBox.toString());
		Unit target = as.getTarget();
		say("      if "+target.toString());

		return null;
	}

	private static PhantomCodeWrapper doInvoke(JInvokeStmt as) {
		InvokeExpr expr = as.getInvokeExpr();
		say("      Invoke "+expr.toString());
		SootMethodRef methodRef = expr.getMethodRef();
		say("      ."+methodRef.name());
		return null;
	}

	private static PhantomCodeWrapper doRetVoid(JReturnVoidStmt as) {
		
		return PhantomCodeWrapper.getReturnNode();
	}

	private static PhantomCodeWrapper doIdentity(JIdentityStmt as) {
		String ls = as.leftBox.toString();
		String rs = as.rightBox.toString();
		
		say("      Identity '"+ls+"' <- '"+rs+"'");
		
		return null;
	}

	private static void die(String string) {
		System.err.println(string);
		System.exit(33);
	}

	private static void say(String string) {
		System.err.println(string);
	}

}
