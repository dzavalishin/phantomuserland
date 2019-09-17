package ru.dz.plc.compiler.node;

import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeUnknown;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.util.PlcException;

/**
 * <p>Class def node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */

public class ClassDefinitionNode extends Node {
	PhantomClass my_class;
	public ClassDefinitionNode( PhantomClass my_class, Node expr ) {
		super(expr);
		this.my_class = my_class;
	}
	public String toString()  {    return "class def ("+my_class.toString()+")";  }
	public void find_out_my_type() { type = new PhTypeUnknown(); }
	public void preprocess_me(ParseState ps) throws PlcException
	{
		my_class.preprocess(ps);
	}
}

