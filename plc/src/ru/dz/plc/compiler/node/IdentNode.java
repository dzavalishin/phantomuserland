package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomField;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.util.PlcException;

 /**
 * Identifier Node. If this Node is executed - it is a variable load.
 */

public class IdentNode extends Node {
	String ident;
	PhantomClass my_class;

	public String get_name() { return ident; }

	public IdentNode( /*PhantomClass c,*/ String ident ) {
		super(null);
		this.ident = ident;
		//my_class = c;
	}
	public String toString()  {    return "ident "+ident;  }
	public void find_out_my_type() throws PlcException { if( type == null ) throw new PlcException( "ident Node", "no type known", ident ); }
	public boolean is_const() { return false; }

	public void preprocess_me( ParseState s ) throws PlcException {
		//PhantomField f = s.get_class().ft.get(ident);
		PhantomField f = s.get_class().find_field(ident);
		
		
		if( f != null )
		{
			/*if (type == null || type.is_unknown())*/ type = f.get_type();
			return;
		}

		PhantomStackVar svar = s.stack_vars().get_var(ident);
		if( svar == null )
			throw new PlcException( "ident Node", "no such field in class", ident );

		type = svar.get_type();
	}

	// load variable
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		//PhantomField f = s.get_class().ft.get(ident);
		PhantomField f = s.get_class().find_field(ident);

		if( f != null )
		{
			//if (type == null || type.is_unknown()) type = f.get_type();
			c.emitLoad(f.get_ordinal());
			return;
		}

		PhantomStackVar svar = s.stack_vars().get_var(ident);
		if( svar == null )
			throw new PlcException( "ident Node", "no field", ident );

		//if (type == null || type.is_unknown()) type = svar.get_type();
		c.emitGet(svar.get_abs_stack_pos()); // get stack variable

	}

}
