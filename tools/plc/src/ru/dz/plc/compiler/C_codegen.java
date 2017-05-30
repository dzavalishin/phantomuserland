package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;

import ru.dz.plc.compiler.node.Node;
import ru.dz.plc.util.PlcException;


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

		// ------------------------------------------------------------------------
		// labels
		// ------------------------------------------------------------------------

		private int next_label_no = 0;

		/**
		 * Create a new unique label name.
		 * @return Name of a new label.
		 */
		public String getLabel()
		{
			return "_label_"+Integer.toString(next_label_no++);
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
			putln("JIT_PhantomVm_"+funcName+"("+b+");");
			
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


		/** Return C type name for object ref */
		public static String getObjectType() { return "JIT_object_ref_t"; }

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


		public void markLabel(String label) {
			putln(label+":");
		}


		public void emitJump(String label) {
			putln("goto "+label+";");
		}


		private void doEmitIf(Node ifValue, CodeGeneratorState s, boolean invert) throws PlcException {
			put("if( ");		

			if(invert) put("!");
			
			// I need it on int stack!
			if( !ifValue.is_on_int_stack() )
			{
				put("JIT_o2i( ");
				ifValue.generate_C_code(this,s); // calc value
				put(") ");					
			}
			else
				ifValue.generate_C_code(this,s); // calc value
		}


		public void emitIf(Node ifValue, CodeGeneratorState s) throws PlcException {
			doEmitIf(ifValue, s, false);
		}

		public void emitIfNot(Node ifValue, CodeGeneratorState s) throws PlcException {
			doEmitIf(ifValue, s, true);
		}





		public void putMethodName( Node new_this, int ordinal ) throws PlcException {
			put( String.format("generated_class_%s_method_%d", 
					new_this.getType().toString(), ordinal) );
		}








	}
