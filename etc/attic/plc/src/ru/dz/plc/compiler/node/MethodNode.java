package ru.dz.plc.compiler.node;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Method definition node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class MethodNode extends Node {
	String ident;
	int    num;

	public int get_ordinal(PhantomType obj_type) throws PlcException {
		if( num >= 0 )
			return num;

		if( obj_type.is_unknown() )
			throw new PlcException("MethodNode", "don't know class", ident);

		PhantomClass pc = obj_type.get_class();
		if( pc == null )
			throw new PlcException("MethodNode", "class is null", ident);

		//Method m = pc.mt.get(ident);
		Method m = pc.findMethod(ident);

		//pc.mt.set_ordinals();
		pc.set_ordinals();

		if(m.ordinal < 0)      throw new PlcException("MethodNode","don't know Method number for class "+pc.toString(), ident);

		return m.ordinal;
	}

	public PhantomType get_return_type(PhantomType obj_type) throws PlcException {
		if( num >= 0 ) return new PhTypeUnknown();

		if( obj_type.is_unknown() ) throw new PlcException("MethodNode", "don't know class", ident);

		PhantomClass pc = obj_type.get_class();
		if( pc == null )throw new PlcException("MethodNode", "class is null", ident );

		//Method m = pc.mt.get(ident);
		Method m = pc.findMethod(ident);
		if( m == null )
			throw new PlcException("MethodNode", "Method of "+pc.toString()+" is null", ident );

		return m.type;
	}


	public MethodNode( String ident ) {
		super(null);
		this.ident = ident;
		num = -1;
	}

	public MethodNode( int meth_no ) {
		super(null);
		this.ident = null;
		num = meth_no;
	}

	// BUG?
	public void find_out_my_type() { type = new PhTypeUnknown(); }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	public String toString()
	{
		return "Method "+ ((ident == null) ? "(no. "+Integer.toString(num)+")" : ident );
	}
}
