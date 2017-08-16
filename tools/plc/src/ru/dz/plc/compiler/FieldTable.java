package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.RandomAccessFile;
import java.util.*;

import ru.dz.phantom.code.FieldFileInfo;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.ReturnNode;
import ru.dz.plc.compiler.node.StatementsNode;
import ru.dz.plc.util.*;

/**
 * <p>Fields of a class.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2016 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class FieldTable {
	private Map<String, PhantomField> table;
	private ordinals_generator ordinals = new ordinals_generator();

	void setBase(int base) throws PlcException { ordinals.setBase(base); }

	public FieldTable() { table = new HashMap<String, PhantomField>(); }

	/*
	private boolean have_ord( int ord )
	{
		for( Iterator<PhantomField> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomField m = i.next();
			if( m.getOrdinal() == ord ) return true;
		}
		return false;
	}
	*/

	PhantomField add( String name, PhantomType type )
	{
		PhantomField f = new PhantomField( name, type, ordinals.getNext() );
		table.put(name, f);
		return f;
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
			ps.println("  Field "+f.getName()+"");
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

	
	private static String capitalizeFirst(String name)
	{
		char ch = name.charAt(0);
		char uch = Character.toUpperCase(ch);
		return new String("")+uch+name.substring(1);
	}
	
	public static String makeGetterName(String fld)
	{
		return "get"+capitalizeFirst(fld);
	}
	
	public static String makeSetterName(String fld)
	{
		return "set"+capitalizeFirst(fld);
	}
	
	public void generateGettersSetters(PhantomClass pc) throws PlcException
	{
		for( PhantomField f : table.values())
		{
			generategetterSetter(pc,f);
		}
	}
	
	private void generategetterSetter(PhantomClass pc, PhantomField f) throws PlcException
	{
		if(!f.isPublic())
			return;
		
		Method get = new Method(makeGetterName(f.getName()), f.getType(), false );
		pc.addMethod(get);
		
		StatementsNode getNodes = new StatementsNode();
		get.code = getNodes;			
		getNodes.addNode(new ReturnNode(new IdentNode(f.getName())));
		
		
		// TODO write setter!
		/*
		Method set = new Method(makeSetterName(f.getName()), f.getType());
		pc.addMethod(set);
		
		StatementsNode setNodes = new StatementsNode();
		set.code = setNodes;			
		setNodes.addNode(new ReturnNode(new IdentNode(f.getName()))); 
		*/
		
	}
	
	public void codegen(RandomAccessFile os, FileWriter lst,
			BufferedWriter llvmFile, BufferedWriter c_File, CodeGeneratorState s, String version) throws PlcException 
	{
		//llvmFile.write("; fields: \n");
		for( PhantomField f : table.values())
		{
			FieldFileInfo info = new FieldFileInfo(os, lst, f);
			try {
				info.write();
				llvmFile.write("; - field "+f.getName()+"\n");
				c_File.write("// - field "+f.getName()+"\n");
			} catch (IOException e) {
				throw new PlcException("Writing field "+f.getName(), e.toString());
			}
		}
	}

}

