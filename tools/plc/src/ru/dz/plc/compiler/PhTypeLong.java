package ru.dz.plc.compiler;

import ru.dz.plc.util.PlcException;

public class PhTypeLong extends PhantomType {
	public PhTypeLong() throws PlcException
	{
		super( ClassMap.get_map().get(".internal.long",false,null) );	
		_is_long = true;
	}

}
