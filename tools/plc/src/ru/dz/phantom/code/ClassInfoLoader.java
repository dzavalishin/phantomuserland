package ru.dz.phantom.code;

import java.io.*;

import ru.dz.phantom.file.pclass.AbstractClassInfoLoader;
import ru.dz.phantom.file.pclass.GenericMethodSignatureLoaderHandler;
import ru.dz.phantom.file.pclass.PhantomTypeInfo;
import ru.dz.plc.compiler.*;
import ru.dz.plc.util.*;

import java.util.*;


/**
 * <p>Class file loader.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ClassInfoLoader extends AbstractClassInfoLoader {
	PhantomClass my_class;

	public ClassInfoLoader(RandomAccessFile is)
	{
		super(is);
	}

	public PhantomClass get_class() { return my_class; }

	@Override
	protected boolean isClassDefined() { return my_class != null; }


	@Override
	protected void createPoolConst(int cOrdinal, PhantomTypeInfo ti, byte[] buf) throws IOException {
		try {
			my_class.setPoolConstant( cOrdinal, new PhantomType(ti), buf );
		} catch (PlcException e) {
			throw new IOError(e);
		}
	}

	@Override
	protected void createField(String fieldName, int fOrdinal, PhantomTypeInfo ti) throws IOException {
		try {
			my_class.setField( fieldName, new PhantomType(ti), fOrdinal );
		} catch (PlcException e) {
			throw new IOError(e);
		}
	}

	@Override
	protected void createClass() throws IOException {
		try {
			my_class = new PhantomClass(class_name);

			if( (class_parent_name != null) && (class_parent_name.length() > 0) && (!class_parent_name.equals(".internal.object")) )
				my_class.addParent(class_parent_name,null);

			if(class_name != null && class_name.equals(".internal.object"))
			{
				// Hack! Hack!
				ru.dz.plc.compiler.ClassMap.get_map().imported_add_hack(my_class);
				// or else ww will die on adding Method signatures,
				// as they need types too
			}
		} catch (PlcException e) {
			throw new IOError(e);
		}
	}


	@Override
	protected GenericMethodSignatureLoaderHandler 
		getMethodSignatureLoaderHandler(RandomAccessFile is2, int in_size) throws IOException 
	{
		MethodSignatureLoaderHandler msh = new MethodSignatureLoaderHandler( is, in_size, this);
		try {
			my_class.addMethod(msh.getMe());
		} catch (PlcException e) {
			throw new IOError(e);
		}
		return msh;
	}


}








class MethodSignatureLoaderHandler extends GenericMethodSignatureLoaderHandler
{
	private Method    					me = null;
	private LinkedList<ArgDefinition> 	args = new LinkedList<ArgDefinition>();

	public MethodSignatureLoaderHandler(RandomAccessFile is, int in_size, AbstractClassInfoLoader _loader)
			throws IOException {
		super(is, in_size, _loader);
		load();
		
		_loader.addMethod(me);

	}


	@Override
	protected void createArg(String arg_name, PhantomTypeInfo arg_ti) throws IOException {
		PhantomType arg_type;
		try {
			arg_type = new PhantomType(arg_ti);
			ArgDefinition ad = new ArgDefinition( arg_name, arg_type );

			args.add(ad);
			getMe().addArg(arg_name, arg_type);
		} catch (PlcException e) {
			throw new IOException(e);
		}
	}

	@Override
	protected void createMethod(boolean constructor, PhantomTypeInfo ret_ti) throws IOException {
		try {
			PhantomType myReturnType = new PhantomType(ret_ti);

			//System.out.println("Create meth "+myName);

			me = new Method( myName, myReturnType, constructor );
			me.setOrdinal(myOrdinal);
			//return my_return_type;
		} catch (PlcException e) {
			throw new IOException(e);
		}
	}

	public int getOrdinal() { return myOrdinal; }
	public int getArgCount() { return myArgCount; }
	public String getName() { return myName; }

	public Method getMe() {		return me;	}



};


