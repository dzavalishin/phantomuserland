package ru.dz.soot;

import java.util.LinkedList;
import java.util.List;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.PhantomVariable;
import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.NullNode;
import ru.dz.plc.compiler.node.ThisNode;
import ru.dz.plc.util.PlcException;
import soot.Body;
import soot.PatchingChain;
import soot.SootMethod;
import soot.SootMethodRef;
import soot.Type;
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
import soot.util.Switch;

public class SootMethodTranslator {

	private SootLabelMap lmap = new SootLabelMap();
	private SootMethod m;

	private List<PhantomCodeWrapper> statements = new LinkedList<PhantomCodeWrapper>();
	private String mName;
	private Method phantomMethod;
	private PhantomClass pc;
	private ParseState s;
	
	public SootMethodTranslator(SootMethod m, PhantomClass pc) throws PlcException {
		this.m = m;
		this.pc = pc;
		mName = m.getName();

		s = new ParseState(pc);
		
		Type returnType = m.getReturnType();
		
		PhantomType type = SootExpressionTranslator.convertType(returnType);
		phantomMethod = new Method(mName, type); 
		pc.addMethod(phantomMethod);
	
		for( int i = 0; i < m.getParameterCount(); i++ )
		{
			Type parameterType = m.getParameterType(i);
			PhantomType pptype = SootExpressionTranslator.convertType(parameterType);
			String parName = String.format("parm%d", i);
			phantomMethod.addArg(parName, pptype);
		}

	}

	
	
	
	public void process() throws PlcException {
		say("Method "+mName);

		m.retrieveActiveBody();
		
		Body body = m.getActiveBody();
		
		PatchingChain<Unit> units = body.getUnits();
		
		for( Unit u : units )
		{
			doUnit(u);
		}
		
	}

	
	public void codegen()
	{
		// TODO write codegen
	}
	
	//static private JimpleToBafContext context = new JimpleToBafContext(0);

	private void doUnit(Unit u) throws PlcException {
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
		
		phantomMethod.code = new SequenceNode(null, null);
		
		if( u instanceof AbstractStmt )
		{
			 AbstractStmt as = (AbstractStmt)u;
			 PhantomCodeWrapper statement = doStatement(as);
			 if( statement != null)
			 {
			 statements.add(statement);
			 phantomMethod.code  =
					 new SequenceNode(
							 phantomMethod.code, 
							 statement.getNode());
			 }
			 else
				 SootMain.say("null statement");
		}
		
		s.set_method(phantomMethod);
		phantomMethod.code.preprocess(s);
		
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

	private PhantomCodeWrapper doStatement(AbstractStmt as) throws PlcException {
	
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
		
		
		SootMain.say("s ?? "+as.getClass().getName()+" ("+as.toString()+")");

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




	private PhantomCodeWrapper doAssign(JAssignStmt as) throws PlcException {
		ValueBox leftBox = as.leftBox;
		ValueBox rightBox = as.rightBox;
		
		String ls = leftBox.toString();
		String rs = rightBox.toString();
		
		say("      Assign '"+ls+"' = '"+rs+"'");

		PhantomCodeWrapper expression = PhantomCodeWrapper.getExpression( rightBox.getValue(), phantomMethod );
		
		return PhantomCodeWrapper.getAssign( leftBox.getValue(), expression, phantomMethod );
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

	private PhantomCodeWrapper doReturn(JReturnStmt as) throws PlcException {
		Value op = as.getOp();
		PhantomCodeWrapper v = PhantomCodeWrapper.getExpression( op, phantomMethod );
		say("      Return "+op.toString());
		return PhantomCodeWrapper.getReturnValueNode(v);
	}



	
	/**
	 * Identity - make ident to refer to some stuff, like arg or this.
	 * @param as statement
	 * @return generated code tree
	 * @throws PlcException
	 */
	private PhantomCodeWrapper doIdentity(JIdentityStmt as) throws PlcException {
		Value lv = as.leftBox.getValue();
		Value rv = as.rightBox.getValue();

		// Arg: we just create var def and stick it to fixed object stack position, which is
		// a correct way to do it.
		if(
				(lv.getClass() == soot.jimple.internal.JimpleLocal.class) &&
				(rv.getClass() == soot.jimple.ParameterRef.class)
				)
		{
			soot.jimple.internal.JimpleLocal jLocal = (soot.jimple.internal.JimpleLocal)lv;
			soot.jimple.ParameterRef parmRef = (soot.jimple.ParameterRef)rv;
			
			String localName = jLocal.getName();
			Type type = jLocal.getType();

			// TODO make sure that parameter position is correct, or else we need here (numPar - parameterPosition - 1)
			int parameterPosition = parmRef.getIndex();
			
			phantomMethod.svars.setParameter(parameterPosition, localName, SootExpressionTranslator.convertType(type));
			return new PhantomCodeWrapper(new NullNode());
		}

		// This: we really should set up a virtual variable that codegens 'summon this',
		// but for simplicity we generate code to load this to some var
		if(
				(lv.getClass() == soot.jimple.internal.JimpleLocal.class) &&
				(rv.getClass() == soot.jimple.ThisRef.class)
				)
		{
			// Create stack var ans load it with 'this'
			
			soot.jimple.internal.JimpleLocal jLocal = (soot.jimple.internal.JimpleLocal)lv;
			//soot.jimple.ThisRef thisRef = (soot.jimple.ThisRef)rv;
			
			//thisRef.
			
			String localName = jLocal.getName();
			Type type = jLocal.getType();

			PhantomVariable v = new PhantomVariable(localName, SootExpressionTranslator.convertType(type));			
			phantomMethod.svars.add_stack_var(v);
			
			OpAssignNode node = new OpAssignNode(new IdentNode(localName) , new ThisNode(pc));
			
			return new PhantomCodeWrapper(node);
		}
		
		
		String ls = as.leftBox.toString();
		String rs = as.rightBox.toString();
		
		say(" ??   Identity '"+ls+"' <- '"+rs+"'");
		
		say("      Identity '"+lv+"' <- '"+rv+"'");
		say("      Identity '"+lv.getClass()+"' <- '"+rv.getClass()+"'");
		
		
		return null;
	}


	private void say(String string) {
		System.err.println(string);
	}
	
	
}
