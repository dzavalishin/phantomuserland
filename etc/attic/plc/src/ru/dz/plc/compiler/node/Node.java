package ru.dz.plc.compiler.node;

import java.io.IOException;
import java.io.PrintStream;
import java.util.logging.Logger;

import ru.dz.phantom.code.*;
import ru.dz.plc.compiler.AttributeSet;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.parser.ParserContext;
import ru.dz.plc.util.*;

/**
 * <p>The most general node class.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

abstract public class Node {
	protected Logger log = Logger.getLogger(Node.class.getName());
	//{		log.setLevel(Level.SEVERE);	}
	
	protected Node           _l;
	protected PhantomType        type;
	protected AttributeSet  attributes;
	protected ParserContext context = null;

	public Node(Node l) {
		this._l = l;
		type = null;
	}
	
	/** Remember where this node was parsed in source file to be able to print error/warning later. */
	public Node setContext( ParserContext context ) { this.context = context; return this; }
	/** Remember where this node was parsed in source file to be able to print error/warning later. */
	public Node setContext( ru.dz.plc.parser.Lex l ) { this.context = new ParserContext(l); return this; }

	// -------------------------------- getters -------------------------------

	public Node getLeft() { return _l; }
	
	// -------------------------------- messages -------------------------------

	protected void print_warning( String w )
	{
		if( context != null ) System.out.print( context.get_position() );
		System.out.println("warning: "+w);
	}

	protected void print_error( String w )
	{
		if( context != null ) System.out.print( context.get_position() );
		System.out.println("error: "+w);
	}

	// -------------------------------- types ----------------------------------

	public PhantomType getType() throws PlcException
	{
		if( type == null || type.is_unknown() ) find_out_my_type();
		return type;
	}

	// TODO make abstract? Override!
	public void find_out_my_type() throws PlcException
	{
		if( type != null ) return;
		if( _l != null ) type = _l.getType();
	}


	// -------------------------------- atttributes ----------------------------

	public AttributeSet get_attributes() throws PlcException {
		if( attributes == null ) find_out_my_attrs();
		if( attributes == null )
			throw new PlcException("get_attributes","can't find out my attributes");
		return attributes;
	}

	// Override!
	void find_out_my_attrs() throws PlcException
	{
		throw new PlcException("find_out_my_attrs","not overriden",toString());
	}

	// -------------------------------- general --------------------------------

	public abstract String toString();
	public void print(PrintStream ps) throws PlcException { print(ps,0,0); }

	public void print( PrintStream ps, int level, int start_level ) throws PlcException
	{
		print_offset( ps, level, start_level );
		print_me(ps);
		if( _l != null )     _l.print(ps, level+1, start_level+1 );
		//if( _r != null )     _r.print(level+1, start_level+1 );
	}

	protected void print_me(PrintStream ps ) throws PlcException {
		ps.print(toString());
		if( type == null ) find_out_my_type();
		if( type != null ) ps.print(" : " + type.toString()+"");
		if( is_const()   ) ps.print(" const");
		if( attributes != null ) ps.print( " @"+attributes.toString() );
		ps.println();
	}

	public void print_offset( PrintStream ps, int level, int start_level )
	{
		while( level-- > 0 )
		{
			/*if( level > start_level )        System.out.print("| ");
      else*/        ps.print("  ");
		}
	}

	// ------------------------------ preprocessing ----------------------------

	/**
	 * preprocessing - must be called after everything is parsed
	 * and before any printing, optimization and code generation
	 * 
	 * this Method must:
	 * 
	 * - find out data type of the Node
	 * - check if everything is ok
	 * 
	 */

	public void preprocess( ParseState s ) throws PlcException
	{
		if(_l != null) _l.preprocess(s);
		preprocess_me(s);
	}

	// TODO abstract?
	public void preprocess_me( ParseState s ) throws PlcException
	{
		System.out.println("Not-overriden preprocess called: "+toString());
	}


	// ------------------------------ optimizations ----------------------------

	Node evaluate_const_expr() throws PlcException { throw new PlcException( "Node evaluate_const_expr", "not redefined for this Node", toString() ); }
	public boolean is_const() { return _l != null && _l.is_const(); }


	// ---------------------------- code generation ----------------------------

	// result will be on int stack by default?
	public boolean is_on_int_stack() { return false; }
	public boolean args_on_int_stack() { return is_on_int_stack(); }

	protected void move_between_stacks(Codegen c, boolean fromint ) throws IOException {
		if( args_on_int_stack() && (!fromint) ) c.emit_o2i();
		if( (!args_on_int_stack()) && fromint ) c.emit_i2o();
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		if( _l != null ) {
			_l.generate_code(c, s);
			move_between_stacks(c, _l.is_on_int_stack());
		}
		if(context != null)
		{
			c.emitComment("Line "+context.getLineNumber());
			c.recordLineNumberToIPMapping(context.getLineNumber());
		}
		log.fine("Node "+this+" codegen");
		generate_my_code(c,s);
	}

	// TODO abstract?
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		System.out.println("Not-overriden generate_my_code called: "+toString());
	}

	// -------------------------------- type checks -----------------------------

	public void check_assignment_types(String name, PhantomType dest, PhantomType src) throws PlcException {
		if( src == null || src.is_unknown() )
			print_warning("Assignment to "+name+": source type is unknown");
		if( dest == null || dest.is_unknown() )
			print_warning("Assignment to "+name+": destination type is unknown");
		else if( !dest.can_be_assigned_from(src) )
			print_warning("Assignment to "+name+": incompatible source type");
	}


}



// --------------------------------------------------------------------------
// -------------------------------- abstract nodes --------------------------
// --------------------------------------------------------------------------





abstract class SwitchLabelNode extends Node {
	String my_label = null;

	public String get_label() { return my_label; }

	public boolean is_const() { return false; }
	public void find_out_my_type() { if( type == null ) type = new PhTypeVoid(); }

	protected SwitchLabelNode( String label, Node expr ) { super(expr); this.my_label = label; }

	public void preprocess_me( ParseState s ) throws PlcException	{	}

	/* Wrong: parent automatically calcs our _l, which is not needed here, see SwitchNode
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		c.markLabel(my_label);
	}*/

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		c.markLabel(my_label);
		c.emitIsDrop(); // We have a copy of switch expression os stack - drop it
	}
	
};




