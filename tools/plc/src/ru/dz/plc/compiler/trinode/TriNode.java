package ru.dz.plc.compiler.trinode;

import java.io.IOException;
import java.io.PrintStream;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.binode.BiNode;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Three children abstract node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

abstract public class TriNode extends BiNode {
	Node _m;

	public TriNode(Node l, Node m, Node r) {
		super(l,r);
		this._m = m;
	}

	public void print( PrintStream ps, int level, int start_level ) throws PlcException
	{
		print_offset( ps, level, start_level );
		//System.out.println(toString());
		print_me(ps);
		if( _l != null )     _l.print(ps, level+1, _m != null ? start_level : start_level+1 );
		if( _m != null )     _m.print(ps, level+1, _r != null ? start_level : start_level+1 );
		if( _r != null )     _r.print(ps, level+1, start_level+1 );
	}

	public boolean is_const()
	{
		return
		_l != null && _l.is_const() &&
		_m != null && _m.is_const() &&
		_r != null && _r.is_const()
		;
	}

	public void preprocess( ParseState s ) throws PlcException
	{
		if(_l != null) _l.preprocess(s);
		if(_m != null) _m.preprocess(s);
		if(_r != null) _r.preprocess(s);
		preprocess_me(s);
	}

	public void preprocess_me( ParseState s ) throws PlcException {}

	public void propagateVoidParents()
	{
		if( _l != null )
		{
			//_l.setParentIsVoid();
			_l.propagateVoidParents();
		}
		
		if( _m != null )
		{
			//_m.setParentIsVoid();
			_m.propagateVoidParents();
		}
		
		if( _r != null )
		{
			//_r.setParentIsVoid();
			_r.propagateVoidParents();
		}
	}
	
	
	// NB! Move between stacks is not done automatically for tri-nodes, do it manually!
	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		if(context !=null)			c.emitComment("Line "+context.getLineNumber());

		// All of 3-way nodes do it some special way, no general approach exist
		generate_my_code(c,s);
	}

}


// ------------------------------------------------------------------------
// Assert
// ------------------------------------------------------------------------



class assert_node extends TriNode {
	public assert_node(Node a, Node b, Node to_throw ) {
		super(a, b, to_throw );
	}
	public String toString()  {    return "assert";  }
	public PhantomType find_out_my_type()
	{
		return PhantomType.getVoid();
	}
}



