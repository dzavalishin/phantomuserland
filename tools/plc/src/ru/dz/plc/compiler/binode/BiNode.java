package ru.dz.plc.compiler.binode;

import java.io.IOException;
import java.io.PrintStream;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.CastNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Title: Two children abstract base node.</p>
 * 
 * <p>Description: mostly used as a base for binary operations (math).</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */

abstract public class BiNode extends Node {
	protected Node _r;

	public BiNode(Node l, Node r) {
		super(l);
		this._r = r;
	}

	public Node getRight() { return _r; }

	public void print( PrintStream ps, int level, int start_level ) throws PlcException
	{
		//if(type == null) find_out_my_type();
		print_offset( ps, level, start_level );
		//System.out.println(toString());
		print_me(ps);
		print_children(ps, level, start_level);
	}

	protected void print_children(PrintStream ps, int level, int start_level) throws PlcException  {
		if( _l != null )     _l.print(ps, level+1, _r != null ? start_level : start_level+1 );
		if( _r != null )     _r.print(ps, level+1, start_level+1 );
	}

	public boolean is_const()
	{
		return _l != null && _l.is_const() && _r != null && _r.is_const();
	}

	public PhantomType find_out_my_type() throws PlcException
	{
		PhantomType l_type = null, r_type = null;

		if( _l != null ) l_type = _l.getType();
		if( _r != null ) r_type = _r.getType();

		if( l_type != null && l_type.equals( r_type ) ) return l_type;
		//TODO hack, need to remove and refactor [
		else if( l_type != null &&  r_type instanceof PhTypeUnknown ) return l_type;
		//todo ]
		else return new PhTypeUnknown();
	}

	public void preprocess( ParseState s ) throws PlcException
	{
		if(_l != null) _l.preprocess(s);
		if(_r != null) _r.preprocess(s);
		preprocess_me(s);
	}

	public void preprocess_me( ParseState s ) throws PlcException {}

	/** Override in class if you want to tell children that you don't need their results. */
	public void propagateVoidParents()
	{
		if( _l != null )
		{
			//_l.setParentIsVoid();
			_l.propagateVoidParents();
		}
		
		if( _r != null )
		{
			//_r.setParentIsVoid();
			_r.propagateVoidParents();
		}
	}
	
	
	
	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		if( _l != null ) 
		{ 
			_l.generate_code(c,s); 
			move_between_stacks(c, _l.is_on_int_stack(), _l.getType());
		}
		if( _r != null ) 
		{ 
			_r.generate_code(c,s); 
			move_between_stacks(c, _r.is_on_int_stack(), _r.getType()); 
		}

		log.fine("Node "+this+" codegen");

		if(context != null)			c.emitComment("Line "+context.getLineNumber());
		generate_my_code(c,s);
	}


	@Override
	public void generateLlvmCode(LlvmCodegen llc) throws PlcException {
		llvmTempName = llc.getPhantomMethod().getLlvmTempName(this.getClass().getSimpleName());

		if( _l != null ) { _l.generateLlvmCode(llc); }
		if( _r != null ) { _r.generateLlvmCode(llc); }

		log.fine("Node "+this+" codegen");

		if(context != null)			llc.emitComment("Line "+context.getLineNumber());
		generateMyLlvmCode(llc);
	}

	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException {
		cTempName = cgen.getPhantomMethod().get_C_TempName(this.getClass().getSimpleName());

		if(context != null)			cgen.emitComment("Line "+context.getLineNumber());

		// Generate left child, then me, then right child
		// For example, l = const 5, r = read var, me = +
		// Result will be ((5) + (var))
		if( _l != null ) { _l.generate_C_code(cgen,s); }
		generateMy_C_Code(cgen);
		if( _r != null ) { _r.generate_C_code(cgen,s); }

		log.fine("Node "+this+" codegen");

	}

	// Used by compare nodes
	protected void generate_cmp_C_code(C_codegen cgen, CodeGeneratorState s, String opName) throws PlcException 
	{
		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"valueCmp_"+opName+"( " );
		_l.generate_C_code(cgen, s);
		cgen.put(" ) "); 
		_r.generate_C_code(cgen, s);	
		cgen.put(" ) "); 
	}


}


// -------------------------------- real nodes ------------------------------





/**
 * This node can generate code to any stack, depending on children results.
 * Result will be on object stack if any of children is returning value on object
 * stack.
 */
abstract class BiBistackNode extends BiNode {
	//boolean go_to_object_stack = false;
	public boolean args_on_int_stack() { return !go_to_object_stack(); }

	public BiBistackNode( Node l, Node r)
	{
		super(l,r);

	}

	public boolean go_to_object_stack() 
	{
		return (!_l.is_on_int_stack()) || (!_r.is_on_int_stack()); 
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		_l.generate_code(c, s);
		if (go_to_object_stack() && _l.is_on_int_stack()) 
		{
			//c.emit_i2o();
			move_between_stacks(c, _l.is_on_int_stack(), _l.getType());
		}
		
		_r.generate_code(c, s);
		if (go_to_object_stack() && _r.is_on_int_stack())
		{
			//c.emit_i2o();
			move_between_stacks(c, _r.is_on_int_stack(), _r.getType());
		}
		
		log.fine("Node "+this+" codegen");
		generate_my_code(c,s);
	}


	protected PhantomType common_type;
	
	@Override
	public void preprocess_me(ParseState s) throws PlcException {
		super.preprocess_me(s);

		common_type = PhantomType.findCompatibleType(_l.getType(),_r.getType());
		if( common_type == null )
		{
			print_error(String.format("types %s and %s are incompatible", 
					_l.getType().toString(),
					_r.getType().toString()
					));
			throw new PlcException("bibistack find type "+context.get_position(),"types are not compatible");
		}
		
		if( !_l.getType().equals(common_type) ) 
			_l = new CastNode(_l, common_type);

		if( !_r.getType().equals(common_type) ) 
			_r = new CastNode(_r, common_type);
		
	}
}






// ------------------------------------------------------------------------

/* TODO       check_assignment_types("container element", type, _r.getType()); */



