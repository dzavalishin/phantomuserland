package ru.dz.plc.compiler;

import ru.dz.plc.util.PlcException;

public class PhTypeObject extends PhantomType {

	public PhTypeObject() throws PlcException //throws PlcException
	{
		//super( null );
		//super( new PhantomClass(".internal.int") );

		super( ClassMap.get_map().get(".internal.object",false,null) );	
		//_is_int = true;
	}
	
}
