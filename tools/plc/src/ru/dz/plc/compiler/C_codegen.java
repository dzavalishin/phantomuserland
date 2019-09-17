package ru.dz.plc.compiler;

import java.io.BufferedWriter;
import java.io.IOException;

import ru.dz.plc.compiler.binode.BiNode;
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
		// Getters
		// ------------------------------------------------------------------------

		public PhantomStack getIstackVars() { return m.isvars; }
		public PhantomStack GetOstackVars() { return m.svars; }


		public PhantomClass getPhantomClass() { return pc; }
		public  Method getPhantomMethod() { return m; }

		
		// ------------------------------------------------------------------------
		// output
		// ------------------------------------------------------------------------

		
		public void put(String string) {	
			try {
				c_File.write(string);
			} catch (IOException e) {
				throw new RuntimeException(e);
			}
		}

		public void putln(String string) {
			put(string+"\n");		
		}
		
		// ------------------------------------------------------------------------
		// special keywords
		// ------------------------------------------------------------------------

		public static String getJitRuntimeFuncPrefix() {	return "JIT_PhantomVm_";	}
		
		public static String getThisVarName() {				return "_vm_this_";			}
		
		public static String getLocalVarNamePrefix() {		return "_local_var_";		}

		public static String get_vm_state_var_name() {		return "JIT_vm_state";		}

		/** Return C type name for object ref */
		public static String getObjectType() { 				return "JIT_object_ref_t"; 	}
		
		// ------------------------------------------------------------------------
		// generate specific code
		// ------------------------------------------------------------------------

		private void callRuntime(String funcName, int b) {
			putln(getJitRuntimeFuncPrefix()+funcName+"("+b+");");			
		}
		
		public void emitDebug(byte b) {	callRuntime( "debug", (int)b ); }

		public void emitComment(String string) {	putln(" /* "+string+" */ ");	}

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

		public String getMethodName( PhantomClass pc, int ordinal )
		{
			String className = pc.toString();
			// TODO possible conflict - a.b_c will give same result to a_b.c
			className = className.replace('.', '_');
			return String.format("generated_class_%s_method_%d", 
					className, ordinal);			
		}
		
		public String getMethodName( Node new_this, int ordinal ) throws PlcException {
			
			String className = new_this.getType().toString();
			// TODO possible conflict - a.b_c will give same result to a_b.c
			className = className.replace('.', '_');
			return String.format("generated_class_%s_method_%d", 
					className, ordinal);
			
			//return getMethodName(new_this.getType().get_class(), ordinal);
		}
		
		public void putMethodName( Node new_this, int ordinal ) throws PlcException {
			put( getMethodName( new_this, ordinal ) );
		}


		public void emitMethodCall( Node new_this, int method_ordinal, Node args, CodeGeneratorState s ) throws PlcException
		{

			putMethodName( new_this, method_ordinal);
			put("( JIT_vm_state, /* this */ ");
			
			new_this.generate_C_code(this,s); // get object
			//if( args != null )				put(", ");

			for( Node i = args; i != null; i = ((BiNode)i).getRight() )      
			{
				put(", ");
				//boolean haveNext = ((BiNode)i).getRight() != null;
				
				i.generate_C_code(this, s);
				
				//if( haveNext )					put(", ");
			}
			//c.emitCall(method_ordinal,n_param);
			put(" ) ");
			
		}

		public void emitSnapShotTrigger() {
			// TODO need such checks on long runs too, and on method enters
			putln("JIT_check_snapshot_trigger( "+
			get_vm_state_var_name()
			+" ); // If snapshot request is active, pause self");
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
	
		
		// ------------------------------------------------------------------------
		// ip mapping
		// ------------------------------------------------------------------------

		public void recordLineNumberToIPMapping(int lineNumber) {
			// Not needed?			
		}


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
