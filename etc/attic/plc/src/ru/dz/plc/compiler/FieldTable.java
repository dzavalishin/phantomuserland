package ru.dz.plc.compiler;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.RandomAccessFile;
import java.util.*;

import ru.dz.phantom.code.FieldFileInfo;
import ru.dz.plc.util.*;

/**
 * <p>Fields of a class.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class FieldTable {
	private Map<String, PhantomField> table;
	private ordinals_generator ordinals = new ordinals_generator();
	//int nextord = 0;

	void setBase(int base) throws PlcException { ordinals.setBase(base); }

	public FieldTable() { table = new HashMap<String, PhantomField>(); }

	private boolean have_ord( int ord )
	{
		for( Iterator<PhantomField> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomField m = i.next();
			if( m.getOrdinal() == ord ) return true;
		}
		return false;
	}

	void add( String name, PhantomType type )
	{
		table.put(name, new PhantomField( name, type, ordinals.getNext() ));
	}

	void set( int ordinal, String name, PhantomType type ) throws PlcException
	{
		table.put(name, new PhantomField( name, type, ordinal ));
		ordinals.ensureBase(ordinal+1);
	}
	
	/*void add( String name, PhantomType type, int ordinal  ) throws PlcException {
    if( have_ord(ordinal) )
      throw new PlcException( "field table add", "ordinal is already used", Integer.toString(ordinal) );
    table.put(name, new PhantomField( name, type, ordinal ));
    nextord = ordinal+1;
  }*/

	boolean have( String name ) { return table.containsKey(name); }

	//int get_ord( String name ) { return ((field)table.get(name)).ordinal; }

	PhantomField get( String name ) { return (PhantomField)table.get(name); }

	void print(PrintStream ps)
	{
		for( Iterator<PhantomField> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomField f = i.next();
			ps.println("  Field "+f.name+"");
		}
	}

	public int slots_needed()
	{
		int max = -1;

		for( Iterator<PhantomField> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomField m = i.next();
			if( m.getOrdinal() > max ) max = m.getOrdinal();
		}

		return max+1;
	}

	public void codegen(RandomAccessFile os, FileWriter lst,
			CodeGeneratorState s, String version) throws PlcException 
	{
		for( PhantomField f : table.values())
		{
			FieldFileInfo info = new FieldFileInfo(os, lst, f);
			try {
				info.write();
			} catch (IOException e) {
				throw new PlcException("Writing field "+f.getName(), e.toString());
			}
		}
	}

}

