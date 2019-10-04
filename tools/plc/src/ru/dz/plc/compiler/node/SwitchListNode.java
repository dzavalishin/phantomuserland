package ru.dz.plc.compiler.node;

import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedList;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * 
 * Switch with list of ifs.
 * 
 * @author dz
 *
 */

public class SwitchListNode extends Node {

	private LinkedList<String> caseLabels = new LinkedList<String>();
	private LinkedList<Integer> caseValues = new LinkedList<Integer>();
	private String defaultLabel = null;

	private Node expr = null;

	public SwitchListNode(Node expr) {
		super(null);
		this.expr = expr;
	}

	@Override
	public String toString() {		return "switch list";	}
	public PhantomType find_out_my_type() { return PhantomType.getVoid(); }
	public boolean is_const() { return true; }
	public boolean args_on_int_stack() { return true; }

	public void addCase( String c, int value ) { caseLabels.add(c); caseValues.add(value); }
	
	public void addDefault( String c ) throws PlcException {
		if( defaultLabel != null )
			throw new PlcException(toString(),"second default");

		defaultLabel = c;
	}


	public void preprocess_me( ParseState s ) throws PlcException
	{
		expr.preprocess(s);
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		if( expr == null ) throw new PlcException(toString(),"no expression to switch on");
		if( !expr.getType().is_int() ) throw new PlcException(toString(),"not an integer expression");

		expr.generate_code(c, s);
		move_between_stacks(c, expr);

		generate_my_code(c,s);
		
		/*
		String break_label = c.getLabel();
		String prev_break = s.break_label;
		s.break_label = break_label;

		if( _l != null ) _l.generate_code(c, s);

		c.markLabel(break_label);
		s.break_label = prev_break;
 		*/		
	}


	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		Iterator<Integer> vi = caseValues.iterator();
		Iterator<String> i = caseLabels.iterator();
		while( i.hasNext() && vi.hasNext() )
		{
			c.emitIsDup();
			String caseLabel = i.next();
			int caseValue = vi.next();
			
			c.emitIConst_32bit(caseValue);			
			c.emitISubLU();
			c.emitJz(caseLabel);
		}
		c.emitIsDrop();
		
		//if(defaultLabel != null) 
		c.emitJmp(defaultLabel);
		//else c.emitJmp(s.break_label);
	}
	
	

}