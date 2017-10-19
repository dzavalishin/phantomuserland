package ru.dz.phantom.file.pclass;

import java.io.IOException;
import java.io.RandomAccessFile;

import ru.dz.phantom.code.Fileops;

public abstract class GenericMethodSignatureLoaderHandler extends GenericLoaderHandler 
{
	boolean debug_print = false;

	protected String    myName;
	protected int       myOrdinal;
	protected int       myArgCount = 0;

	public GenericMethodSignatureLoaderHandler( RandomAccessFile is, int in_size, AbstractClassInfoLoader _loader ) throws
	IOException {
		super( is, in_size, _loader );
	}
	
	/**
	 * Call me from child c'tor
	 * @throws IOException
	 */
	protected void load() throws	IOException {

		myName = Fileops.get_string(is);

		myOrdinal = Fileops.get_int32(is);
		//if(debug_print) System.out.println(", ordinal: ", my_ordinal, "\n" );

		myArgCount = Fileops.get_int32(is);
		if(debug_print) System.out.println(myArgCount);

		boolean constructor = Fileops.get_int32(is) == 1;
		if(debug_print) System.out.println("    Is constructor: " + constructor);

		PhantomTypeInfo ret_ti = new PhantomTypeInfo(is);
		//PhantomType my_return_type = createMethod(constructor, ret_ti);
		createMethod(constructor, ret_ti);

		if(debug_print) System.out.print("Method is: " + myName + ", ret type is '"+ret_ti.toString()+"', arg count = "+myArgCount );

		for( int arg_no = 0; arg_no < myArgCount; arg_no++ )
		{
			String arg_name = Fileops.get_string(is);
			PhantomTypeInfo arg_ti = new PhantomTypeInfo(is);

			if(debug_print) System.out.print("Arg '"+arg_name+"' "+arg_ti.toString());
			
			/*
			if(debug_print) System.out.print(": '"+arg_ti.getMainClassName()+"' ");
			if(debug_print && arg_ti.isContainer())
			{
				System.out.print("( "+arg_ti.get_contained_class_name()+"[])");
			}
			if(debug_print) System.out.println("");
			*/
			
			createArg(arg_name, arg_ti);
		}


		if(debug_print) System.out.println("");
	}

	protected abstract void createMethod(boolean constructor, PhantomTypeInfo ret_ti) throws IOException;
	protected abstract void createArg(String arg_name, PhantomTypeInfo arg_ti) throws IOException;
	
}
