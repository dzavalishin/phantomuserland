package ru.dz.soot;

import java.util.List;

import soot.Body;
import soot.PatchingChain;
import soot.SootMethod;
import soot.SootMethodRef;
import soot.Unit;
import soot.UnitBox;
import soot.Value;
import soot.ValueBox;
import soot.jimple.InvokeExpr;
import soot.jimple.internal.AbstractStmt;
import soot.jimple.internal.JAssignStmt;
import soot.jimple.internal.JGotoStmt;
import soot.jimple.internal.JIdentityStmt;
import soot.jimple.internal.JIfStmt;
import soot.jimple.internal.JInvokeStmt;
import soot.jimple.internal.JReturnStmt;
import soot.jimple.internal.JReturnVoidStmt;
import soot.tagkit.Tag;

public class SootMethodTranslator {

	private SootLabelMap lmap = new SootLabelMap();
	private SootMethod m;

	public SootMethodTranslator(SootMethod m) {
		this.m = m;
	}

	
	
	
	public void process() {
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

	private void doUnit(Unit u) {
		String dump = u.toString(); say("\n  "+dump);
		//say("  "+u.getClass().getName());

		List<UnitBox> boxes = u.getBoxesPointingToThis();
		for( UnitBox ub : boxes )
		{
			if( ub.isBranchTarget() )
			{
				String labelFor = lmap.getLabelFor(ub);
				say(""+labelFor+":");
			}
		}
		
		
		if( u instanceof AbstractStmt )
		{
			 AbstractStmt as = (AbstractStmt)u;
			 doStatement(as);
		}
		
		List<Tag> tags = u.getTags();
		for( Tag t : tags )
		{
			String ts = t.toString();
			say("     "+ts);
		}

		/*
		List<UnitBox> boxes = u.getUnitBoxes();
		for( UnitBox b : boxes )
		{
			say("     "+b.toString());
		}
		*/
	}

	private PhantomCodeWrapper doStatement(AbstractStmt as) {
	
		if( as instanceof JIdentityStmt )
			return doIdentity( (JIdentityStmt)as );
	
		if( as instanceof JReturnVoidStmt )
			return doRetVoid( (JReturnVoidStmt)as );
		
		if( as instanceof JReturnStmt )
			return doReturn( (JReturnStmt)as );
		
		if( as instanceof JInvokeStmt )
			return doInvoke( (JInvokeStmt)as );
		
		if( as instanceof JGotoStmt )
			return doGoto( (JGotoStmt)as );
		
		if( as instanceof JAssignStmt )
			return doAssign( (JAssignStmt)as );
		
		if( as instanceof JIfStmt )
			return doIf( (JIfStmt)as );
		
		say(" ?? "+as.getClass().getName());
		say("    "+as.toString());

		return PhantomCodeWrapper.getNullNode();
	}

	
	
	
	
	


	private PhantomCodeWrapper doGoto(JGotoStmt as) {
		UnitBox targetBox = as.getTargetBox();
		String label = lmap.getLabelFor(targetBox);
		
		say("      go to "+targetBox.toString()+" ("+label+")");
		//Unit target = as.getTarget();
		//say("      if "+target.toString());

		return PhantomCodeWrapper.getJumpNode(label);
	}
	
	
	private PhantomCodeWrapper doIf(JIfStmt as) {
		UnitBox targetBox = as.getTargetBox();
		String label = lmap.getLabelFor(targetBox);

		say("      go to "+targetBox.toString()+" ("+label+")");
		Unit target = as.getTarget();
		say("      if "+target.toString());
		
		return null;
	}




	private PhantomCodeWrapper doAssign(JAssignStmt as) {
		ValueBox leftBox = as.leftBox;
		ValueBox rightBox = as.rightBox;
		
		String ls = leftBox.toString();
		String rs = rightBox.toString();
		
		say("      Assign '"+ls+"' = '"+rs+"'");

		return null;
	}


	private PhantomCodeWrapper doInvoke(JInvokeStmt as) {
		InvokeExpr expr = as.getInvokeExpr();
		say("      Invoke "+expr.toString());
		SootMethodRef methodRef = expr.getMethodRef();
		say("      ."+methodRef.name());
		return null;
	}

	
	
	private PhantomCodeWrapper doRetVoid(JReturnVoidStmt as) {
		say("      Return void ");
		return PhantomCodeWrapper.getReturnNode();
	}

	private PhantomCodeWrapper doReturn(JReturnStmt as) {
		Value op = as.getOp();
		PhantomCodeWrapper v = PhantomCodeWrapper.getExpression( op );
		say("      Return "+op.toString());
		return PhantomCodeWrapper.getReturnValueNode(v);
	}



	
	
	private PhantomCodeWrapper doIdentity(JIdentityStmt as) {
		String ls = as.leftBox.toString();
		String rs = as.rightBox.toString();
		
		say("      Identity '"+ls+"' <- '"+rs+"'");
		
		return null;
	}


	private void say(String string) {
		System.err.println(string);
	}
	
	
}
