package ru.dz.plc.compiler;

public class PhTypeInt extends PhantomType {
	public PhTypeInt() 
	{
		super( ClassMap.get_map().get(".internal.int",false,null) );	
		_is_int = true;
	}

}
