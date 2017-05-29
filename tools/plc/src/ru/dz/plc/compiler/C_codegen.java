package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.IOException;


public class C_codegen {

		private PhantomClass pc;
		private Method m;
		private BufferedWriter c_File;

		public C_codegen(PhantomClass pc, Method m, BufferedWriter file) {
			this.pc = pc;
			this.m = m;
			//this.llvmFile = new BufferedWriter(llvmFile2);
			this.c_File = file;
		}


		public void put(String string) {
			//System.out.println(string);		
			try {
				c_File.write(string);
			} catch (IOException e) {
				throw new RuntimeException(e);
				//e.printStackTrace();
			}
		}

		public void putln(String string) {
			put(string+"\n");		
		}
		
		
		private void callRuntime1(String funcName, int b) {
			putln("call void JIT_PhantomVm_"+funcName+"("+b+");");
			
		}
		
		
		
		public void emitDebug(byte b) {
			callRuntime1( "debug", (int)b );		
		}


		
		

		public void emitComment(String string) {
			putln("// "+string);		
		}



		public void recordLineNumberToIPMapping(int lineNumber) {
			// TODO Auto-generated method stub
			
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
