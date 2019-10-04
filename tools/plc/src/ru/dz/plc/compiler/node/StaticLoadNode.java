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

public class StaticLoadNode extends Node {
	private String ident;
	private PhantomClass my_class;
	//private boolean onIntStack = false;
	//private boolean onObjStack = false;

	public String get_name() { return ident; }

	public StaticLoadNode( PhantomClass c, String ident ) {
		super(null);
		this.ident = ident;
		my_class = c;
	}
	public String toString()  {    return "load static field "+ident;  }
	public PhantomType find_out_my_type() throws PlcException { throw new PlcException( "static load Node", "no type known", ident ); }
	public boolean is_const() { return false; }

	
	@Override
	public boolean is_on_int_stack() {
		return false;
		
		//if( onIntStack )			return true;
		//if( onObjStack )			return false;
		//throw new RuntimeException("Call to is_on_int_stack() before preprocess");
	}
	
	public void preprocess_me( ParseState s ) throws PlcException {
		PhantomField f = s.get_class(). findStaticField(ident);
		if( f == null )
			throw new PlcException( "load static field Node", "no such field in class", ident );
	}

	// load static fld
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		PhantomField f = s.get_class(). findStaticField(ident);
		c.emitSummonByName(my_class.getName());
		c.emitIConst_32bit(f.getOrdinal()); // Parameter
		c.emitCall(11,1); // Method number 11, 1 parameter (static field ordinal) - read static field
		
	}
/*
	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s)
			throws PlcException {
		
		BiNode args = new BiNode() {
			
			@Override
			public String toString() {
				// TODO Auto-generated method stub
				return null;
			}
		};
		cgen.emitMethodCall(new ThisNode(my_class), 11, args , s);
	}
*/	
	
}
