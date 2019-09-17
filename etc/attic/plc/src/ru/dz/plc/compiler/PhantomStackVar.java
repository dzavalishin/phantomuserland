package ru.dz.plc.compiler;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class PhantomStackVar extends PhantomVariable
{
	int      abs_stack_pos;

	public PhantomStackVar( String name, PhantomType type , int n)
	{
		super( name, type );
		abs_stack_pos = n;
	}

	public PhantomStackVar( PhantomVariable v, int n)
	{
		super( v );
		abs_stack_pos = n;
	}


	public int      get_abs_stack_pos() { return abs_stack_pos; }

}
