package ru.dz.plc.compiler.node;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.C_codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhTypeString;
import ru.dz.plc.compiler.PhantomClass;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.util.PlcException;

/**
 * Generate string const in constant pool. 
 *
 * @author dz
 *
 */

public class StringConstPoolNode extends Node {

	private int id;
	private String val;

	public StringConstPoolNode(String val, PhantomClass c) {
		super(null);
		this.val = val;
		id = c.addStringConst(val);
	}

	public String toString()  {    return "string pool const id=\""+id+"\"";  }
	public PhantomType find_out_my_type() { return PhantomType.getString(); }
	
	public boolean is_const() { return true; }
	public void preprocess_me( ParseState s ) throws PlcException  {  }

	protected void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException {
		c.emitComment("str '"+val+"'");
		c.emitConstantPool(id);
	}

	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		//LlvmStringConstant ls = new LlvmStringConstant(llc, val);
		//llc.postponeCode(ls.getDef()+";\n");
		//llc.putln(ls.getCast());
		//llvmTempName = ls.getReference();
		llc.putln(llvmTempName+" = call "+LlvmCodegen.getObjectType()+" @PhantomVm_loadObjectFromPool( i4* "+id+" );");
	}
	
	@Override
	protected void generateMy_C_Code(C_codegen cgen) throws PlcException {

		cgen.put(C_codegen.getJitRuntimeFuncPrefix()+"LoadObjectFromPool( "+
		C_codegen.get_vm_state_var_name()+","+
		Integer.toString(id)
		+" )");
		
	}
	
}


