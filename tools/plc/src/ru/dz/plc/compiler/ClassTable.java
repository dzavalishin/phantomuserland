package ru.dz.plc.compiler;

import java.util.*;
import java.io.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.PlcMain;
//import ru.dz.plc.parser.ParserContext;
import ru.dz.plc.util.*;

/**
 * <p>Used to look for classes.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ClassTable {

	private Map<String, PhantomClass> table;

	ClassTable() {    
		table = new HashMap<String, PhantomClass>();  
	}

	void add( PhantomClass pc ) throws PlcException {
		String name = pc.getName();
		if( have( name ) && !(name.equals(".internal.object")) ) 
			throw new PlcException( "class table add", "duplicate class name", name ) ;
		table.put(name, pc );
	}

	boolean have( String name ) { return table.containsKey(name); }

	public PhantomClass get( String name, boolean justTry, ParseState pc ) { 
		if(name.charAt(0) != '.' && (!justTry))
		{
			System.out.println("Warning: Request for non-absolute class "+name);
			name = "."+name;
		}

		PhantomClass ret = ((PhantomClass)table.get(name));

		// Note which classes caller references
		if( pc != null && ret != null )
			pc.addReferencedClass(ret);

		return ret;
	}

	public void print() throws PlcException
	{
		for( Iterator<PhantomClass> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomClass c = i.next();
			//System.out.println("    Class    :");
			System.out.println("--------------");
			c.print(System.out);
		}
	}

	public void codegen() throws IOException, PlcException {
		for( Iterator<PhantomClass> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomClass c = i.next();

			CodeWriters cw = new CodeWriters( c.getName() );

			ClassFileInfo cf =
					new ClassFileInfo(
							cw.get_of(), c.getName(), c.getParent() == null ? "" : c.getParent(),
									c.getFieldSlotsNeeded(), c.getMethodSlotsNeeded(),
									cw.getVersionString()
							);

			cf.write();
			//c.codegen(of, lst, llvmFile, c_File, verstr);
			c.codegen( cw );
			//cf.reWrite();


			c.print(cw.get_lstps());

			{

				PrintStream d_ps = cw.get_d_ps();

				d_ps.format("%s: ", c.getName().substring(1)+".pc" );
				d_ps.format("%s", c.getName().substring(1)+".ph" );

				Set<PhantomClass> referencedClasses = c.getReferencedClasses();
				if( referencedClasses != null )
				{
					for( PhantomClass ref : referencedClasses )
					{
						d_ps.format("\t%s", ref.getName().substring(1)+".pc" );
					}
				}

				d_ps.format("\n");

			}

			cw.closeAll();


		}
	}

	RandomAccessFile find_import_file( String iname ) throws java.io.IOException
	{
		RandomAccessFile of = null;

		String tryname = iname + ".pc";

		try { of = new RandomAccessFile(tryname, "r"); return of; }
		catch (FileNotFoundException ex) { /* fall through */ }

		for( File path : PlcMain.getClassFileSearchPath() )
		{
			File tryFile = new File(path,tryname);
			try { of = new RandomAccessFile(tryFile, "r"); return of; }
			catch (FileNotFoundException ex) { /* fall through */ }
		}

		//System.out.println("Search failed: "+tryname);

		return null;
	}

	boolean do_import( String name ) throws PlcException {
		boolean starred = false;

		// BUG
		// will search some classpath in future or even access network servers

		int starindex = name.lastIndexOf("*");
		if( starindex > 0 )
		{
			if( starindex != name.length()-1 || name.lastIndexOf(".") != name.length()-2 )
				throw new PlcException("class import", "invalid * placement in import", name );

			starred = true;
			name = name.substring( 0, name.length()-2 );
		}

		if(starred)
			throw new PlcException("class import", "* in import is not implemented", name );

		if( name.indexOf(".") == 0 ) name = name.substring(1); // remove leading '.'

		return tryImport(name);
	}

	private boolean tryImport(String name) throws PlcException {
		try
		{
			RandomAccessFile is = find_import_file(name);
			if (is == null) return false;
			// actual import 

			ClassInfoLoader ci = new ClassInfoLoader(is);

			boolean load_success = ci.load_class_file();
			is.close();

			if(!load_success)
				return false;

			add( ci.get_class() );

		} catch (java.io.IOException e)
		{
			return false;
		}


		return true;
	}

	public void clear() {
		table.clear();
	}

	public void listMethods() {
		for( Iterator<PhantomClass> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomClass c = i.next();
			c.listMethods();
		}
	}

	public void set_ordinals() throws PlcException 
	{
		for( PhantomClass c : table.values() )
			c.set_ordinals();
	}

	
	public void preprocess() throws PlcException 
	{
		// Process referenced classes first
		
		/*		
		List<PhantomClass> todo = new ArrayList<>( table.values() ); 
		
		do {
			boolean skipped = false;
			PhantomClass c = todo.remove(0);
			
			for( PhantomClass ref : todo )
			{
				if( c.getReferencedClasses().contains(ref) )
				{
					System.err.println("Class "+c.getName()+" depends on "+ref);
					skipped = true;
				}
				
			}

			if( skipped )
			{
				todo.add(c);
				System.err.println("Reordered "+c.getName());
				continue;
			}
			
			c.createDefaultConstructor( new ParseState(c) );
			c.set_ordinals();
			//c.preprocess( new ParseState(c) );
			
		} while(!todo.isEmpty());
		
		for( Iterator<PhantomClass> i = table.values().iterator(); i.hasNext(); )
		{
			PhantomClass c = i.next();
			
			//c.getReferencedClasses()
			
			//c.listMethods();
			c.createDefaultConstructor( new ParseState(c) );
			c.preprocess( new ParseState(c) );
		}
		*/

		for( PhantomClass c : table.values() )
		{
			c.createDefaultConstructor( new ParseState(c) );
			c.set_ordinals();
		}

		for( PhantomClass c : table.values() )
			c.preprocess( new ParseState(c) );
		
		
	}

	public void propagateVoid() throws PlcException 
	{
		for( PhantomClass c : table.values() )
			c.propagateVoid();
	}


}

