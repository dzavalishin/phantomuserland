package ru.dz.plc.compiler;

import ru.dz.plc.util.PlcException;

import java.io.IOException;

import org.omg.PortableInterceptor.SYSTEM_EXCEPTION;

/**
 * <p>Class map. Integrates classes we did now and those we imported.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ClassMap {
	ClassTable       classes = new ClassTable();
	ClassTable       imported_classes = new ClassTable();

	static ClassMap  static_hack;
	public static ClassMap  get_map() { return static_hack; }
	public ClassMap() { static_hack = this; }



	public void print() throws PlcException { classes.print(); }
	public void codegen() throws PlcException, IOException { classes.codegen(); }

	/**
	 * do_import - import class
	 *
	 * @param cn String - class name
	 * @throws PlcException
	 * @return boolean - true for success (class imported or already is here)
	 */
	public boolean do_import( String cn ) throws PlcException
	{
		if( imported_classes.have(cn) ) return true;
		if( classes.have(cn) ) return true;
		//System.out.println("ClassMap.do_import "+cn);
		return imported_classes.do_import(cn);
	}

	public void add( PhantomClass c ) throws PlcException { classes.add(c); }

	public void imported_add_hack( PhantomClass c ) throws PlcException { imported_classes.add(c); }

	public PhantomClass get( String cln, boolean justTry )
	{
		PhantomClass c = classes.get(cln,justTry);
		if (c == null)
			c = imported_classes.get(cln,justTry);
		return c;
	}


}
