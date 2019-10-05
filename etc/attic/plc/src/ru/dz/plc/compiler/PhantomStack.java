package ru.dz.plc.compiler;

import java.util.*;

/**
 * <p>Title: ru.dz.plc.compiler</p>
 * <p>Description: runtime stack planning and access class</p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: </p>
 * @author dz
 * @version 1.0
 */

public class PhantomStack
{
	private int used_slots = 0;
	private Map<String, PhantomStackVar> vars = new HashMap<String, PhantomStackVar>();

	public PhantomStackVar add_stack_var( PhantomVariable v )
	{
		PhantomStackVar sv = new PhantomStackVar( v, used_slots++ );
		vars.put(v.getName(),sv);
		return sv;
	}

	public boolean have( String name )
	{
		return vars.containsKey(name);
	}

	public PhantomStackVar get_var( String name )
	{
		return (PhantomStackVar)vars.get(name);
	}

	public int get_stackpos( String name )
	{
		PhantomStackVar sv = vars.get(name);
		return sv.get_abs_stack_pos();
	}

	public int getUsedSlots() { return used_slots; }

}
