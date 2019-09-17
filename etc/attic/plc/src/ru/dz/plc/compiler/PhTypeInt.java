package ru.dz.plc.compiler;

import ru.dz.plc.util.PlcException;

public class PhTypeInt extends PhantomType {
	public PhTypeInt() throws PlcException //throws PlcException
	{
		//super( null );
		//super( new PhantomClass(".internal.int") );

		super( ClassMap.get_map().get(".internal.int",false,null) );	
		_is_int = true;
	}

}
