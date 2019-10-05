package ru.dz.plc.compiler;

public class PhTypeFloat extends PhantomType 
{
	public PhTypeFloat() 
	{

		super( ClassMap.get_map().get(".internal.float",false,null) );	
		_is_float = true;
	}

}
