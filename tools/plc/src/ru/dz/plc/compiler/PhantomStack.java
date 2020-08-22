package ru.dz.plc.compiler;

import java.util.*;

/**
 * <p>Phantom language compiler</p>
 * <p>Runtime stack planning and access class</p>
 * <p>Copyright: Copyright (c) 2004-2019</p>

 * @author dz
 * @version 1.0
 */

public class PhantomStack
{
	private int used_slots = 0;
	private Map<String, PhantomStackVar> vars = new HashMap<String, PhantomStackVar>();

	public PhantomStackVar addStackVar( PhantomVariable v )
	{		
		PhantomStackVar sv = new PhantomStackVar( v, used_slots );
		vars.put(v.getName(),sv);
		
		if(v.getType().is64bit()) used_slots += 2;
		else used_slots++;
		
		return sv;
	}

	/**
	 *  Reserve a parameter on stack, setting its name and type
	 * @param parameterPosition object stack slot position for parameter
	 * @param name var name
	 * @param type var type
	 * @return stack var def (supposed to be unused, though)
	 */
	public PhantomStackVar setParameter( int parameterPosition, String name, PhantomType type )
	{
		if( used_slots <= parameterPosition )
			used_slots = parameterPosition+1;
		
		PhantomVariable v = new PhantomVariable(name, type);
		
		PhantomStackVar sv = new PhantomStackVar( v, parameterPosition );
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

	/*
	public int get_stackpos( String name )
	{
		PhantomStackVar sv = vars.get(name);
		return sv.get_abs_stack_pos();
	}*/

	/**
	 * 
	 * @return Number of 32 bit stack slots needed to store all the variables.
	 */
	public int getUsedSlots() { return used_slots; }

}
