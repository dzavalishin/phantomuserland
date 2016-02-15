package ru.dz.plc.compiler.node;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.MethodSignature;
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


public class MethodNode extends Node 
{
	private String ident;
	private int    ordinal;
	private MethodSignature signature;

	public int get_ordinal(PhantomType obj_type) throws PlcException {
		if( ordinal >= 0 )
			return ordinal;

		if( obj_type.is_unknown() )
			throw new PlcException("MethodNode", "don't know class", ident);

		PhantomClass pc = obj_type.get_class();
		if( pc == null )
			throw new PlcException("MethodNode", "class is null", ident);

		Method m = findMethod(pc);

		pc.set_ordinals();

		if(m.getOrdinal() < 0)      throw new PlcException("MethodNode","don't know Method number for class "+pc.toString(), ident);

		return m.getOrdinal();
	}

	public PhantomType get_return_type(PhantomType obj_type) throws PlcException {
		if( ordinal >= 0 ) return new PhTypeUnknown();

		if( obj_type.is_unknown() ) throw new PlcException("MethodNode", "don't know class", ident);

		PhantomClass pc = obj_type.get_class();
		if( pc == null )throw new PlcException("MethodNode", "class is null", ident );

		Method m = findMethod(pc);

		return m.getType();
	}

	private Method findMethod(PhantomClass pc) throws PlcException {
		Method m = null;
		
		if( signature != null ) m = pc.findMethod(signature);
		if( m == null ) m = pc.findMethod(ident); // old way

		if( m == null )
			throw new PlcException("MethodNode", "Method of "+pc.toString()+" is null", ident );
		
		return m;
	}


	public MethodNode( String ident ) {
		super(null);
		this.ident = ident;
		ordinal = -1;
	}

	public MethodNode( int meth_no ) {
		super(null);
		this.ident = null;
		ordinal = meth_no;
	}

	// BUG?
	public void find_out_my_type() { type = new PhTypeUnknown(); }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	public String toString()
	{
		return "Method "+ ((ident == null) ? "(no. "+Integer.toString(ordinal)+")" : ident );
	}


	public void setSignature(MethodSignature sig) {
		this.signature = sig;
	}

	public String getIdent() {
		return ident;
	}
}
