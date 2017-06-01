package ru.dz.plc.compiler.binode;

import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;

/**
 * <p>Generic value compare node.</p>
 * <p>Copyright: Copyright (c) 2004-2017 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public abstract class ValCmpNode extends BiBistackNode {

	protected String opName = "__undefined_in_subclass!!__";
	
	public ValCmpNode(Node l, Node r) {
		super(l, r);
	}

	@Override
	public void generate_C_code(C_codegen cgen, CodeGeneratorState s) throws PlcException {

		// TODO move between stacks?
		
		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"valueCmp_"+opName+"( " );
		_l.generate_C_code(cgen, s);
		cgen.put(" ) "); 
		_r.generate_C_code(cgen, s);	
		cgen.put(" ) "); 
	}
	
}