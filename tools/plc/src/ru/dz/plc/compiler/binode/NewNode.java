package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.MethodSignature;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.llvm.LlvmStringConstant;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;



/**
 * <p>New (create object) node.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2017 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */


public class NewNode extends Node 
{
	private PhantomType static_type = null;
	private Node args;
	
	private boolean dynamic; // use dynamic type

	public NewNode(PhantomType static_type, Node dynamic_type, Node args)
	{
		super(dynamic_type);
		this.static_type = static_type;
		this.args = args;
		
		dynamic = dynamic_type != null;
	}

	@Override
	public
	PhantomType find_out_my_type() throws PlcException {

		if(static_type != null)
			return static_type;

		return super.find_out_my_type(); // TODO right?
	}

	public String toString()  
	{    
		if(dynamic)
		{
			return "new (dynamic type)" + ((static_type != null) ? static_type.toString() : "");
		}
		else
			return "new " + (dynamic ? "(dynamic type)" : static_type.toString());  
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		generate_my_code(c,s);
	}

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {

		if( static_type == null && _l == null)
			throw new PlcException( "new Node", "no type known" );

		//c.emitDebug((byte)0, "before new");
		
		int n_param = countParameters();

		if( dynamic )
		{
			_l.generate_code(c,s);
			if(_l.is_on_int_stack())
				throw new PlcException( "new Node", "type expression can't be int" );

		}
		else
		{
			if(static_type.get_class().isInternal())
			{
				if( n_param > 0 )
					throw new PlcException( "new Node", "attempt to construct internal class object with parameters" );
				//return;
			}
			
			static_type.emit_get_class_object(c,s);
		}
		
		//dynamicClass = true; // disable ctor call temp FIXME
		
/* can't drop it! will regenerate class ptr below
		if( !dynamicClass ) // no c'tor call for dyn class - TODO FIXME
			c.emitOsDup(); // copy of pointer to class - MUST POP BELOW, pull copies me, not moves
*/		
		//c.emitDebug((byte)0, "class for new");

		c.emitNew();

		//c.emitOsDup(); // copy of new object

		// prepare to call constructor

		//c.emitDebug((byte)0, "new_this");



		if( dynamic )
		{
			print_warning("No constructor call for dynamic new!");
		}
		else if(!static_type.get_class().isInternal()) // No c'tor for internal obj
		{

			// args - TODO are we sure they're on obj stack?
			if( args != null ) {

				if( dynamic )
					throw new PlcException(context.get_context(), "No constructor with args for dynamic class new" );

				args.generate_code(c, s);
				//move_between_stacks(c, _l.is_on_int_stack());
			}
			
			c.emit_pull(0+n_param); // get copy of object ptr
			//c.emit_pull(2+n_param); // get copy of class ptr - NO!
			if( !dynamic ) // no c'tor call for dyn class - TODO FIXME
				static_type.emit_get_class_object(c,s);
			else
				throw new PlcException(context.get_context(), "No constructor for dynamic class new" );
			// TODO args first! - done? test!
			
			

			//if( n_param > 0 )				throw new PlcException(context.get_context(), "can generate just argless c'tors, sorry" );


			int method_ordinal = findConstructorOrdinal(n_param);


			c.emitStaticCall(method_ordinal, n_param);

			c.emitOsDrop(); // c'tor is void
			
			//c.emitOsDrop(); // Dupped class ref
		}

		//c.emitDebug((byte)0, "after new");

	}

	private int countParameters() {
		int n_param = 0;

		// bug - wrong count of args?
		for( Node i = args; i != null; i = ((BiNode)i).getRight() )      n_param++;
		return n_param;
	}


	/*
	private int findConstructorOrdinal(int n_param) throws PlcException 
	{
		int method_ordinal = 0; // .internal.object constructor

		PhantomClass pclass = static_type.get_class();
		if( pclass != null )
		{
			Method cm = pclass.findMethod(new MethodSignature(Method.CONSTRUCTOR_M_NAME, args));
			if( cm == null )
			{					
				if( n_param > 0 )
					throw new PlcException(context.get_position(), "No constructor found" );

				//print_warning("No constructor found, will call Object constructor");
			}
			else
				method_ordinal = cm.getOrdinal();
		}
		else
			print_warning("Can't call c'tor for "+static_type);
		
		return method_ordinal;
	}*/

	
	private int findConstructorOrdinal(int n_param) throws PlcException 
	{
		PhantomClass pclass;
		if( static_type.is_container() )
		{
			if( !static_type.isSpecificContainerClass() )
				return 0; // Default container constructor ordinal
			
			String ccn = static_type.get_main_class_name();
			throw new PlcException("findConstructorOrdinal", "Specific container class support is not implemented", ccn);
		}
		pclass = static_type.get_class();
		
		if( pclass == null )
			throw new PlcException("NewNode","Can't call c'tor for "+static_type);
			
		Method cm = pclass.findMethod(new MethodSignature(Method.CONSTRUCTOR_M_NAME, args));
		if( cm == null )
		{					
			//if( n_param > 0 )
			throw new PlcException(context.get_position(), "No constructor found" );
			
			//print_warning("No constructor found, will call Object constructor");
		}
		
		return cm.getOrdinal();
	}
	
	
	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {

		if( static_type == null && _l == null)
			throw new PlcException( "new Node", "no type known" );


		if( dynamic )
		{
			// TODO Auto-generated method stub
			llc.put("; new dynamic: ");

			//_l.generateLlvmCode(llc);
			if(_l.is_on_int_stack())
				throw new PlcException( "new Node", "type expression can't be int" );

			llc.putln(llvmTempName+" = call "+LlvmCodegen.getObjectType()+" @PhantomVm_new_dynamic( "+LlvmCodegen.getObjectType()+" "+_l.getLlvmTempName()+" ); ");

			llc.put("; end new ");
		}
		else
		{
			LlvmStringConstant ls = new LlvmStringConstant(llc, static_type.toString());
			// TODO Auto-generated method stub
			//llc.putln("; new "+static_type.toString());
			llc.postponeCode(ls.getDef());
			llc.putln(ls.getCast());
			llc.putln(llvmTempName+" = call "+LlvmCodegen.getObjectType()+" @PhantomVm_new_static( i8* "+ls.getReference()+"); ");
		}
	}

	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException {
		
		if( static_type == null && _l == null)
			throw new PlcException( "new Node", "no type known" );

		cgen.putln("({ ");
		cgen.put( C_codegen.getObjectType()+" tmp_class = ");


		if( dynamic )
		{
			// TODO Auto-generated method stub
			cgen.put("/* new dynamic: */");

			if(_l.is_on_int_stack())
				throw new PlcException( "new Node", "type expression can't be int" );
			_l.generate_C_code(cgen, s);
		}
		else
		{
			cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"SummonClass( \""+static_type.toString()+"\" )");
		}
		
		cgen.putln(";");

		cgen.put(C_codegen.getObjectType()+" tmp_new = ");
		cgen.putln(C_codegen.getJitRuntimeFuncPrefix()+"New( tmp_class );");

		// TODO call constructor
		cgen.putln("// TODO call constructor");
		
		cgen.putln("// end");
		cgen.putln(" tmp_new; })");

	}
	
	
	@Override
	public void preprocess_me(ParseState s) throws PlcException {
		// None
	}
}
