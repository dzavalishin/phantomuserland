package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeVoid;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * <p>Return node.</p>
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * @author dz
 */


public class ReturnNode extends Node {
	
	public ReturnNode( Node expr ) {
		super(expr);
	}
	public boolean args_on_int_stack() { return false; }
	public String toString()  {    return "return";  }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException {
		if( s.get_method().getDebugMethod() )
			c.emitDebug((byte)0x2,"Disabled debug");
		c.emitRet();
	}
	
	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		if( _l == null )
		{
			llc.putln("ret void;");			
			return;
		}
		
		//_l.generateLlvmCode(llc);
		llc.putln("ret "+LlvmCodegen.getObjectType()+" "+_l.getLlvmTempName()+" ;");
	}
	
	public PhantomType find_out_my_type()  {    return PhantomType.getVoid();  }

}
