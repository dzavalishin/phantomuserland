package ru.dz.phantom.code;

import java.io.*;
import java.util.*;

import ru.dz.plc.util.*;

/**
 * <p>Title: Codegen</p>
 *
 * <p>Description: low level code generator</p>
 *
 * <p>Copyright: Copyright (c) 2004-2009 dz@dz.ru</p>
 *
 * <p>Company: Digital Zone</p>
 * @author dz
 */
public class Codegen extends opcode_ids {
	private RandomAccessFile os;
	private ForwardAddressMap fmap = new ForwardAddressMap();
	private BackAddressMap    bmap = new BackAddressMap();

	private long                start_position_in_file = 0;

	// ------------------------------------------------------------------------
	// labels
	// ------------------------------------------------------------------------

	private int next_label_no = 0;
	private FileWriter lst = null;

	/**
	 * Create a new unique label name.
	 * @return Name of a new label.
	 */
	public String getLabel()
	{
		return "_label_"+Integer.toString(next_label_no++);
	}


	// ------------------------------------------------------------------------


	/*public Codegen(String f) throws FileNotFoundException, IOException {
    this.os = new RandomAccessFile( f, "rw" );
  }*/

	public Codegen(RandomAccessFile f, FileWriter lst ) throws IOException {
		this.os = f;
		this.lst  = lst;
		start_position_in_file = os.getFilePointer();
	}

	public long getIP() throws IOException
	{
		return os.getFilePointer() - start_position_in_file;
	}

	public Codegen() {
		this.os = null;
	}

	public void set_os(RandomAccessFile f, FileWriter lst ) throws IOException {
		this.os = f;
		this.lst  = lst;
		start_position_in_file = os.getFilePointer();
	}

	private void list(String s) throws IOException 
	{
		if(lst == null ) return;
		lst.write("  ");
		lst.write(s); 
		lst.write("\n//  @"+getIP());
		lst.write('\n');
	}
	private void listlbl(String s) throws IOException 
	{
		if(lst == null ) return;
		lst.write(s); 
		lst.write("\n//  @"+getIP());
		lst.write('\n');
	}
	
	public void put_byte( byte v ) throws IOException { Fileops.put_byte( os, v ); }
	public void put_int32( int v ) throws IOException { Fileops.put_int32(os, v ); }
	public void put_string_bin( String v ) throws IOException { Fileops.put_string_bin(os, v); }
	public void put_string_bin( byte [] v ) throws IOException { Fileops.put_string_bin(os, v); }	

	// ------ map works --------

	// write an int 32 into the place
	/**
	 * Write correct values into the code references (int32 relative addresses).
	 * Scan all the points saved for this name for later relocation, write
	 * corresponding addresses into. Note that address is written relative to
	 * itself, NOT relative to its instruction!!
	 * <p>
	 * TODO constant 4 must be defined or derived
	 *
	 * @param name String - name of label which reference should look to.
	 * @param f RandomAccessFile
	 * @param address_to_point_to int - address in file of the labeled stuff
	 * @throws IOException
	 */
	public void correct_int32( String name, RandomAccessFile f, long address_to_point_to )
	throws IOException {
		LinkedList<Long> posl = bmap.get(name);

		if( posl == null ) return;

		for( Iterator<Long> i = posl.iterator(); i.hasNext(); )
		{
			Long pos = i.next();
			f.seek(pos.longValue());
			// TODO Loss of precision?
			//put_int32( (int)(address_to_point_to - pos.longValue() - 4) ); // BUG! 4!
			put_int32( (int)(address_to_point_to - pos.longValue()) );
		}
	}


	public void relocate_all() throws EmptyPlcException, IOException, PlcException {
		for( Iterator<String> i = fmap.iterator(); i.hasNext(); )
		{
			String name = i.next();
			long address = fmap.get(name);
			correct_int32( name, os, address );

			bmap.remove(name);
		}
		// @todo - check for unreferenced labels and for unlabeller references

		StringBuffer sb = new StringBuffer();
		boolean errors = false;
		for( Iterator<String> i = bmap.iterator(); i.hasNext(); )
		{
			if(errors) // not first
				sb.append( ", " );
			sb.append( "'" );
			sb.append( i.next() );
			sb.append( "'" );
			errors = true;
		}
		if( errors ) throw new PlcException( "code generator", "Undefined labels", sb.toString() );
	}

	/**
	 * This will put a jump destination address for a given label.
	 * @param name Label to jump to.
	 * @throws IOException
	 */
	public void putNamedInt32Reference(String name) throws IOException {
		long now_position_in_file = os.getFilePointer();
		put_int32(0xFFFFFFFF); // just write something completely wrong
		bmap.add(name,now_position_in_file);
	}

	/**
	 * Mark a place in code label will refer to. This is a position where jump on label will bring you. 
	 * @param name Label name.
	 * @throws IOException
	 */
	public void markLabel( String name ) throws IOException {
		long now_position_in_file = os.getFilePointer();
		fmap.add(name,now_position_in_file);
		listlbl(name+":");
	}

	// ------------- operations code generators

	public void emitNop() throws IOException {
		put_byte(opcode_nop);
		list("nop");
	}

	public void emitJmp(String label) throws IOException {
		put_byte(opcode_jmp);
		putNamedInt32Reference(label);
		list("jmp "+label);
	}

	public void emitRet() throws IOException {
		put_byte(opcode_ret);
		list("ret");
	}


	/**
	 * Decrement top of integer stack. If top of integer stack is not zero – jump to label.
	 * NB! This operation does not pop integer stack!
	 * @param label Where to jump to.
	 * @throws IOException
	 */
	public void emitDjnz(String label) throws IOException {
		put_byte(opcode_djnz);
		putNamedInt32Reference(label);
		list("djnz "+label);
	}

	/**
	 * Jump if top of integer stack is zero. Pops.
	 * @param label Where to jump to.
	 * @throws IOException
	 */
	public void emitJz(String label) throws IOException {
		put_byte(opcode_jz);
		putNamedInt32Reference(label);
		list("jz "+label);
	}

	//  public void emit_skipz() throws IOException {    put_byte(opcode_skipz);  }

	//  public void emit_skipnz() throws IOException {    put_byte(opcode_skipnz);  }


	public void emitSwitch(Collection<String> table, int shift, int divisor )
	throws IOException
	{
		put_byte(opcode_switch);
		put_int32(table.size());
		put_int32(shift);
		put_int32(divisor);
		list("switch -"+shift+" /"+divisor);
		for( Iterator<String> i = table.iterator(); i.hasNext(); )
		{
			String lbl = i.next();
			putNamedInt32Reference(lbl);
			list("  "+lbl+":");
		}
	}

	/**
	 * drop and dup
	 */
	public void emitIsDup() throws IOException {
		put_byte(opcode_is_dup);
		list("is dup");
	}

	public void emitIsDrop() throws IOException {
		put_byte(opcode_is_drop);
		list("is drop");
	}

	public void emitOsDup() throws IOException {
		put_byte(opcode_os_dup);
		list("os dup");
	}

	public void emitOsDrop() throws IOException {
		put_byte(opcode_os_drop);
		list("os drop");
	}


	// constants

	public void emitIConst_0() throws IOException {
		put_byte(opcode_iconst_0);
		list("cons 0");
	}

	public void emitIConst_1() throws IOException {
		put_byte(opcode_iconst_1);
		list("const 1");
	}

	public void emitIConst_8bit(byte val) throws IOException {
		/*
		if(val == 0)
			put_byte(opcode_iconst_0);
		else if(val == 1)
			put_byte(opcode_iconst_1);
		else*/
		{
			put_byte(opcode_iconst_8bit);
			put_byte(val);
		}
		list("const "+val);
	}

	public void emitIConst_32bit(int val) throws IOException {
		/*if(val == 0)
			put_byte(opcode_iconst_0);
		else if(val == 1)
			put_byte(opcode_iconst_1);
		else*/
		{
			put_byte(opcode_iconst_32bit);
			put_int32(val);
		}
		list("const "+val);
	}

	/**
	 * emit_string
	 *
	 * @param string String
	 */
	public void emitString(String string) throws IOException {
		put_byte(opcode_sconst_bin);
		put_string_bin( string );
		list("const \""+string+"\"");
	}

	/**
	 * emit binary data as string
	 *
	 * @param data array of bytes to put out
	 */
	public void emitBinary(byte data[]) throws IOException {
		put_byte(opcode_sconst_bin);
		put_string_bin( data );
		list("const <bindata>");
	}
	
	/**
	 * summon
	 */
	public void emitSummonThread() throws IOException {
		put_byte(opcode_summon_thread);
		list("summon thread");
	}

	public void emitSummonThis() throws IOException {
		put_byte(opcode_summon_this);
		list("summon this");
	}

	public void emitSummonNull() throws IOException {
		put_byte(opcode_summon_null);
		list("summon null");
	}

	public void emit_i2o() throws IOException {
		put_byte(opcode_i2o);
		list("i2o");
	}

	public void emit_o2i() throws IOException {
		put_byte(opcode_o2i);
		list("o2i");
	}


	public void emitISum() throws IOException {
		put_byte(opcode_isum);
		list("isum");
	}

	public void emitIMul() throws IOException {
		put_byte(opcode_imul);
		list("imul");
	}

	/** Integer subtract upper from lower. */
	public void emitISubUL() throws IOException {
		put_byte(opcode_isubul);
		list("isubul");
	}

	/** Integer subtract lower from upper. */
	public void emitISubLU() throws IOException {
		put_byte(opcode_isublu);
		list("isublu");
	}

	public void emitIDivUL() throws IOException {
		put_byte(opcode_idivul);
		list("idivul");
	}

	public void emitIDivLU() throws IOException {
		put_byte(opcode_idivlu);
		list("idivlu");
	}

	public void emitIRemLU() throws IOException {
		put_byte(opcode_iremlu);
		list("iremlu");
	}



	public void emitDebug(byte type, String text) throws IOException {
		put_byte(opcode_debug);
		type &= 0x7F;
		if( text != null ) type |= 0x80;
		put_byte(type);
		if( text != null ) put_string_bin( text );
		list("debug");
	}

	/**
	 * Emits a call instruction. n_param objects from o stack are parameters, one more is 
	 * 'this' for a call. On return result is on o stack.
	 * @param method_index Ordinal (VMT offset) of method to call.
	 * @param n_param Number of parameters.
	 * @throws IOException
	 */
	public void emitCall(int method_index, int n_param ) throws IOException {

		list("call m="+method_index+" nparm="+n_param);
		
		if( n_param == 0 )
		{
			switch(method_index) {
			case 0:     put_byte(opcode_short_call_0);          return;
			case 1:     put_byte(opcode_short_call_1);          return;
			case 2:     put_byte(opcode_short_call_2);          return;
			case 3:     put_byte(opcode_short_call_3);          return;
			default:
				break;
			}
		}

		// 8-bit n_param
		if( (n_param & 0xFFF0) == 0 && method_index < 32 )
		{
			put_byte((byte)(opcode_call_00+method_index));
			put_byte((byte)n_param);
			return;
		}

		if( (method_index & 0xFFF0) == 0 )
		{
			put_byte(opcode_call_8bit);
			put_byte((byte)method_index);
			put_int32(n_param);
		}
		else
		{
			put_byte(opcode_call_32bit);
			put_int32(method_index);
			put_int32(n_param);
		}
	}

	/**
	 * Loads (pushes to object stack) object reference from given slot (field) of the current object.
	 * @param fieldnum Number of field to load.
	 * @throws IOException
	 */
	public void emitLoad( int fieldnum ) throws IOException {
		list("load fld="+fieldnum);
		if( fieldnum < 256 )
		{
			put_byte(opcode_os_load8);
			put_byte((byte)fieldnum);
		}
		else
		{
			put_byte(opcode_os_load32);
			put_int32(fieldnum);
		}
	}

	public void emitSave( int fieldnum ) throws IOException {
		list("save fld="+fieldnum);
		if( fieldnum < 256 )
		{
			put_byte(opcode_os_save8);
			put_byte((byte)fieldnum);
		}
		else
		{
			put_byte(opcode_os_save32);
			put_int32(fieldnum);
		}
	}

	// TODO: iload/isave instructions - like load/save but directly to/from int stack

	/**
	 * Create an object of class defined by a top-of-stack object. Will throw at runtime if not a class.
	 */
	public void emitNew() throws IOException {  put_byte(opcode_new); list("new"); }

	public void emitCopy() throws IOException {  put_byte(opcode_copy); list("copy"); }


	public void emit_ior()  throws IOException {  put_byte(opcode_ior); list("ior"); }
	public void emit_iand() throws IOException {  put_byte(opcode_iand); list("iand"); }
	public void emit_ixor() throws IOException {  put_byte(opcode_ixor); list("ixor"); }
	public void emit_inot() throws IOException {  put_byte(opcode_inot); list("inot"); }

	public void emitLogOr()  throws IOException {  put_byte(opcode_log_or);  list("logor ||"); }
	public void emitLogAnd() throws IOException {  put_byte(opcode_log_and); list("logand &&"); }
	public void emitLogXor() throws IOException {  put_byte(opcode_log_xor); list("logxor"); }
	public void emitLogNot() throws IOException {  put_byte(opcode_log_not); list("lognot"); }

	public void emit_igt() throws IOException {  put_byte(opcode_igt); list("igt >"); }
	public void emit_ilt() throws IOException {  put_byte(opcode_ilt); list("ilt <"); }
	public void emit_ige() throws IOException {  put_byte(opcode_ige); list("ige >="); }
	public void emit_ile() throws IOException {  put_byte(opcode_ile); list("ile <="); }

	/**
	 * Class object for given name pushed on stack.
	 * @param name Name of class to find. 
	 * @throws IOException
	 */
	public void emitSummonByName( String name ) throws IOException
	{
		if( name.startsWith(".") )
			name = name.substring(1);
		name = name.toLowerCase();

		list("summon class "+name);
		
		if( name.equals( "internal.class" ) )                put_byte(opcode_summon_class_class);
		else if( name.equals( "internal.class.class" ))      put_byte(opcode_summon_class_class);
		else if( name.equals( "internal.interface" ))        put_byte(opcode_summon_interface_class);
		else if( name.equals( "internal.class.interface" ))  put_byte(opcode_summon_interface_class);
		else if( name.equals( "internal.code" ))             put_byte(opcode_summon_code_class);
		else if( name.equals( "internal.int" ))              put_byte(opcode_summon_int_class);
		else if( name.equals( "internal.class.int" ))        put_byte(opcode_summon_int_class);
		else if( name.equals( "internal.string" ))           put_byte(opcode_summon_string_class);
		else if( name.equals( "internal.class.string" ))     put_byte(opcode_summon_string_class);
		else if( name.equals( "internal.container.array" ))  put_byte(opcode_summon_array_class);
		else
		{
			put_byte(opcode_summon_by_name);
			put_string_bin( "." + name );
		}
	}


	@Deprecated
	public void emit_compose( int num ) throws IOException
	{
		list("??? compose "+num);
		put_byte( opcode_os_compose32 );
		put_int32(num);
	}

	@Deprecated
	public void emit_decompose() throws IOException
	{
		list("??? decompose ");
		put_byte( opcode_os_decompose );
	}

	/**
	 * Object on top of stack is thrown.
	 * @throws IOException
	 */
	public void emitThrow() throws IOException
	{
		list("throw");
		put_byte( opcode_throw );
	}

	/**
	 * Last pushed catcher will be killed.
	 * @throws IOException
	 */
	public void emitPopCatcher() throws IOException
	{
		list("pop catcher");
		put_byte( opcode_pop_catcher );
	}

	/**
	 * Top of stack contains class object. Exceptions of that class 
	 * will be catched here and control will be passed to a label 
	 * if such exception is thrown. Thrown object will be on stack top in this case.
	 * @param label Where to pass control in case of exception catched.
	 * @throws IOException
	 */
	public void emitPushCatcher(String label) throws IOException
	{
		list("push catcher "+label);
		put_byte( opcode_push_catcher );
		putNamedInt32Reference(label);
	}

	@Deprecated
	public void emit_pull( int depth ) throws IOException
	{
		put_byte( opcode_os_pull32 );
		put_int32(depth);
		list("?? pull depth="+depth);
	}

	/**
	 * Load object from given position of object stack, push to top of stack.
	 * Local variables access.
	 * @param pos Stack position to get from.
	 * @throws IOException
	 */
	public void emitGet( int pos ) throws IOException
	{
		put_byte( opcode_os_get32 );
		put_int32(pos);
		list("get o stk pos="+pos);
	}

	/**
	 * Pop object, store to selected position of object stack. 
	 * Local variables access.
	 * @param pos
	 * @throws IOException
	 */
	public void emitSet( int pos ) throws IOException
	{
		put_byte( opcode_os_set32 );
		put_int32(pos);
		list("set o stk pos="+pos);
	}

	/**
	 * Load object from given position of object stack, push to top of stack.
	 * Local variables access.
	 * @param pos Stack position to get from.
	 * @throws IOException
	 */
	public void emitIGet( int pos ) throws IOException
	{
		put_byte( opcode_is_get32 );
		put_int32(pos);
		list("get i stk pos="+pos);
	}

	/**
	 * Pop object, store to selected position of object stack. 
	 * Local variables access.
	 * @param pos
	 * @throws IOException
	 */
	public void emitISet( int pos ) throws IOException
	{
		put_byte( opcode_is_set32 );
		put_int32(pos);
		list("set i stk pos="+pos);
	}

	
	
	public void emit_ptr_eq() throws IOException
	{
		put_byte(opcode_os_eq);
		list("ptr eq");
	}

	public void emit_ptr_neq() throws IOException
	{
		put_byte(opcode_os_neq);
		list("ptr neq");
	}

	public void emitPushNull() throws IOException
	{
		put_byte(opcode_os_push_null);
		list("push null");
	}

	public void emitIsNull() throws IOException
	{
		put_byte(opcode_os_isnull);
		list("isnull");
	}


	public void emitComment(String string) throws IOException {
		list("; "+string);		
	}

	private Map<Long,Integer> IpToLine = new HashMap<Long, Integer>();

	public void recordLineNumberToIPMapping(int lineNumber) {
		try {
			long ip = getIP();
			
			IpToLine.put(ip, lineNumber);
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			
		}
		
	}


	public Map<Long, Integer> getIpToLine() {
		return IpToLine;
	}

}
