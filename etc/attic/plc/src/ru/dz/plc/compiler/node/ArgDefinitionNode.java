package ru.dz.plc.compiler.node;

import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * Method argument.
 * @author dz
 */

public class ArgDefinitionNode extends Node {
	String my_name;
	public ArgDefinitionNode( String my_name, PhantomType my_type ) {
		super(null);
		this.type = my_type;
		this.my_name = my_name;
	}
	public String toString()  {    return "arg def ("+my_name+")";  }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	public void find_out_my_type() { if( type == null ) type = new PhTypeUnknown(); }
}

