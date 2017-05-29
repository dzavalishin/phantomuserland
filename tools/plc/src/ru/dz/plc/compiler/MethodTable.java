package ru.dz.plc.compiler;

import java.util.*;
import java.io.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.util.*;

/**
 * <p>Class methods table.</p>
 * <p>Dumb implementation, does not support polymorphism.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */

public class MethodTable implements IMethodTable 
{
	private Map<String, Method> table = new HashMap<String, Method>();
	private Map<MethodSignature, Method> mstable = new HashMap<MethodSignature, Method>();
	
	protected ordinals_generator ordinals = new ordinals_generator();

	//public MethodTable() { table = new HashMap<String, Method>(); }

	public Iterator<Method> iterator() { return table.values().iterator(); }

	private boolean have_ord( int ord )
	{
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			if( m.getOrdinal() == ord ) return true;
		}
		return false;
	}

	private boolean mine( Method t )
	{
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
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
		m.setOrdinal( ord );
	}

	@Override @Deprecated
	public Method add( String name, PhantomType type ) throws PlcException
	{
		//assert(name != null);
		if(name == null)
			throw new PlcException("Null method name", name);
		Method m = new Method( name, type );
		table.put(name, m);
		return m;
	}

	@Override
	public Method add( Method m ) throws PlcException
	{
		//assert(m.getName() != null);
		if(m.getName() == null)
			throw new PlcException("Null method name", m.toString());
		mstable.put(m.getSignature(), m);
		table.put(m.getName(), m);
		return m;
	}

	@Override @Deprecated
	public boolean have( String name ) { return table.containsKey(name); }
	@Override @Deprecated
	public Method get( String name ) { return (Method)table.get(name); }

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
		Method m =  mstable.get(signature);
		if( m != null ) return m;
		
		return checkPossibleConversions(signature);
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
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			ps.println("  Method "+m.toString()+":");
			if( m.code != null )      m.code.print(ps,2,2);
			else ps.println("  -- No code!");
			ps.println("");
		}
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#set_ordinals()
	 */
	@Override
	public void set_ordinals()
	{
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			if( m.getOrdinal() < 0 )
			{
				while( true )
				{
					int ord = ordinals.getNext();
					if( !have_ord( ord ) )
					{
						m.setOrdinal(ord);
						break;
					}
				}
			}
		}
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#slots_needed()
	 */
	@Override
	public int slots_needed()
	{
		set_ordinals();

		int max = -1;

		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
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
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			ps.set_method( m );
			if( m.code != null ) m.code.preprocess(ps);
			ps.set_method( null );
		}
	}

	/* (non-Javadoc)
	 * @see ru.dz.plc.compiler.IMethodTable#codegen(java.io.RandomAccessFile, java.io.FileWriter, java.io.BufferedWriter, ru.dz.plc.compiler.CodeGeneratorState, java.lang.String)
	 */
	@Override
	public void codegen(RandomAccessFile os, FileWriter lst, BufferedWriter llvmFile, BufferedWriter c_File, CodeGeneratorState s, String version) throws IOException, PlcException {
		set_ordinals();
		lst.write("Class version "+version+"\n\n");

		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			s.set_method( m );

			lst.write("method "+m.getName()+" ordinal "+m.getOrdinal()+"\n--\n");
			llvmFile.write("\n\n; method "+m.getName()+" ordinal "+m.getOrdinal()+"\n; --\n");

			MethodFileInfo mf = new MethodFileInfo(os, lst, m, s);
			mf.write();

			MethodSignatureFileInfo ms = new MethodSignatureFileInfo(os, m, s);
			ms.write();

			MethodLineNumbersFileInfo ml = new MethodLineNumbersFileInfo(os,m);
			ml.write();

			m.generateLlvmCode(s, llvmFile);
			m.generateC_Code(s, c_File);
			
			s.set_method( null );
			lst.write("--\nmethod end\n\n");
		}
	}


	@Override
	public void dump()
	{
		System.out.println("Methods:");
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			System.out.println("  Method "+m.toString()+":");
		}
	}

	
}

