package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomField;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.trinode.OpMethodCallNode;
import ru.dz.plc.util.PlcException;
import ru.dz.soot.SootMain;

/**
 * Identifier Node. If this Node is executed - it is a variable load.
 */

public class IdentNode extends Node {
	private String ident;
	//private PhantomClass my_class;
	private boolean onIntStack = false;
	private boolean onObjStack = false;

	private Method getterMethod = null;
	//private Node target = null; // if we access field of another object, need code to get it here

	public String getName() { return ident; }

	public IdentNode( /*PhantomClass c,*/ String ident ) {
		super(null);
		this.ident = ident;
		//my_class = c;
	}

	public IdentNode( Node target, String ident ) {
		super(target);
		this.ident = ident;
	}


	public String toString()  {    return "ident "+ident;  }
	
	public void find_out_my_type() throws PlcException 
	{ 
		if( type == null ) 
			throw new PlcException( "ident Node", "no type known", ident ); 
	}
	
	public boolean is_const() { return false; }

	/*@Override
	public boolean args_on_int_stack() {
		return s.istack_vars().get_var(ident) != null;
	}*/

	@Override
	public boolean is_on_int_stack() {
		if( onIntStack )			return true;
		if( onObjStack )			return false;
		throw new RuntimeException("Call to is_on_int_stack() before preprocess");
	}

	public void preprocess_me( ParseState s ) throws PlcException {

		PhantomField f = s.get_class().find_field(ident);
		if( f != null )
		{
			type = f.getType();
			onObjStack = true;
			return;
		}

		PhantomStackVar svar = s.istack_vars().get_var(ident);
		if(svar != null)
		{
			onIntStack = true;
			type = svar.getType();
			if( !type.is_int() )
				throw new PlcException("Not an integer auto var on integer stack");
			return;
		}

		svar = s.stack_vars().get_var(ident);

		if( (svar == null) && (_l != null) )
		{
			PhantomClass tc = _l.getType().get_class();
			//SootMain.say("come load '"+ident+"' by getter from class "+tc);

			// ERROR TODO wrong - must use target class, not our class
			//getterMethod = s.get_class().getGetter(ident);

			getterMethod = tc.getGetter(ident);

			/*
			Node methodName = new MethodNode(getterMethod.getName()).setContext( context );

			callNode = new OpMethodCallNode(out, methodName, args).setContext( context );
			 */

			if( getterMethod == null )
				throw new PlcException( "ident Node", "no such field in class and no getter", ident );

			onObjStack = true;
			type = getterMethod.getType();
			return;
		}	

		if( (svar == null) && (getterMethod == null) )
			throw new PlcException( "ident Node", "no such field in class and no getter", ident );

		onObjStack = true;
		type = svar.getType();
	}

	// load variable
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {

		if( getterMethod != null )
		{
			// if we are here, we have _l - object to call getter in, and it's code is generated
			// should we have call node instead?
			c.emitCall(getterMethod.getOrdinal(),0);
			return;
		}

		//PhantomField f = s.get_class().ft.get(ident);
		PhantomField f = s.get_class().find_field(ident);

		if( f != null )
		{
			//if (type == null || type.is_unknown()) type = f.get_type();
			c.emitLoad(f.getOrdinal());
			return;
		}

		PhantomStackVar svar = s.istack_vars().get_var(ident);
		if( svar != null )
		{
			c.emitIGet(svar.get_abs_stack_pos()); // get stack variable
		}
		else
		{
			svar = s.stack_vars().get_var(ident);
			if( svar == null )
				throw new PlcException( "ident Node", "no field", ident );

			//if (type == null || type.is_unknown()) type = svar.get_type();
			c.emitGet(svar.get_abs_stack_pos()); // get stack variable
		}

	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {

		if( getterMethod != null )
		{
			// if we are here, we have _l - object to call getter in, and it's code is generated
			// should we have call node instead?
			// TODO llvm write codegen
			llc.putln("; TODO call getter "+getterMethod.getName());
			return;
		}


		PhantomField f = llc.getPhantomClass().find_field(ident);
		if( f != null )
		{
			// TODO llvm field load
			//c.emitLoad(f.getOrdinal());
			return;
		}

		PhantomStackVar svar = llc.getIstackVars().get_var(ident);
		if( svar != null )
		{
			//c.emitIGet(svar.get_abs_stack_pos()); // get stack variable
			// TODO llvm
			llc.putln("; %"+llvmTempName+ " = alias "+getType().toLlvmType()+" @"+svar.getName());
		}
		else
		{
			svar = llc.GetOstackVars().get_var(ident);
			if( svar == null )
				throw new PlcException( toString(), "no field", ident );

			//if (type == null || type.is_unknown()) type = svar.get_type();
			//c.emitGet(svar.get_abs_stack_pos()); // get stack variable
			// TODO llvm
			llc.putln("; %"+llvmTempName+ "= alias "+getType().toLlvmType()+" @"+svar.getName());
		}
	}

	
	@Override
	protected void generateMy_C_Code(C_codegen cgen) throws PlcException {

		if( getterMethod != null )
		{
			// if we are here, we have _l - object to call getter in, and it's code is generated
			// should we have call node instead?
			// TODO c write codegen
			cgen.putln("// call getter "+getterMethod.getName());
			//cgen.emitMethodCall(_l, getterMethod.getOrdinal(), null, s)
			return;
}


		PhantomField f = cgen.getPhantomClass().find_field(ident);
		if( f != null )
		{
			// TODO llvm field load
			//c.emitLoad(f.getOrdinal());
			cgen.put(" "+C_codegen.getJitRuntimeFuncPrefix()+"GetField( "+
			C_codegen.getThisVarName()+", "+f.getOrdinal()+") ");
			return;
		}

		PhantomStackVar svar = cgen.getIstackVars().get_var(ident);
		if( svar != null )
		{
			//c.emitIGet(svar.get_abs_stack_pos()); // get stack variable
			// TODO llvm
			//cgen.putln("; %"+llvmTempName+ " = alias "+getType().toLlvmType()+" @"+svar.getName());
			cgen.put(" "+C_codegen.getLocalVarNamePrefix()+svar.getName()+" ");
		}
		else
		{
			svar = cgen.GetOstackVars().get_var(ident);
			if( svar == null )
				throw new PlcException( toString(), "no field", ident );

			//if (type == null || type.is_unknown()) type = svar.get_type();
			//c.emitGet(svar.get_abs_stack_pos()); // get stack variable
			// TODO c
			//cgen.putln("; %"+llvmTempName+ "= alias "+getType().toLlvmType()+" @"+svar.getName());
			cgen.put(" "+C_codegen.getLocalVarNamePrefix()+svar.getName()+" ");
		}
	}
	
	
}
