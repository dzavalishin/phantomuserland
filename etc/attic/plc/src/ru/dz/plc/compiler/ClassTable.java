package ru.dz.plc.compiler;

import java.util.*;
import java.io.*;

import ru.dz.phantom.code.*;
import ru.dz.plc.PlcMain;
import ru.dz.plc.parser.ParserContext;
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
		if( have( name ) && !(name.equals(".internal.object")) ) throw new PlcException( "class table add", "duplicate class name", name ) ;
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
			{
				String classfns = c.getName()+".pc";
				String lstfns = c.getName()+".lstc";
				String verfns = c.getName()+".ver";
				
				// skip leading point
				File classfn = new File( PlcMain.getOutputPath(), classfns.substring(1));
				File lstfn = new File( PlcMain.getOutputPath(), lstfns.substring(1));

				classfn.delete();
				RandomAccessFile of = new RandomAccessFile( classfn, "rw" );

				lstfn.delete();
				FileWriter lst = new FileWriter(lstfn);

				{
					/*
					File verfn = new File( PlcMain.getOutputPath(), verfns.substring(1));
					RandomAccessFile vf = new RandomAccessFile( verfn, "rw" );
					String verstr = vf.readLine();
					int ver = 0;
					if(verstr.matches("Version=[0-9]+"))
					{
						
					}
					else
					{
						verstr = String.format("%4d", year, month, day, hour, min, sec );
					}*/

					
				}

				Calendar cc = Calendar.getInstance(TimeZone.getTimeZone("GMT"));						
				String verstr = String.format("%4d%02d%02d%02d%02d%02d%03d", 
						cc.get(Calendar.YEAR), 
						cc.get(Calendar.MONTH), 
						cc.get(Calendar.DAY_OF_MONTH), 
						cc.get(Calendar.HOUR), 
						cc.get(Calendar.MINUTE), 
						cc.get(Calendar.SECOND),
						cc.get(Calendar.MILLISECOND)
						);
				
				ClassFileInfo cf =
					new ClassFileInfo(
							of, c.getName(), c.getParent() == null ? "" : c.getParent(),
									c.getFieldSlotsNeeded(), c.getMethodSlotsNeeded(),
									verstr
					);

				cf.write();
				c.codegen(of, lst, verstr);
				//cf.reWrite();

				of.close();
				lst.close();
			}

			{
				String fns = c.getName()+".lst";
				// skip leading point
				File fn = new File( PlcMain.getOutputPath(), fns.substring(1));			
				fn.delete();
				
				FileOutputStream os = new FileOutputStream( fn );
				//BufferedOutputStream bos = new BufferedOutputStream(os);
			
				PrintStream ps = new PrintStream(os);
				
				c.print(ps);
				os.close();
				ps.close();
			}

			{
				String fns = c.getName()+".d";
				// skip leading point
				File fn = new File( PlcMain.getOutputPath(), fns.substring(1));			
				fn.delete();
				
				FileOutputStream os = new FileOutputStream( fn );
				//BufferedOutputStream bos = new BufferedOutputStream(os);
			
				PrintStream ps = new PrintStream(os);
				
				//c.print(ps);
				
				ps.format("%s: ", c.getName().substring(1)+".pc" );
				ps.format("%s", c.getName().substring(1)+".ph" );
				
				for( PhantomClass ref : c.getReferencedClasses() )
				{
					ps.format("\t%s", ref.getName().substring(1)+".pc" );
				}
				ps.format("\n");
				
				os.close();
				ps.close();
			}
		
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

}

