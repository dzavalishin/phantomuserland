package ru.dz.plc.compiler.node;

import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedList;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Switch node.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */


public class SwitchNode extends Node {
	LinkedList<SwitchCaseNode> cases = new LinkedList<SwitchCaseNode>();
	SwitchDefaultNode my_default = null;
	Node expr = null;

	public SwitchNode(Node expr) {
		super(null);
		this.expr = expr;
	}

	
	
	public String toString()  {    return "switch "; /*+ident;*/  }
	public PhantomType find_out_my_type() { return PhantomType.getVoid(); }
	public boolean is_const() { return true; }

	public void add_case( SwitchCaseNode c ) { cases.add(c); }
	public void add_default( SwitchDefaultNode c ) throws PlcException {
		if( my_default != null )
			throw new PlcException("switch_node","second default");

		my_default = c;
	}

	public void add_code( Node code ) { _l = code; }

	public boolean args_on_int_stack() { return true; }

	public void preprocess_me( ParseState s ) throws PlcException
	{
		expr.preprocess(s);
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		if( expr == null ) throw new PlcException("switch","no expression to switch on");

		//if( !expr.getType().is_int() ) throw new PlcException("switch","not an integer expression");
		if( !expr.getType().is_int() ) 
			print_warning("switch: not an integer expression");

		expr.generate_code(c, s);
		move_between_stacks(c, expr);

		generate_my_code(c,s);

		String break_label = c.getLabel();
		String prev_break = s.break_label;
		s.break_label = break_label;

		if( _l != null ) _l.generate_code(c, s); // ? _l is allways null?

		c.markLabel(break_label);
		s.break_label = prev_break;
	}


	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		for( Iterator<SwitchCaseNode> i = cases.iterator(); i.hasNext(); )
		{
			c.emitIsDup();
			SwitchCaseNode case_node = (SwitchCaseNode)i.next();
			Node case_expr = case_node._l;
			case_expr.generate_code(c,s);
			move_between_stacks(c, case_expr);
			c.emitISubLU();
			c.emitJz(case_node.get_label());
		}
		//c.emitIsDrop(); - will be dropped by SwitchDefaultNode code
		if(my_default != null) c.emitJmp(my_default.get_label());
		else c.emitJmp(s.break_label);
	}

}
