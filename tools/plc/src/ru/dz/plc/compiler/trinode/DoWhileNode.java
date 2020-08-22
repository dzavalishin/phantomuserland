package ru.dz.plc.compiler.trinode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>General loop node. Implements do and while loops.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class DoWhileNode extends TriNode {
	
	public DoWhileNode(Node preop, Node value, Node operator) {
		super(preop, value, operator);
	}
	
	public String toString()  {    return "do-while";  }
	
	public PhantomType find_out_my_type()
	{
		return PhantomType.getVoid();
	}

	public boolean is_on_int_stack() { return false; }

	// NB! Move between stacks is not done automatically for tri-nodes, do it manually!

	public void propagateVoidParents()
	{
		if( _l != null )
		{
			_l.setParentIsVoid();
			_l.propagateVoidParents();
		}
		
		//_m.setParentIsVoid();
		_m.propagateVoidParents();
		
		if( _r != null )
		{
			_r.setParentIsVoid();
			_r.propagateVoidParents();
		}
	}
	
	
	@Override
	public void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		if( _m == null ) throw new PlcException("while code", "no expression");

		String label_continue = c.getLabel(), label_break = c.getLabel();

		String prev_continue = s.continue_label;
		String prev_break = s.break_label;

		s.continue_label = label_continue;
		s.break_label = label_break;

		c.markLabel(label_continue);
		if( _l != null ) _l.generate_code(c,s); // pre code

		_m.generate_code(c,s); // calc value
		// I need it on int stack!
		if( !_m.is_on_int_stack() ) c.emit_o2i();
		if( !_m.getType().is_int() )
		{
			//System.out.println("Warning: not an integer expression in while ");
			print_warning("not an integer expression in while: "+ _m.getType().toString());
			// TODO error?!
		}

		c.emitJz(label_break);
		if( _r != null ) _r.generate_code(c,s); // post code
		c.emitJmp(label_continue);

		c.markLabel(label_break);

		s.continue_label = prev_continue;
		s.break_label = prev_break;
	}

	@Override
	public void generate_C_code (C_codegen cgen, CodeGeneratorState s) throws PlcException
	{
		if( _m == null ) throw new PlcException("while code", "no expression");

		// TODO need CodeGeneratorState s
		
		String label_continue = cgen.getLabel(), label_break = cgen.getLabel();

		String prev_continue = s.continue_label;
		String prev_break = s.break_label;

		s.continue_label = label_continue;
		s.break_label = label_break;

		cgen.markLabel(label_continue);
		if( _l != null ) _l.generate_C_code(cgen,s); // pre code

		cgen.emitIfNot(_m, s);
		cgen.emitJump(label_break);

		if( _r != null ) _r.generate_C_code(cgen,s); // post code

		cgen.emitSnapShotTrigger();
		
		cgen.emitJump(label_continue);

		cgen.markLabel(label_break);

		s.continue_label = prev_continue;
		s.break_label = prev_break;
	}

}
