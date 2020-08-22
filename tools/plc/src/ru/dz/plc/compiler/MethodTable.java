package ru.dz.plc.compiler;

import java.util.*;

import com.oracle.xmlns.internal.webservices.jaxws_databinding.JavaMethod;

import java.io.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.PlcMain;
import ru.dz.plc.compiler.binode.OpStaticMethodCallNode;
import ru.dz.plc.compiler.node.EmptyNode;
import ru.dz.plc.compiler.node.MethodNode;
import ru.dz.plc.compiler.node.ThisNode;
import ru.dz.plc.util.*;

/**
 * <p>Class methods table.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2019 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */

public class MethodTable implements IMethodTable 
{
	//private Map<String, Method> table = new HashMap<String, Method>();
	private Map<MethodSignature, Method> mstable = new HashMap<MethodSignature, Method>();

	protected ordinals_generator ordinals = new ordinals_generator();

	//public MethodTable() { table = new HashMap<String, Method>(); }

	//public Iterator<Method> iterator() { return table.values().iterator(); }
	public Iterator<Method> iterator() { return mstable.values().iterator(); }

	private boolean have_ord( int ord )
	{
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			if( m.getOrdinal() == ord ) return true;
		}
		return false;
	}

	// TODO Method.equals?
	private boolean mine( Method t )
	{
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
			if( t == i.next() ) return true;
		return false;
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#set_ordinal(ru.dz.plc.compiler.Method, int)
	 */
	@Override
	public void set_ordinal( Method m, int ord ) throws PlcException 
	{
		if( !mine( m ) ) 
			throw new PlcException("set_ordinal","not my Method");

		if( ord != -1 && have_ord( ord ) ) 
			throw new PlcException("set_ordinal","duplicate");

		//if( m.isConstructor() && (m.getArgCount() == 0) && (ord != 0) )
		//	throw new PlcException("set_ordinal","argless constructor must have ord 0");

		m.setOrdinal( ord );
	}
	/*
	@Override @Deprecated
	public Method add( String name, PhantomType type, boolean constructor ) throws PlcException
	{
		//assert(name != null);
		if(name == null)
			throw new PlcException("Null method name", name);
		Method m = new Method( name, type, constructor );
		table.put(name, m);
		return m;
	}
	 */
	@Override
	public Method add( Method m ) throws PlcException
	{
		if(m.getName() == null)
			throw new PlcException("Null method name", m.toString());
		MethodSignature signature = m.getSignature();

		//System.err.println("Add method "+m+" sig "+signature);

		mstable.put(signature, m);
		//table.put(m.getName(), m);
		return m;
	}
	/*
	@Override @Deprecated
	public boolean have( String name ) { return table.containsKey(name); }
	@Override @Deprecated
	public Method get( String name ) { return (Method)table.get(name); }
	 */
	/**
	 * Get method by signature.
	 * 
	 * TODO incomplete, must check by name first and try signatures using possible 
	 * superclasses, see MethodSignature.canBeCalledFor()
	 * 
	 * @param signature Caller's set of parameter types and method name.
	 * @return Metod found or null.
	 */
	public Method get(MethodSignature signature) 
	{ 
		//System.err.println("Get method "+signature);

		Method m =  mstable.get(signature);
		if( m != null ) return m;

		return checkPossibleConversions(signature);
	}


	/** get method by ordinal */
	public Method get(int ordinal) {
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			if( m.getOrdinal() == ordinal ) 
				return m;
		}
		return null;
	}


	private Method checkPossibleConversions(MethodSignature signature) {
		List<Method> all = getAllForName(signature.getName());

		for( Method m: all)
		{
			if( m.getSignature().canBeCalledFor(signature) )
				return m;
		}

		return null;
	}

	private List<Method> getAllForName(String name) {
		LinkedList<Method> out = new LinkedList<Method>();

		for( Method m : mstable.values() )
			if(m.getName().equals(name))
				out.add(m);

		return out;
	}

	@Override
	public void print(PrintStream ps) throws PlcException
	{
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			ps.println(
					(m.isConstructor() ? "  C'tor  " : "  Method ")
					+m.toString()+":");
			if( m.code != null )      m.code.print(ps,2,2);
			else ps.println("  -- No code!");
			ps.println("");
		}
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#set_ordinals()
	 */
	@Override
	public void set_ordinals() throws PlcException
	{
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			setupMethodOrdinal(m);
		}
	}

	public void propagateVoid() 
	{
		for( Method m : mstable.values() )
			m.propagateVoid();
	}
	
	
	private void setupMethodOrdinal(Method m) throws PlcException 
	{
		if( m.getOrdinal() >= 0 )
			return;

		//System.out.println("Set ordinal for "+m.getSignature());
		
		int ord;
		while( true )
		{
			ord = ordinals.getNext();

			if( !have_ord( ord ) )
			{
				m.setOrdinal(ord);
				break;
			}
		}

	}


	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#slots_needed()
	 */
	@Override
	public int slots_needed() throws PlcException
	{
		set_ordinals();

		int max = -1;

		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			if( m.getOrdinal() > max )
				max = m.getOrdinal();
		}

		return max+1;
	}

	
	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#preprocess(ru.dz.plc.compiler.ParseState)
	 */
	@Override
	public void preprocess( ParseState ps ) throws PlcException
	{

		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			ps.set_method( m );
			m.preprocess(ps.get_class());
			if( m.code != null ) m.code.preprocess(ps);
			ps.set_method( null );
		}
	}

	
	//@Override
	public void checkDefaultConstructor( ParseState ps ) throws PlcException
	{
		// Find all constructors
		//System.out.println("Check default constructor for "+ps.get_class().getName());
		//List<Method> ctors = new LinkedList<>();
		boolean haveConstructor = false;

		for( Method cm : mstable.values() )
			if( cm.isConstructor() )
			{
				//ctors.add(cm);
				haveConstructor = true;
			}

		if(!haveConstructor)
			createDefaultConstructor(ps);
		
	}
	
	private void createDefaultConstructor(ParseState ps) throws PlcException 
	{
		PhantomClass me = ps.get_class();
		
		if(me.isInternal())
		{
			// It is pointless to generate anything for .internal classes, they're hardcoded in VM
			return;
			//throw new PlcException("createDefaultConstructor","Attempt to create c'tor for internal class "+me.getName());
		}
		
		String parentName = me.getParent();
		PhantomClass parent = ClassMap.get_map().get(parentName, true, ps);
		if( parent == null )
			throw new PlcException("Can't find base class "+parentName+" for class "+me.getName());

		// make sure he did it - can't, dies
		//parent.createDefaultConstructor(new ParseState(parent));
		
		Method defc = parent.getDefaultConstructor();

		if( defc == null )
			throw new PlcException("createDefaultConstructor","No default constructor in base class "+parentName+" for class "+me.getName());

		Method cm = new Method( Method.CONSTRUCTOR_M_NAME, PhantomType.getVoid(), true );

		cm.code = new EmptyNode(); // Preprocess must add call to parent c'tor

		//cm.preprocess(me);

		//System.out.println("Created default constructor for "+me.getName());
		add(cm);
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#codegen(java.io.RandomAccessFile, java.io.FileWriter, java.io.BufferedWriter, ru.dz.plc.compiler.CodeGeneratorState, java.lang.String)
	 */
	@Override
	//public void codegen(RandomAccessFile os, FileWriter lst, BufferedWriter llvmFile, BufferedWriter c_File, CodeGeneratorState s, String version) throws IOException, PlcException {
	public void codegen(CodeWriters cw, CodeGeneratorState s) throws IOException, PlcException {
		set_ordinals();
		cw.lstc.write("Class version "+cw.getVersionString()+"\n\n");

		// Preprocess all first
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			s.set_method( m );

			m.preprocess( s.get_class() );
		}		
		
		// Now codegen
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			s.set_method( m );

			//m.preprocess( s.get_class() );

			cw.lstc.write("method "+m.getName()+" ordinal "+m.getOrdinal()+"\n--\n");
			cw.llvmFile.write("\n\n; method "+m.getName()+" ordinal "+m.getOrdinal()+"\n; --\n\n");
			cw.c_File.write("\n\n// method "+m.getName()+" ordinal "+m.getOrdinal()+"\n// --\n\n");
			//cw.javaFile.write("\n\n// method "+m.getName()+" ordinal "+m.getOrdinal()+"\n// --\n\n");

			generateJavaStub(cw, s, m);

			MethodFileInfo mf = new MethodFileInfo(cw.get_os(), cw.lstc, m, s);
			mf.write();

			MethodSignatureFileInfo ms = new MethodSignatureFileInfo(cw.get_os(), m, s);
			ms.write();

			MethodLineNumbersFileInfo ml = new MethodLineNumbersFileInfo(cw.get_os(),m);
			ml.write();

			m.generateLlvmCode(s, cw.llvmFile);
			m.generateC_Code(s, cw.c_File);

			s.set_method( null );
			cw.lstc.write("--\nmethod end\n\n");
			cw.llvmFile.write("\n\n; end of method "+m.getName()+" ordinal "+m.getOrdinal()+"\n; --\n\n");
			cw.c_File.write("\n\n// end of method "+m.getName()+" ordinal "+m.getOrdinal()+"\n// --\n\n");
		}
	}

	public void generateJavaStub(CodeWriters cw, CodeGeneratorState s, Method m) throws PlcException, IOException {
		String javaMethodName = m.isConstructor() ? s.get_class().getShortName() : m.getName();

		cw.javaFile.write( "\tpublic abstract " );
		if(!m.isConstructor()) cw.javaFile.write( m.getType().toJavaType()+" " );
		cw.javaFile.write( javaMethodName+"( " );

		Iterator<ArgDefinition> iter = m.getArgIterator();
		while( iter.hasNext() )
		{
			ArgDefinition arg = iter.next();
			cw.javaFile.write( arg.getType().toJavaType()+" "+arg.getName() );

			if(iter.hasNext())
				cw.javaFile.write( ", " );
		}
		if(m.getArgCount() == 0)
			cw.javaFile.write( "void" );

		cw.javaFile.write( " );\n" );
	}


	@Override
	public void dump()
	{
		System.out.println("Methods:");
		for( Iterator<Method> i = mstable.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			System.out.println("  Method "+m.toString()+":");
		}
	}

	/**
	 * Is there a method with given name
	 * @param name Method name
	 * @return true if at least one method with this name exists.
	 * 
	 */
	public boolean has(String name) {
		List<Method> ml = getAllForName(name);
		return !ml.isEmpty();
	}




}

