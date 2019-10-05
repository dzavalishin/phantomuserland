package ru.dz.plc.compiler.node;

import java.io.IOException;
import java.nio.charset.Charset;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeString;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.llvm.LlvmStringConstant;
import ru.dz.plc.util.PlcException;

/**
 * Replace with StringConstPoolNode
 * @author dz
 *
 */
@Deprecated
public class StringConstNode extends Node {
	private String val;
	
	public StringConstNode(String val) {
		super(null);
		this.val = val;
	}
	
	public String toString()  {    return "string const \""+val+"\"";  }

	public PhantomType find_out_my_type() { return PhantomType.getString(); }
	
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }
	
	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {
		c.emitString(val);
	}
	
	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		LlvmStringConstant ls = new LlvmStringConstant(llc, val);
		llc.postponeCode(ls.getDef()+";\n");
		llc.putln(ls.getCast());
		//llvmTempName = ls.getReference();
		llc.putln(llvmTempName+" = call "+LlvmCodegen.getObjectType()+" @PhantomVm_createStringObjefct( i8* "+ls.getReference()+" );");
	}
	
	public String getValue(){
		return val;
    }

	@Override
	protected void generateMy_C_Code(C_codegen cgen) throws PlcException {
		cgen.put( C_codegen.getJitRuntimeFuncPrefix()+ "CreateStringObjefct( \"");
		
		byte[] bytes = val.getBytes(Charset.forName("UTF8"));
		
		for (int i = 0; i < bytes.length; i++) {
			int c = ((int)bytes[i]) & 0xFF;
			if( c < ' ' || c > 'z' )
				cgen.put(String.format("\\x%02x", c ) );
			else
				cgen.put(String.format("%c", c ) );
		}
		
		cgen.put( "\" ) " );
	}
}
