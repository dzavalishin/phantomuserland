package ru.dz.plc.compiler;

import ru.dz.plc.util.PlcException;

public class PhTypeDouble extends PhantomType {
	public PhTypeDouble() throws PlcException 
	{
		super( ClassMap.get_map().get(".internal.double",false,null) );	
		_is_double = true;
	}
}
