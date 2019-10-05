package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.binode.OpStaticMethodCallNode;
import ru.dz.plc.compiler.binode.SequenceNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.StatementsNode;
import ru.dz.plc.compiler.node.ThisNode;
import ru.dz.plc.util.PlcException;

/**
 * <p>Represents a method.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */

public class Method
{
	public final static String CONSTRUCTOR_M_NAME = "<init>";

	private boolean		    constructor;
	private int			    ordinal;
	public Node             code;
	private String          name;
	private PhantomType     type;
	private LinkedList<ArgDefinition>   args_def = new LinkedList<ArgDefinition>();

	private boolean         requestDebug = false;

	// Main ones - on object stack
	public PhantomStack     svars = new PhantomStack();
	// Binary ones - on integer stack
	public PhantomStack     isvars = new PhantomStack();


	private Codegen c = new Codegen();



	public Method( String name, PhantomType type, boolean isConstructor )
	{
		//this.name = isConstructor ? "<init>" : name; 
		this.name = isConstructor ? CONSTRUCTOR_M_NAME : name; 
		this.type = type;
		this.ordinal = -1;
		this.code = null;
		this.constructor = isConstructor;
	}

	/** NB! Must be called in correct order! */
	public void addArg( String name, PhantomType type ) throws PlcException
	{
		if( type == null )
			throw new PlcException("Method add_arg", "null type of arg", name);

		args_def.add(new ArgDefinition(name, type));
		svars.addStackVar( new PhantomVariable(name, type ) );
	}


	public Codegen get_cg() { return c; }

	public void generate_code( CodeGeneratorState s ) throws PlcException, IOException 
	{
		if(!preprocessed)
			throw new PlcException("generateC_Code", "not preprocessed");

		if(code == null)
		{
			c.emitRet(); // empty function code
			return;
		}


		// ------------------------------------------
		// traverse tree to allocate automatic vars?
		// ------------------------------------------
		int n_auto_vars = svars.getUsedSlots();
		int n_int_auto_vars = isvars.getUsedSlots();
		// ------------------------------------------

		// ------------------------------------------
		// generate prologue code here
		// ------------------------------------------

		// check number of args
		int n_args = args_def.size();
		if(n_args < Byte.MAX_VALUE)
		{
			c.emit_arg_count_check(n_args);
		}
		else
		{
			String good_args_label = c.getLabel();

			c.emitIConst_32bit(n_args);
			c.emitISubLU();
			c.emitJz(good_args_label);

			// TODO throw not string, but some meaningful class
			// wrong count - throw string
			
			String msg = "arg count: "+name + " in " + s.get_class().getName();
			
			//c.emitString(msg);
			
			int const_id = s.get_class().addStringConst(msg);
			c.emitConstantPool(const_id);
			
			c.emitThrow();

			c.markLabel(good_args_label);
		}

		if(requestDebug) c.emitDebug((byte)0x1,"Enabled debug");

		if( (n_int_auto_vars > 0) || (n_auto_vars > 0) )
			c.emitStackReserve(n_auto_vars, n_int_auto_vars);

		/*{
		// push nulls to reserve stack space for autovars
		// BUG! We can execute vars initialization code here, can we?
		// We can if code does not depend on auto vars itself, or depends only on
		// previous ones.
		// TODO: BUG! autovars in arguments are already on stack, we don't have to reserve space
		for( int i = n_auto_vars; i > 0; i-- )
			c.emitPushNull();

		// TODO introduce instruction to reserve o+i space in one step
		// Reserve integer stack place for int vars
		for( int i = n_int_auto_vars; i > 0; i-- )
			c.emitIConst_0();
		}*/
		// TODO we can provide const pool binary blob with init data and load it with one 
		// big instruction

		// ------------------------------------------


		// ------------------------------------------
		// generate main code by descending the tree
		// ------------------------------------------
		code.generate_code( c, s );
		// ------------------------------------------

		// ------------------------------------------
		// generate epilogue code here
		// ------------------------------------------
		if(requestDebug) c.emitDebug((byte)0x2,"Disabled debug");
		c.emitRet(); // catch the fall-out
		// ------------------------------------------

	}


	// ------------------------------------------
	// LLVM codegen support
	// ------------------------------------------

	private static int llvmTempNum = 0;

	public String getLlvmTempName(String nodeName ) 
	{
		if( null == nodeName )
			return String.format("%%tmp_%d", llvmTempNum++ );
		return String.format("%%tmp_%d_%s", llvmTempNum++, nodeName ); 
	}



	public void generateLlvmCode( CodeGeneratorState s, BufferedWriter llvmFile ) throws PlcException, IOException 
	{
		if(!preprocessed)
			throw new PlcException("generateC_Code", "not preprocessed");

		LlvmCodegen llc = new LlvmCodegen(s.get_class(),this,llvmFile);

		StringBuilder argdef = new StringBuilder();

		boolean firstParm = true;
		for( ArgDefinition a : args_def )
		{
			if(!firstParm)
				argdef.append(", ");

			firstParm= false;

			argdef.append("i64 %"+a.getName());
		}

		String llvmMethodName = name.replaceAll(Method.CONSTRUCTOR_M_NAME, "_\\$_Constructor");

		llc.putln(String.format("define %s @%s(%s) {", LlvmCodegen.getObjectType(), llvmMethodName, argdef )); // function 

		if(code != null)
		{

			// ------------------------------------------
			// traverse tree to allocate automatic vars?
			// ------------------------------------------
			//int n_auto_vars = svars.getUsedSlots();
			//int n_int_auto_vars = isvars.getUsedSlots();
			// ------------------------------------------

			// ------------------------------------------
			// generate prologue code here
			// ------------------------------------------

			/*
		// check number of args
		int n_args = args_def.size();
		String good_args_label = c.getLabel();

		c.emitIConst_32bit(n_args);
		c.emitISubLU();
		c.emitJz(good_args_label);

		// wrong count - throw string
		// BUG! Need less consuming way of reporting this. Maybe by
		// calling class object Method? Or by summoning something?
		c.emitString("arg count: "+name + " in " + s.get_class().getName());
		c.emitThrow();

		c.markLabel(good_args_label);
			 */

			if(requestDebug) llc.emitDebug((byte) 0x1);
			//if(requestDebug) llc.emitDebug((byte)0x1,"Enabled debug");

			// push nulls to reserve stack space for autovars
			// BUG! We can execute vars initialization code here, can we?
			// We can if code does not depend on auto vars itself, or depends only on
			// previous ones.
			// 		for( int i = n_auto_vars; i > 0; i-- ) 			c.emitPushNull();

			// Reserve integer stack place for int vars 		for( int i = n_int_auto_vars; i > 0; i-- )			c.emitIConst_0();

			// ------------------------------------------


			// ------------------------------------------
			// generate main code by descending the tree
			// ------------------------------------------
			code.generateLlvmCode( llc );
			// ------------------------------------------

			// ------------------------------------------
			// generate epilogue code here
			// ------------------------------------------

			//if(requestDebug) c.emitDebug((byte)0x2,"Disabled debug");
			if(requestDebug) llc.emitDebug((byte) 0x2);

		}

		// catch the fall-out
		llc.putln("ret "+LlvmCodegen.getObjectType()+" <{ i8* null, i8* null }> ;"); // empty function code
		llc.putln("}"); // end of function 

		// ------------------------------------------
		// Part of code is generated to the buffer to 
		// be emitted after the method code. Flush it
		// now.
		// ------------------------------------------

		llc.flushPostponedCode();

	}







	private static int cTempNum = 0;

	public String get_C_TempName(String nodeName ) 
	{
		if( null == nodeName )
			return String.format("tmp_%d", cTempNum++ );
		return String.format("tmp_%d_%s", cTempNum++, nodeName ); 
	}





	public void generateC_Code(CodeGeneratorState s, BufferedWriter c_File) throws PlcException 
	{
		if(!preprocessed)
			throw new PlcException("generateC_Code", "not preprocessed");

		// TODO not finished, sketch
		C_codegen cgen = new C_codegen(s.get_class(),this,c_File);

		StringBuilder argdef = new StringBuilder();

		argdef.append(" jit_vm_state_t "+
				C_codegen.get_vm_state_var_name()
		+", jit_object_t "+C_codegen.getThisVarName()+" ");

		//boolean firstParm = true;
		for( ArgDefinition a : args_def )
		{
			//if(!firstParm)
			argdef.append(", ");

			//firstParm= false;

			argdef.append("jit_object_t "+C_codegen.getLocalVarNamePrefix()+a.getName());
		}

		//String C_MethodName = name.replaceAll(Method.CONSTRUCTOR_M_NAME, "_Phantom_Constructor");
		PhantomClass my_class = s.get_class();
		String C_MethodName = cgen.getMethodName(my_class, ordinal);

		cgen.putln(String.format("%s %s(%s) {", C_codegen.getObjectType(), C_MethodName, argdef )); // function 

		cgen.emitSnapShotTrigger(); // on func enter check for snapshot request

		if(code != null)
		{

			// ------------------------------------------
			// traverse tree to allocate automatic vars?
			// ------------------------------------------
			//int n_auto_vars = svars.getUsedSlots();
			//int n_int_auto_vars = isvars.getUsedSlots();
			// ------------------------------------------

			// ------------------------------------------
			// generate prologue code here
			// ------------------------------------------

			/*
		// check number of args
		int n_args = args_def.size();
		String good_args_label = c.getLabel();

		c.emitIConst_32bit(n_args);
		c.emitISubLU();
		c.emitJz(good_args_label);

		// wrong count - throw string
		// BUG! Need less consuming way of reporting this. Maybe by
		// calling class object Method? Or by summoning something?
		c.emitString("arg count: "+name + " in " + s.get_class().getName());
		c.emitThrow();

		c.markLabel(good_args_label);
			 */

			if(requestDebug) cgen.emitDebug((byte) 0x1);
			//if(requestDebug) llc.emitDebug((byte)0x1,"Enabled debug");

			// push nulls to reserve stack space for autovars
			// BUG! We can execute vars initialization code here, can we?
			// We can if code does not depend on auto vars itself, or depends only on
			// previous ones.
			// 		for( int i = n_auto_vars; i > 0; i-- ) 			c.emitPushNull();

			// Reserve integer stack place for int vars 		for( int i = n_int_auto_vars; i > 0; i-- )			c.emitIConst_0();

			// ------------------------------------------


			// ------------------------------------------
			// generate main code by descending the tree
			// ------------------------------------------
			code.generate_C_code( cgen, s );
			// ------------------------------------------

			// ------------------------------------------
			// generate epilogue code here
			// ------------------------------------------

			//if(requestDebug) c.emitDebug((byte)0x2,"Disabled debug");
			if(requestDebug) cgen.emitDebug((byte) 0x2);

		}

		// catch the fall-out TODO must return null phantom ptr const
		cgen.putln(";"); // finish last statement
		cgen.putln(""); 
		cgen.putln("return 0;"); // empty function code
		cgen.putln("}"); // end of function 

		// ------------------------------------------
		// Part of code is generated to the buffer to 
		// be emitted after the method code. Flush it
		// now.
		// ------------------------------------------

		cgen.flushPostponedCode();


	}



	// ------------------------------------------
	// Getters/setters
	// ------------------------------------------


	public int getOrdinal() { return ordinal; }
	public void setOrdinal(int ord) { ordinal = ord; }

	public void setDebugMethod(boolean requestDebug) { this.requestDebug = requestDebug; }
	public boolean getDebugMethod() { return requestDebug; }

	public String getName() { return name; }
	public MethodSignature getSignature() { return new MethodSignature( getName(), getArgIterator() ); }

	public PhantomType getType() { return type; }
	public void setType(PhantomType phantomType) { type = phantomType; }

	public int getArgCount() { return args_def.size(); }
	public Iterator<ArgDefinition> getArgIterator() { return args_def.iterator(); }

	public boolean isConstructor() { return constructor; 	}

	// ------------------------------------------
	// Dump/print
	// ------------------------------------------


	public String toString()
	{
		StringBuffer sb = new StringBuffer();

		sb.append( type.toString() );
		sb.append( " " );
		sb.append( name );
		sb.append( "( " );
		boolean first = true;
		for( Iterator<ArgDefinition> i = args_def.iterator(); i.hasNext(); )
		{
			if( !first ) sb.append(", ");
			first = false;
			sb.append(i.next().toString());
		}
		sb.append( " )" );

		return sb.toString();
	}

	public String dumpArgs() 
	{
		StringBuilder s = new StringBuilder();
		Iterator<ArgDefinition> i = getArgIterator();

		while (i.hasNext()) {
			ArgDefinition ad = i.next();

			s.append(ad.toString());
			s.append(" ");
		}

		return s.toString();
	}

	public void propagateVoid() {
		if( code != null )
			code.propagateVoidParents();		
	}
	

	private boolean preprocessed = false;
	public void preprocess( PhantomClass myClass ) throws PlcException 
	{
		// TODO If I am c'tor make sure we have call to parent class c'tor or add one

		if(isConstructor())
			preprocessParentConstructor( myClass );

		preprocessed = true;
	}

	private void preprocessParentConstructor(PhantomClass myClass) throws PlcException 
	{
		// TODO will fail on empty constructors in intermediate classed - code == null
		//System.out.println("preprocessParentConstructor for "+myClass.getName());
		
		// don't call constructor for parent of all classes
		if( myClass.getParent().equals(".internal.object") )
			return;

		// And for itself
		if( myClass.getName().equals(".internal.object") )
			return;

		if( checkConstructorCall( code, myClass ) == SearchResult.Ok )
			return;

		// No base class c'tor call found
		// Create call to default c'tor
		//System.out.println("create parent constructor call "+myClass.getName());

		String parentClassName = myClass.getParent();

		if( (parentClassName == null) || (parentClassName.equals(".internal.object")) )
			return;

		PhantomClass parentClass = ClassMap.get_map().get(parentClassName, true, null);

		if( parentClass == null )
			throw new PlcException("preprocessParentConstructor", "parent class not found: "+parentClassName, myClass.getName());

		Method defCtor = parentClass.getDefaultConstructor();

		if( defCtor == null )
			throw new PlcException("preprocessParentConstructor", "parent class ("+parentClassName+") has no default constructor", myClass.getName());

		Node ctorCall = new OpStaticMethodCallNode(
				new ThisNode(myClass),
				defCtor.getOrdinal(),
				null, // no args
				// myClass // No!!
				parentClass
				);
		ctorCall.presetType(PhantomType.getVoid());
		Node newRoot = new SequenceNode(ctorCall, code);

		code = newRoot;
	}

	static private enum SearchResult {
		Continue, Ok, Fail
	};

	private SearchResult checkConstructorCall(Node n, final PhantomClass myClass) throws PlcException {
		// Find first meaningful node, check if it is static call of parent's c'tor

		if( n == null ) return SearchResult.Continue;

		if (n instanceof SequenceNode) {
			SequenceNode sn = (SequenceNode) n;

			SearchResult r;

			r = checkConstructorCall( sn.getLeft(), myClass );
			if( r != SearchResult.Continue ) return r;

			return checkConstructorCall( sn.getRight(), myClass );				
		}

		if (n instanceof StatementsNode) 
		{
			StatementsNode sn = (StatementsNode) n;

			List<Node> nodes = sn.getNodeList();

			for( Node t : nodes )
			{			
				SearchResult r = checkConstructorCall( t, myClass );
				if( r != SearchResult.Continue ) return r;
			}

			return SearchResult.Continue;
		}

		// not sequence - now check it

		if (n instanceof OpStaticMethodCallNode) {
			OpStaticMethodCallNode mc = (OpStaticMethodCallNode) n;

			// got static call - supposedly it calls base class constructor, check it

			Node new_this = mc.getLeft(); // This for static call

			if (!(new_this instanceof ThisNode)) {			
				throw new PlcException("checkConstructorCall", "first call is not for 'this', so it is not a base class constructor");
			}

			PhantomClass cc = mc.getCallClass();

			if( !cc.getName().equals(myClass.getParent()) )
				throw new PlcException("checkConstructorCall", "our parent is "+myClass.getParent()+", but c'tor call is for "+cc.getName());
			//throw new PlcException("checkConstructorCall", "our base is "+myClass.getName()+", but c'tor call is for "+cc.getName());

			int ord = mc.getOrdinal();

			Method ctor = cc.getMethod(ord);
			if( ctor == null )
				throw new PlcException("checkConstructorCall", "base c'tor is null", cc.getName() );

			if( !ctor.isConstructor() )
				throw new PlcException("checkConstructorCall", "base c'tor is not really c'tor" );

			if( !ctor.getName().equals(Method.CONSTRUCTOR_M_NAME) )
				throw new PlcException("checkConstructorCall", "base c'tor method name is not "+Method.CONSTRUCTOR_M_NAME );

			// Seems we checked it all

			return SearchResult.Ok;
		}

		// First found code is not base c'tor call - fail

		return SearchResult.Fail;
	}

}
















