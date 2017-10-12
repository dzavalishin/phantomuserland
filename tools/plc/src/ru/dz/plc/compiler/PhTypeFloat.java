package ru.dz.plc.compiler;

import ru.dz.plc.util.PlcException;

public class PhTypeFloat extends PhantomType {
	public PhTypeFloat() throws PlcException 
	{

		super( ClassMap.get_map().get(".internal.float",false,null) );	
		_is_float = true;
	}

}
