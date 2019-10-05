package ru.dz.plc.compiler.node;

import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.MethodSignature;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Method definition node. Used as method call node child too.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */


public class MethodNode extends Node 
{
	private String ident;
	private int    ordinal;
	private MethodSignature signature;
	private ParseState ps;

	public int get_ordinal(PhantomType obj_type) throws PlcException {
		if( ordinal >= 0 )
			return ordinal;

		if( obj_type.is_unknown() )
			throw new PlcException("MethodNode get ordinal", "don't know class", ident);

		PhantomClass pc = obj_type.get_class();
		if( pc == null )
			throw new PlcException("MethodNode", "class is null", ident);

		Method m = findMethod(pc);

		// TODO hack!
		pc.set_ordinals();

		int ord = m.getOrdinal();
		
		if(ord < 0)
		{
			if( signature == null)
				throw new PlcException("MethodNode","don't know (no signature) Method number for class "+pc.toString(), ident);
			else
				throw new PlcException("MethodNode","don't know Method number for class "+pc.toString(), signature.toString());
		}

		return ord;
	}

	public PhantomType get_return_type(PhantomType obj_type) throws PlcException {
		if( ordinal >= 0 ) return new PhTypeUnknown();

		if( obj_type.is_unknown() ) 
		{
			throw new PlcException("MethodNode get return type", "don't know class", ident);
			//obj_type = PhantomType.getObject();
		}
		
		PhantomClass pc = obj_type.get_class();
		if( pc == null )throw new PlcException("MethodNode", "class is null", ident );

		Method m = findMethod(pc);

		return m.getType();
	}

	private Method findMethod(PhantomClass pc) throws PlcException {
		Method m = null;
		
		if( signature == null )
			print_warning("No signature for "+toString());
		
		if( signature != null ) m = pc.findMethod(signature);
		//if( m == null ) m = pc.findMethod(ident); // old way

		if( m == null )
		{
			print_error("Can't find method "+signature);
			throw new PlcException("MethodNode", "Method of "+pc.toString()+" is null", ident );
		}
		
		return m;
	}


	public MethodNode( String ident, ParseState ps ) {
		super(null);
		this.ident = ident;
		this.ps = ps;
		ordinal = -1;
	}

	public MethodNode( int meth_no, ParseState ps ) {
		super(null);
		this.ps = ps;
		this.ident = null;
		ordinal = meth_no;
	}

	public PhantomType find_out_my_type() { return PhantomType.getVoid(); }
	
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	public String toString()
	{
		return "Method "+ ((ident == null) ? "(no. "+Integer.toString(ordinal)+")" : ident );
	}


	public void setSignature(MethodSignature sig) {
		this.signature = sig;
		//ps.get_class().addMethod(this); //
	}

	public String getIdent() {
		return ident;
	}
}
