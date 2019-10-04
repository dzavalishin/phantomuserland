package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>ForEach Node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ForeachNode extends BiNode {
	private String ident;

	public ForeachNode( String ident, Node expr, Node code )  throws IOException, PlcException
	{
		super(expr, code);
		this.ident = ident;
	}
	public String toString()  {    return "foreach " + ident;  }
	public PhantomType find_out_my_type() { return PhantomType.getVoid(); }
	public boolean is_const() { return false; }


	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		String break_label = c.getLabel();
		String continue_label = c.getLabel();

		String prev_continue = s.continue_label;
		String prev_break = s.break_label;

		// Automatic variable we use to keep iteration object in
		PhantomStackVar svar = s.stack_vars().get_var(ident);
		if( svar == null ) throw new PlcException( "foreach Node", "no variable??", ident );

		PhantomStackVar ivar = s.stack_vars().get_var(ident+"$iterator");
		if( ivar == null )  throw new PlcException( "foreach Node", "no variable??", ident+"$iterator" );

		if( _l == null)
			throw new PlcException("foreach_node","need something to iterate on");

		if( _l.is_on_int_stack() )
			throw new PlcException("foreach_node","can't iterate on integer");

		//if( !type.is_container() )      System.out.println("Need container: "+type.toString());

		log.fine("Node "+this+" codegen");


		s.continue_label = continue_label;
		s.break_label = break_label;

		// Generate container to iterate in
		_l.generate_code(c,s);
		// Method 8 of container is 'get iterator'. 0 args.
		c.emitCall(8,0);
		// Store iterator to our variable
		c.emitSet(ivar.get_abs_stack_pos());


		c.markLabel(continue_label); // again...
		c.emitGet(ivar.get_abs_stack_pos()); // get iterator variable
		c.emitOsDup(); // copy it --> A

		// Method 8 of iterator - get current value
		c.emitCall(8,0);
		c.emitOsDup(); // copy it --> B
		c.emitSet(svar.get_abs_stack_pos()); // store to loop variable

		c.emitIsNull(); // will compare with null   <-- B
		c.emitLogNot(); // isnull gives 0 if var == 0
		c.emitJz(break_label);

		// now use dup-ped value (iterator) - increment
		c.emitCall(333,0); // BUG! don't know what Method to call for increment :(
		c.emitOsDrop(); // call result

		if( _r != null )
			_r.generate_code(c,s);

		c.emitJmp(continue_label);
		c.markLabel(break_label);
		c.emitOsDrop();

		s.continue_label = prev_continue;
		s.break_label = prev_break;

	}




}
