package ru.dz.jpc.tophantom.node.binode;

import ru.dz.plc.compiler.binode.OpAssignNode;
import ru.dz.plc.compiler.binode.OpSubscriptNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.compiler.node.IdentNode;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomField;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;
import ru.dz.phantom.code.Codegen;

import java.io.IOException;

/**
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * Date: 19.10.2009 22:03:57
 *
 * @author Irek
 */
public class OpAssignTransNode extends OpAssignNode {
    public OpAssignTransNode(Node var_or_array, Node expr) {
        super(var_or_array, expr);
    }

    protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException,
                PlcException {
		if( _l instanceof IdentNode) {
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
//				c.emitOsDup(); // return a copy
				c.emitSave(f.get_ordinal());
				if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
				return;
			}

			// Stack var?
			PhantomStackVar svar = s.istack_vars().get_var(dest_name);
			if (svar != null)
			{
				if (type == null || type.is_unknown()) type = svar.getType();
				check_assignment_types(svar.getName(), type,_r.getType());
//				c.emitIsDup(); // return a copy
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
//				c.emitOsDup(); // return a copy
				c.emitSet(svar.get_abs_stack_pos()); // set stack variable
				if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
			}
		}
//		else if( _l.getClass() == OpSubscriptNode.class )
//		{
//			// this is assignment to array element
//			OpSubscriptNode dest = (OpSubscriptNode)_l;
//
//			Node atom = dest.getLeft();
//			Node subscr = dest._r;
//
//			// array object to assign to
//			atom.generate_code(c,s);
//			move_between_stacks(c, atom.is_on_int_stack());
//
//			// put value to assign
//			if( _r != null ) { _r.generate_code(c,s); move_between_stacks(c, _r.is_on_int_stack()); }
//			else System.out.println("OpAssignNode.generate_my_code() _r is null!");
//
//			// put subscript
//			subscr.generate_code(c,s);
//			move_between_stacks(c, subscr.is_on_int_stack());
//
//			c.emitCall(11,2); // Method number 11, 2 parameters
//			// NB! Must return copy of assigned stuff! NB! Must increment refcount!
//			// (currently does, make sure it will)
//			if(is_on_int_stack()) System.out.println("OpAssignNode.generate_my_code() i'm on int??!");
//
//			PhantomType destType = new PhantomType( atom.getType().get_class() );
//
//			//check_assignment_types("container element", type, _r.getType());
//			check_assignment_types("container element", destType, _r.getType());
//		}
		else
			throw new PlcException("= Node", "unknown left Node", _l.toString() );

	}
}
