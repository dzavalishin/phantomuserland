package ru.dz.plc.compiler;

import ru.dz.plc.util.*;

public class GeneralState {
	
	protected PhantomClass my_class;
	private Method       my_method;

	public GeneralState() {
		my_class = null;
		my_method = null;
	}
	
	public GeneralState(PhantomClass c)
	{
		my_class = c;
		my_method = null;
	}

	public GeneralState(ParseState ps) 
	{
		my_class = ps.my_class;
		my_method = ps.get_method();
	}

	public void set_method ( Method m ) { my_method = m; }
	public Method  get_method ( ) { return my_method; }

	public PhantomStack stack_vars() { return my_method.svars; }
	public PhantomStack istack_vars() { return my_method.isvars; }

	public PhantomClass get_class() throws PlcException
	{
		if( my_class == null )
			throw new PlcException("state", "class requested, but is null");
		return my_class;
	}

	public Method getMethod() { return my_method; }

}
