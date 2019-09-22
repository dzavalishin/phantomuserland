package ru.dz.plc.compiler;

public class PhTypeLong extends PhantomType 
{
	public PhTypeLong()
	{
		super( ClassMap.get_map().get(".internal.long",false,null) );	
		_is_long = true;
	}

}
