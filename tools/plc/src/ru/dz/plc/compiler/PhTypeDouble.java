package ru.dz.plc.compiler;

public class PhTypeDouble extends PhantomType 
{
	public PhTypeDouble() 
	{
		super( ClassMap.get_map().get(".internal.double",false,null) );	
		_is_double = true;
	}
}
