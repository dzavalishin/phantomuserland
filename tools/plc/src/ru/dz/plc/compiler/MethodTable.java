package ru.dz.plc.compiler;

import java.util.*;
import java.io.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.util.*;

/**
 * <p>Class methods table.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class MethodTable {
	private Map<String, Method> table;
	ordinals_generator ordinals = new ordinals_generator();

	public MethodTable() { table = new HashMap<String, Method>(); }

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

	public void set_ordinal( Method m, int ord ) throws PlcException {
		if( !mine( m ) ) throw new PlcException("set_ordinal","not my Method");
		if( ord != -1 && have_ord( ord ) ) throw new PlcException("set_ordinal","duplicate");
		m.setOrdinal( ord );
	}

	Method add( String name, PhantomType type )
	{
		Method m = new Method( name, type );
		table.put(name, m);
		return m;
	}

	Method add( Method m )
	{
		table.put(m.name, m);
		return m;
	}

	boolean have( String name ) { return table.containsKey(name); }
	Method get( String name ) { return (Method)table.get(name); }

	void print(PrintStream ps) throws PlcException
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

	public void codegen(RandomAccessFile os, FileWriter lst, BufferedWriter llvmFile, CodeGeneratorState s, String version) throws IOException, PlcException {
		set_ordinals();
		lst.write("Class version "+version+"\n\n");

		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			s.set_method( m );

			lst.write("method "+m.name+" ordinal "+m.getOrdinal()+"\n--\n");
			llvmFile.write("\n\n; method "+m.name+" ordinal "+m.getOrdinal()+"\n; --\n");

			MethodFileInfo mf = new MethodFileInfo(os, lst, m, s);
			mf.write();

			MethodSignatureFileInfo ms = new MethodSignatureFileInfo(os, m, s);
			ms.write();

			MethodLineNumbersFileInfo ml = new MethodLineNumbersFileInfo(os,m);
			ml.write();

			m.generateLlvmCode(s, llvmFile);
			
			s.set_method( null );
			lst.write("--\nmethod end\n\n");
		}
	}


	void dump()
	{
		System.out.println("Methods:");
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			System.out.println("  Method "+m.toString()+":");
		}
	}
	
	
}

