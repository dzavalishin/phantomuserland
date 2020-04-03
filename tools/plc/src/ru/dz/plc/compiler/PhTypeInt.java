package ru.dz.plc.compiler;

public class PhTypeInt extends PhantomType {
	public PhTypeInt() 
	{
		super( ClassMap.get_map().get(".internal.int",false,null) );
		//if( _class == null ) throw new PlcException("PhTypeInt", "class not loaded");
		_is_int = true;
	}

}
