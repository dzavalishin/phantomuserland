package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.jpc.python.RegisterNodeWrapper;
import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.PhantomStackVar;
import ru.dz.plc.util.PlcException;

/**
 * <p>Use one result many times node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


//This one is for compiling Python register-based internal representation
//Not yet finished
public class MultiuseNode extends Node {

	private final RegisterNodeWrapper myWrapper;
	private boolean firstTime = true;

	public MultiuseNode(Node l, RegisterNodeWrapper myWrapper) {
		super(l);
		this.myWrapper = myWrapper;
	}

	@Override
	public String toString() { return "Multiused "+_l.toString(); }

	@Override
	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		generate_my_code(c,s);
	}

	static int tempNo = 0;
	private PhantomStackVar tempVar;

	@Override
	protected void generate_my_code(Codegen c, CodeGeneratorState s)
			throws IOException, PlcException {

		if(myWrapper.getUseCount() != 0 )
			System.out.println("MultiuseNode.generate_my_code() use count == 0");
		
		boolean multiuse = myWrapper.getUseCount() > 1;
		
		if(multiuse)
		{
			// Top of stack will be used more than once. On the first use 
			// we will produce it (see above) and put to var. Later times will just 
			// get from var.
			
			if(firstTime)
			{
				super.generate_code(c, s);
				firstTime = false; 
				
				tempVar = s.getMethod().svars.get_var("multiuse_temp_"+(tempNo++));
				c.emitOsDup();
				c.emitSet(tempVar.get_abs_stack_pos()); // set var
				// result copy is on stack here
			}
			else
			{
				c.emitGet(tempVar.get_abs_stack_pos()); // get stack variable
			}
			
		}
		else
		{
			// No special code needed, top of stack is result
			// of usual _l execution
			super.generate_code(c, s);
		}
	}
	
}
