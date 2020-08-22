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
	private boolean onIntStack = false;
	private boolean onObjStack = false;

	public String get_name() { return ident; }

	public IdentNode( /*PhantomClass c,*/ String ident ) {
		super(null);
		this.ident = ident;
		//my_class = c;
	}
	public String toString()  {    return "ident "+ident;  }
	public void find_out_my_type() throws PlcException { if( type == null ) throw new PlcException( "ident Node", "no type known", ident ); }
	public boolean is_const() { return false; }

	/*@Override
	public boolean args_on_int_stack() {
		// TODO Auto-generated method stub
		return s.istack_vars().get_var(ident) != null;
	}*/
	
	@Override
	public boolean is_on_int_stack() {
		if( onIntStack )			return true;
		if( onObjStack )			return false;
		throw new RuntimeException("Call to is_on_int_stack() before preprocess");
	}
	
	public void preprocess_me( ParseState s ) throws PlcException {
		//PhantomField f = s.get_class().ft.get(ident);
		PhantomField f = s.get_class().find_field(ident);
		
		
		if( f != null )
		{
			/*if (type == null || type.is_unknown())*/ type = f.getType();
			onObjStack = true;
			return;
		}

		PhantomStackVar svar = s.istack_vars().get_var(ident);
		if(svar != null)
		{
			onIntStack = true;
			type = svar.getType();
			if( !type.is_int() )
				throw new PlcException("Not an integer auto var on integer stack");
			return;
		}

		svar = s.stack_vars().get_var(ident);
		if( svar == null )
			throw new PlcException( "ident Node", "no such field in class", ident );

		onObjStack = true;
		type = svar.getType();
	}

	// load variable
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		//PhantomField f = s.get_class().ft.get(ident);
		PhantomField f = s.get_class().find_field(ident);

		if( f != null )
		{
			//if (type == null || type.is_unknown()) type = f.get_type();
			c.emitLoad(f.getOrdinal());
			return;
		}

		PhantomStackVar svar = s.istack_vars().get_var(ident);
		if( svar != null )
		{
			c.emitIGet(svar.get_abs_stack_pos()); // get stack variable
		}
		else
		{
		svar = s.stack_vars().get_var(ident);
		if( svar == null )
			throw new PlcException( "ident Node", "no field", ident );

		//if (type == null || type.is_unknown()) type = svar.get_type();
		c.emitGet(svar.get_abs_stack_pos()); // get stack variable
		}
		
	}

}
