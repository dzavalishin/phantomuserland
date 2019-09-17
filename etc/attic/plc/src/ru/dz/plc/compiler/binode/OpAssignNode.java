package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomField;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Assignment node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class OpAssignNode extends BiNode {
	/**
	 * Variable/array assign node.
	 * 
	 * @param var_or_array where to assign
	 * @param expr expression to assign 
	 */
	public OpAssignNode(Node var_or_array, Node expr) {    super(var_or_array,expr);  }
	public String toString()  {    return "=";  }

	public boolean args_on_int_stack() {
		if( _l.getClass() == IdentNode.class )
		{
			return _l.args_on_int_stack(); // integer stack var will tell it has args on int stack
		}
		return false; 
	}

	@Override
	public boolean is_on_int_stack() {
		return args_on_int_stack();
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		generate_my_code(c,s);
	}

	// NB! We must return a copy of assigned value.
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
	PlcException {
		if( _l.getClass() == IdentNode.class )
		{
			//if( _l != null ) { _l.generate_code(c,s); move_between_stacks(c, _l.is_on_int_stack()); }
			if( _r != null ) { _r.generate_code(c,s); move_between_stacks(c, _r.is_on_int_stack()); }

			IdentNode dest = (IdentNode) _l;
			String dest_name = dest.get_name();

			// Field?
			//PhantomField f = s.get_class().ft.get(dest_name);
			PhantomField f = s.get_class().find_field(dest_name);
			if (f != null) {
				if (type == null || type.is_unknown()) type = f.getType();
				check_assignment_types(f.getName(), type,_r.getType());
				c.emitOsDup(); // return a copy
				c.emitSave(f.getOrdinal());
				if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
				return;
			}

			// Stack var?
			PhantomStackVar svar = s.istack_vars().get_var(dest_name);
			if (svar != null)
			{
				if (type == null || type.is_unknown()) type = svar.getType();
				check_assignment_types(svar.getName(), type,_r.getType());
				c.emitIsDup(); // return a copy
				c.emitISet(svar.get_abs_stack_pos()); // set stack variable
				if(!is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on obj??!");
			}
			else
			{
				svar = s.stack_vars().get_var(dest_name);
				if (svar == null)
					throw new PlcException("= Node", "no field", dest_name);

				if (type == null || type.is_unknown()) type = svar.getType();
				check_assignment_types(svar.getName(), type,_r.getType());
				c.emitOsDup(); // return a copy
				c.emitSet(svar.get_abs_stack_pos()); // set stack variable
				if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
			}
		}
		else if( _l.getClass() == OpSubscriptNode.class )
		{
			// this is assignment to array element
			OpSubscriptNode dest = (OpSubscriptNode)_l;

			Node atom = dest.getLeft();
			Node subscr = dest._r;

			// array object to assign to
			atom.generate_code(c,s);
			move_between_stacks(c, atom.is_on_int_stack());

			// put value to assign
			if( _r != null ) { _r.generate_code(c,s); move_between_stacks(c, _r.is_on_int_stack()); }
			else System.out.println("OpAssignNode.generate_my_code() _r is null!");

			// put subscript
			subscr.generate_code(c,s);
			move_between_stacks(c, subscr.is_on_int_stack());

			c.emitCall(11,2); // Method number 11, 2 parameters
			// NB! Must return copy of assigned stuff! NB! Must increment refcount!
			// (currently does, make sure it will)
			if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");

			PhantomType destType = new PhantomType( atom.getType().get_class() );

			//check_assignment_types("container element", type, _r.getType());
			check_assignment_types("container element", destType, _r.getType());
		}
		else
			throw new PlcException("= Node", "unknown left Node", _l.toString() );

	}

}
