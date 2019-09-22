package ru.dz.plc.compiler.binode;

import java.io.PrintStream;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/** 
 * <p>Just order of execution.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */

public class SequenceNode extends BiNode {
	public SequenceNode( Node l, Node r) {    super(l,r);  }
	public String toString()  {    return ", ";  }

	@Override
	public PhantomType find_out_my_type() throws PlcException
	{
		return PhantomType.getVoid();
	}	
	
	public void propagateVoidParents()
	{
		_l.setParentIsVoid();
		_l.propagateVoidParents();
		_r.setParentIsVoid();
		_r.propagateVoidParents();
	}
	
	@Override
	protected void generate_my_code(Codegen c, CodeGeneratorState s) {} // really nothing :)
	
	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {}
	
    @Override
    public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException
    {
    	if( _l != null ) {
    		//cgen.putln(";// l:"); 
    		_l.generate_C_code(cgen, s);    		
    	}
    	if( _r != null ) {
    		cgen.putln(";// r:");
    		_r.generate_C_code(cgen, s); 
    	}
    }

	protected void print_children(PrintStream ps, int level, int start_level) throws PlcException 
	{
		if( _l != null )     _l.print(ps, level+1, start_level+1 );
		if( _r != null )     _r.print(ps, level+1, start_level+1 );
	}
	
	protected void print_me() {}
	public void preprocess_me(ParseState ps) throws PlcException {}
}
