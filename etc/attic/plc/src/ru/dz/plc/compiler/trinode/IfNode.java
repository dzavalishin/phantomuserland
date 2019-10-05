package ru.dz.plc.compiler.trinode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>If operator node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class IfNode extends TriNode {
	public IfNode(Node value, Node op_true, Node op_false) {
		super(value, op_true, op_false);
	}
	public String toString()  {    return "if";  }
	public void find_out_my_type() throws PlcException
	{
		if( type != null ) return;
		PhantomType l_type = null, r_type = null;

		if( _m != null ) l_type = _m.getType();
		if( _r != null ) r_type = _r.getType();

		if( l_type != null && l_type.equals( r_type ) ) type = l_type;
		else type = new PhTypeUnknown();
	}

	public boolean is_on_int_stack() { return false; }

	// NB! Move between stacks is not done automatically for tri-nodes, do it manually!

	public void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		if( _l == null ) throw new PlcException("if code", "no expression");
		_l.generate_code(c,s); // calc value

		// I need it on int stack!
		if( !_l.is_on_int_stack() ) c.emit_o2i();

		// To use if in expressions we need to find if results are int or not int
		// or void and thus eithe move them to one stack or do nothing

		//if( _l != null ) { _l.generate_code(c,s); move_between_stacks(c, _l.is_on_int_stack()); }
		//if( _r != null ) { _r.generate_code(c,s); move_between_stacks(c, _r.is_on_int_stack()); }


		if( !_l.getType().is_int() )
		{
			//System.out.println("Warning: not an integer expression in if: "+ _l.getType().toString());
			print_warning( "Warning: not an integer expression in if: "+ _l.getType().toString() );
			//c.emit_o2i();
		}

		String label_no = c.getLabel(), label_after = c.getLabel();

		c.emitJz(label_no);
		if( _m != null ) _m.generate_code(c,s); // 'yes' case
		if( _r != null )
			c.emitJmp(label_after);
		c.markLabel(label_no);

		if( _r != null )
		{
			_r.generate_code(c,s); // 'no' case
			c.markLabel(label_after);
		}
	}

}
