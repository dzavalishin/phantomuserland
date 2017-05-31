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
	//static int nextord = 0;
	//static int nextord = 16; // BUG! Hack. In normal situation we will start from Method no, which is first not used number in parent. For now we start from 16 just to make sure user Method will not get 'internal' (reserved) number by mistake
	//int nextord = 0;
	ordinals_generator ordinals = new ordinals_generator();

	public MethodTable() { table = new HashMap<String, Method>(); }

	public Iterator<Method> iterator() { return table.values().iterator(); }

	private boolean have_ord( int ord )
	{
		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			if( m.ordinal == ord ) return true;
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
		m.ordinal = ord;
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

	/*Method add( String name, PhantomType type, Node args, Node code )
  {
    Method m = new Method( name, type, args, nextord++ ,code);
    table.put(name, m);
    return m;
  }

  void add( String name, PhantomType type, Node args, Node code, int ordinal  ) throws PlcException {
    if( have_ord(ordinal) )
      throw new PlcException( "Method table add", "ordinal is already used", Integer.toString(ordinal) );
    table.put(name,new Method( name, type, args, ordinal ,code));
    nextord = ordinal+1;
  }*/

	boolean have( String name ) { return table.containsKey(name); }
	Method get( String name ) { return (Method)table.get(name); }
	//Node get_code( String name ) { return ((Method)table.get(name)).code; }

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
			if( m.ordinal < 0 )
			{
				while( true )
				{
					int ord = ordinals.getNext();
					if( !have_ord( ord ) )
					{
						m.ordinal = ord;
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
			if( m.ordinal > max )
				max = m.ordinal;
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

	public void codegen(RandomAccessFile os, FileWriter lst, CodeGeneratorState s, String version) throws IOException, PlcException {
		set_ordinals();
		lst.write("Class version "+version+"\n\n");

		for( Iterator<Method> i = table.values().iterator(); i.hasNext(); )
		{
			Method m = i.next();
			s.set_method( m );

			lst.write("method "+m.name+" ordinal "+m.ordinal+"\n--\n");

			MethodFileInfo mf = new MethodFileInfo(os, lst, m, s);
			mf.write();

			MethodSignatureFileInfo ms = new MethodSignatureFileInfo(os, m, s);
			ms.write();
			
			MethodLineNumbersFileInfo ml = new MethodLineNumbersFileInfo(os,m);
			ml.write();

			s.set_method( null );
			lst.write("--\nmethod end\n\n");
		}
	}


}

