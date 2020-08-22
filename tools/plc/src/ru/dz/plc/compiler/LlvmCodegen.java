package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.IOException;

public class LlvmCodegen {

	private PhantomClass pc;
	private Method m;
	private BufferedWriter llvmFile;

	public LlvmCodegen(PhantomClass pc, Method m, BufferedWriter llvmFile2) {
		this.pc = pc;
		this.m = m;
		//this.llvmFile = new BufferedWriter(llvmFile2);
		this.llvmFile = llvmFile2;
	}


	public void put(String string) {
		//System.out.println(string);		
		try {
			llvmFile.write(string);
		} catch (IOException e) {
			throw new RuntimeException(e);
			//e.printStackTrace();
		}
	}

	public void putln(String string) {
		put(string+"\n");		
	}
	
	
	private void callRuntime1(String funcName, int b) {
		putln("call void @PhantomVm_"+funcName+"("+b+");");
		
	}
	
	
	
	public void emitDebug(byte b) {
		callRuntime1( "debug", (int)b );		
	}


	
	

	public void emitComment(String string) {
		putln("; "+string);		
	}



	public void recordLineNumberToIPMapping(int lineNumber) {
		// Not needed?
		
	}





	public PhantomStack getIstackVars() { return m.isvars; }
	public PhantomStack GetOstackVars() { return m.svars; }


	public PhantomClass getPhantomClass() { return pc; }
	public  Method getPhantomMethod() { return m; }


	/** Return LLVM type name for object ref */
	public static String getObjectType() { return "%OPTR"; }

	// ------------------------------------------
	// Part of code is generated to the buffer to 
	// be emitted after the method code. 
	// ------------------------------------------
	
	private StringBuilder postponed = new StringBuilder();

	public void postponeCode(String code)
	{
		postponed.append(code);
	}
	
	public void flushPostponedCode() {
		put(postponed.toString());		
	}











}
