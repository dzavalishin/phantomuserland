package ru.dz.plc.compiler.llvm;

import ru.dz.plc.compiler.LlvmCodegen;

/**
 * Helper to generate LLVM crazy code for string constants
 * @author dz
 *
 */
public class LlvmStringConstant 
{
	private final LlvmCodegen llc;
	private final String tmp;
	private final String tmps;
	private final String lens;
	private final String value;

	public LlvmStringConstant(LlvmCodegen llc, String value) {
		this.llc = llc;
		this.value = value;
		tmp = llc.getPhantomMethod().getLlvmTempName("StringConst");
		tmps = tmp.replaceAll("%", "Def");
		lens = Integer.toString(value.length()+1);
	}

	/** tmp name to refer to this string const as i8* */
	public String getReference()
	{
		return tmp;
	}
	
	public String getDef()
	{
		// TODO process value to eliminate controls.
		return "@."+tmps+" = private unnamed_addr constant ["+lens+" x i8] c\""+value+"\\00\"; \n";
	}
	public String getCast()
	{
		return tmp+" = getelementptr ["+lens+" x i8]* @."+tmps+", i64 0, i64 0";
	}
	
}
