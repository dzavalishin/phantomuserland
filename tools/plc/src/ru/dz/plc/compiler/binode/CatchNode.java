package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Catch node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class CatchNode extends BiNode {
	private String ident;
	private PhantomType catch_type;

	/**
	 * Exception catch.
	 * @param catch_type Type of exception to catch.
	 * @param ident Variable that will receive thrown value if exception is catched. 
	 * @param code Code to execute and look for exceptions in.
	 * @param catcher Code to execute on catch.
	 */
	public CatchNode(PhantomType catch_type, String ident, Node code, Node catcher)
	{
		super(code,catcher);
		this.catch_type = catch_type;
		this.ident = ident;
	}

	public String toString()  { return "catch";  }


	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		String out_label = c.getLabel();
		String catch_label = c.getLabel();

		if( _l == null )
			throw new PlcException("catch_node","can't skip code");

		if( catch_type.is_container() )
			System.out.println("Warning, trying to catch container: "+catch_type.toString());

		log.fine("Node "+this+" codegen");

		// here we have to push class object of type to catch
		catch_type.emit_get_class_object(c,s);
		c.emitPushCatcher(catch_label);

		_l.generate_code(c,s);
		c.emitPopCatcher();
		c.emitJmp(out_label);


		c.markLabel(catch_label);
		// generate cacher header
		generate_my_code(c, s);
		if( _r != null ) { _r.generate_code(c,s); move_between_stacks(c, _r.is_on_int_stack(), _r.getType()); }

		c.markLabel(out_label);
	}

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {

		// We get here on exception. Top of stack is thrown data.
		// Save it to our variable.

		/* can't be a class field
	    PhantomField f = s.get_class().ft.get(ident);
	    if( f != null )
	    {
	      if (type == null || type.is_unknown()) type = f.get_type();
	      c.emit_save(f.get_ordinal());
	      return;
	    }*/

		PhantomStackVar svar = s.stack_vars().get_var(ident);
		if( svar == null )
			throw new PlcException( "catch Node", "no variable??", ident );

		c.emitSet(svar.get_abs_stack_pos()); // set stack variable
	}

}

