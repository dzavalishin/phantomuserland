package ru.dz.plc.compiler.binode;

import java.io.IOException;

import ru.dz.phantom.code.Codegen;
import ru.dz.plc.compiler.CodeGeneratorState;
import ru.dz.plc.compiler.LlvmCodegen;
import ru.dz.plc.compiler.Method;
import ru.dz.plc.compiler.ParseState;
import ru.dz.plc.compiler.PhantomType;
import ru.dz.plc.compiler.llvm.LlvmStringConstant;
import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;
import ru.dz.soot.SootMain;

/**
 * <p>Dynamic method call node.</p>
 * 
 * <p>Copyright: Copyright (c) 2004-2009 Dmitry Zavalishin</p>
 * 
 * <p>Company: <a href="http://dz.ru/en">Digital Zone</a></p>
 * 
 * @author dz
 */


public class OpDynamicMethodCallNode extends BiNode {
	PhantomType obj_type = null; // type of object, which is used as 'this' in call
	private String methodName;

	public OpDynamicMethodCallNode(Node object, String methodName, Node args) { super(object, args);
	this.methodName = methodName; }
	public String toString()  {    return ".dynamic_call."+methodName;  }

	@Override
	public boolean args_on_int_stack() {
		return super.args_on_int_stack();
	}

	public void preprocess_me( ParseState s ) throws PlcException
	{
	}

	public void generate_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{
		generate_my_code(c,s);
	}

	public PhantomType find_out_my_type() throws PlcException {
		//if( getType() == null ) 
		throw new PlcException("Dynamic method call Node","return type is not set");
	}

	public void generate_my_code(Codegen c, CodeGeneratorState s) throws IOException, PlcException
	{

		int n_param = 0;
		// bug - wrong count of args?
		for( Node i = _r; i != null; i = ((BiNode)i).getRight() )      
			n_param++;

		if(n_param > 1024)
			SootMain.warning("too many params in dyncall: "+n_param);
		
		if( _r != null ) _r.generate_code(c,s); // calc args
		c.emitIConst_32bit(n_param); // n args
		
		_l.generate_code(c,s); // get object
		move_between_stacks(c, _l.is_on_int_stack(), _l.getType());

		c.emitString(methodName);
		
		c.emitDynamicCall();
	}

	/**
	 * The idea is to call special dynlink proxy func, which will set up
	 * data for dynlinker and will be replaced then with call to real method.
	 * Finally dynlinker will reset stack to previous frame's state and jump 
	 * (return) to real method.
	 */
	@Override
	protected void generateMyLlvmCode(LlvmCodegen llc) throws PlcException {
		String proxyArgdef = "";
		String proxyName = methodName;

		proxyName += "_"+llc.getPhantomMethod().getLlvmTempName("dyncall");
		proxyName = proxyName.replaceAll(Method.CONSTRUCTOR_M_NAME, "\\$Constructor");
		proxyName = proxyName.replaceAll("%", "");
		
		boolean first = true;
		for( Node i = _r; i != null; i = ((BiNode)i).getRight() )      
		{
			if( !first ) proxyArgdef += ", ";
			first = false;
			
			proxyArgdef += i.getLlvmType();
			proxyName += ("_"+i.getType().toProxyName());
		}
		
		LlvmStringConstant ls_method = new LlvmStringConstant(llc, proxyName); 
		LlvmStringConstant ls_type = new LlvmStringConstant(llc, obj_type == null ? "void" : obj_type.toString());
		
		llc.postponeCode("\n; dynlink proxy\n");
		llc.postponeCode(ls_method.getDef()+";\n");
		llc.postponeCode(ls_type.getDef()+";\n");

		// We need to do strange things with frame, so don't think about inlining!
		llc.postponeCode(String.format("define %s @%s(%s) noinline {\n", LlvmCodegen.getObjectType(), proxyName, proxyArgdef )); // proxy to be called

		llc.postponeCode(ls_method.getCast()+";\n");
		llc.postponeCode(ls_type.getCast()+";\n");
		
		llc.postponeCode("%stackPos  = call i8* @llvm.stacksave(); @llvm.stackrestore(i8* %ptr)\n");
		//llc.postponeCode("%frameAddr = call i8* @llvm.frameaddress( i32 0 );\n");
		
		//llc.postponeCode("call void @PhantomVM_DynamicLinker( i8* "+castTmp+" );\n");
		llc.postponeCode("call void @PhantomVM_DynamicLinker( i8* "+ls_method.getReference()+", i8* "+ls_type.getReference()+", i8* %stackPos );\n");
		
		llc.postponeCode("ret %OPTR <{ i8* null, i8* null }> ;\n");
		llc.postponeCode("}\n");

		llc.put("; dyn call: ");
		llc.put(llvmTempName+" = call "+LlvmCodegen.getObjectType()+" "+proxyName+"(");
		
		first = true;
		for( Node i = _r; i != null; i = ((BiNode)i).getRight() )      
			{
			if( !first ) llc.put(", ");
			first = false;
			
			//ArgDefinitionNode a = (ArgDefinitionNode) i;
			llc.put(i.getType().toLlvmType()+" "+i.getLlvmTempName());
			}
		
		llc.putln(");");
	}
	
	/*
	@Override
	protected void print_me(PrintStream ps) throws PlcException {
		ps.print(toString());
		if( type == null ) find_out_my_type();
		if( type != null ) ps.print(" : " + type.toString()+"");
		if( is_const()   ) ps.print(" const");
		if( attributes != null ) ps.print( " @"+attributes.toString() );
		ps.println();
	
	}
	*/
}
