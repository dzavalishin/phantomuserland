package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomField;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

 /**
 * Static field Node. If this Node is executed - it is a static field load.
 */

public class StaticSaveNode extends Node {
	private String ident;
	private PhantomClass my_class;

	public String get_name() { return ident; }

	public StaticSaveNode( PhantomClass c, String ident, Node value ) {
		super(value);
		this.ident = ident;
		my_class = c;
	}
	public String toString()  {    return "save static field "+ident;  }
	public PhantomType find_out_my_type() throws PlcException { throw new PlcException( "static save Node", "no type known", ident ); }
	public boolean is_const() { return false; }

	
	@Override
	public boolean is_on_int_stack() {
		return false;
	}
	
	public void preprocess_me( ParseState s ) throws PlcException {
		PhantomField f = s.get_class(). findStaticField(ident);
		if( f == null )
			throw new PlcException( "save static field Node", "no such field in class", ident );
	}

	// save static fld
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		PhantomField f = s.get_class(). findStaticField(ident);
		
		// Class
		c.emitSummonByName(my_class.getName());
		
		// TODO check param order
		
		// Parameter - ordinal
		c.emitIConst_32bit(f.getOrdinal()); 
		
		// Parameter - value
		_l.generate_code(c, s);
		move_between_stacks(c, _l.is_on_int_stack(), _l.getType()); 
		
		c.emitCall(10,2); // Method number 10, 2 parameters (static field ordinal, value) - write static field		
		c.emitOsDrop(); // returned value
	}

}
