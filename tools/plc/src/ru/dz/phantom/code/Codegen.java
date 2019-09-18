package ru.dz.phantom.code;

import java.io.*;
import java.util.*;

import phantom.code.opcode_ids;
import ru.dz.plc.compiler.PhantomType;
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
	private Writer lst = null;

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

	public void set_os(RandomAccessFile f, Writer lst2 ) throws IOException {
		this.os = f;
		this.lst  = lst2;
		start_position_in_file = os.getFilePointer();
	}

	private void list(String s) throws IOException 
	{
		boolean isComment = s.startsWith(";");
		boolean isPrefix = s.startsWith(":");

		if(lst == null ) return;

		if( !isComment )
			lst.write("  ");
		//if( s.startsWith(";") ) lst.write("  "); // indent comments 
		lst.write(s);

		if(! (isComment||isPrefix) )
		{
			int l = s.length();
			int ntab = 7;
			ntab -= l/8;
			if( ntab < 1 ) ntab = 1;
			while( ntab-- > 0 )
				lst.write("\t");

			lst.write("//  @"+getIP());
		}

		if(isPrefix )
			lst.write(' ');
		else
			lst.write('\n');
	}
	private void listlbl(String s) throws IOException 
	{
		if(lst == null ) return;
		lst.write(s); 
		lst.write("\t//  @"+getIP());
		lst.write('\n');
	}

	public void put_byte( byte v ) throws IOException { Fileops.put_byte( os, v ); }
	public void put_int32( int v ) throws IOException { Fileops.put_int32(os, v ); }
	public void put_int64( long v ) throws IOException { Fileops.put_int64(os, v ); }
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
			long diff = address_to_point_to - pos.longValue();
			if( (diff > Integer.MAX_VALUE) || (diff < Integer.MIN_VALUE))
				throw new RuntimeException("Out of int size in relative address");
			put_int32( (int)diff );
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
		list("nop");
		put_byte(opcode_nop);
	}

	public void emitJmp(String label) throws IOException {
		list("jmp "+label);
		put_byte(opcode_jmp);
		putNamedInt32Reference(label);
	}

	public void emitRet() throws IOException {
		list("ret");
		put_byte(opcode_ret);
	}


	/**
	 * Decrement top of integer stack. If top of integer stack is not zero � jump to label.
	 * NB! This operation does not pop integer stack!
	 * @param label Where to jump to.
	 * @throws IOException
	 */
	public void emitDjnz(String label) throws IOException {
		list("djnz "+label);
		put_byte(opcode_djnz);
		putNamedInt32Reference(label);
	}

	/**
	 * Jump if top of integer stack is zero. Pops.
	 * @param label Where to jump to.
	 * @throws IOException
	 */
	public void emitJz(String label) throws IOException {
		list("jz "+label);
		put_byte(opcode_jz);
		putNamedInt32Reference(label);
	}

	//  public void emit_skipz() throws IOException {    put_byte(opcode_skipz);  }

	//  public void emit_skipnz() throws IOException {    put_byte(opcode_skipnz);  }


	public void emitSwitch(Collection<String> table, int shift, int divisor )
			throws IOException
	{
		list("switch -"+shift+" /"+divisor);
		put_byte(opcode_switch);
		put_int32(table.size());
		put_int32(shift);
		put_int32(divisor);

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
		list("is dup");
		put_byte(opcode_is_dup);
	}

	public void emitIsDrop() throws IOException {
		list("is drop");
		put_byte(opcode_is_drop);
	}

	public void emitOsDup() throws IOException {
		list("os dup");
		put_byte(opcode_os_dup);
	}

	public void emitOsDrop() throws IOException {
		list("os drop");
		put_byte(opcode_os_drop);
	}


	// constants

	public void emitIConst_0() throws IOException {
		list("cons 0");
		put_byte(opcode_iconst_0);
	}

	public void emitIConst_1() throws IOException {
		list("const 1");
		put_byte(opcode_iconst_1);
	}

	public void emitIConst_8bit(byte val) throws IOException {
		list("const "+val);
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
	}

	public void emitIConst_32bit(int val) throws IOException {
		list("const "+val);
		/*if(val == 0)
			put_byte(opcode_iconst_0);
		else if(val == 1)
			put_byte(opcode_iconst_1);
		else*/
		{
			put_byte(opcode_iconst_32bit);
			put_int32(val);
		}
	}

	public void emitIConst_64bit(long val) throws IOException {
		list("const64 "+val);

		put_byte(opcode_iconst_64bit);
		put_int64(val);
	}

	/**
	 * <p>Emit string.</p>
	 * 
	 * <p>Use constant pool instead!</p>
	 *
	 * @param string String
	 */
	@Deprecated
	public void emitString(String string) throws IOException {
		list("const \""+string+"\"");
		put_byte(opcode_sconst_bin);
		put_string_bin( string );
	}

	/**
	 * <p>Emit binary data as string.</p>
	 *
	 * <p>Use constant pool instead!</p>
	 *
	 * @param data array of bytes to put out
	 */
	@Deprecated
	public void emitBinary(byte data[]) throws IOException {
		list("const <bindata>");
		put_byte(opcode_sconst_bin);
		put_string_bin( data );
	}

	/**
	 * <p>Emit constant pool reference.</p>
	 *
	 * <p>VM will pull object from corresponding constant pool slot.</p>
	 * 
	 * @param id Number of constant pool slot.
	 */
	public void emitConstantPool(int id) throws IOException {
		list("const_pool <"+id+">");
		put_byte(opcode_const_pool);
		put_int32(id);
	}


	public void emitStackReserve( int objectStackReserve, int intStackReserve ) throws IOException
	{
		assert( objectStackReserve >= 0 );
		assert( objectStackReserve < Byte.MAX_VALUE );
		assert( intStackReserve >= 0 );
		assert( intStackReserve < Byte.MAX_VALUE );

		list("stack_reserve obj="+objectStackReserve+" int="+intStackReserve);

		put_byte(opcode_stack_reserve);
		put_byte((byte)objectStackReserve);
		put_byte((byte)intStackReserve);
	}

	/**
	 * <p>Emit cast.</p>
	 *
	 * <p>VM will pop target class and object to cast, then push cast result or throw.</p>
	 * 
	 */
	public void emitCast() throws IOException {
		list("cast obj class");
		put_byte(opcode_cast);
	}


	/**
	 * summon
	 */
	public void emitSummonThread() throws IOException {
		list("summon thread");
		put_byte(opcode_summon_thread);
	}

	public void emitSummonThis() throws IOException {
		list("summon this");
		put_byte(opcode_summon_this);
	}

	public void emitSummonNull() throws IOException {
		list("summon null");
		put_byte(opcode_summon_null);
	}

	public void emit_i2o() throws IOException {
		list("i2o");
		put_byte(opcode_i2o);
	}

	public void emit_o2i() throws IOException {
		list("o2i");
		put_byte(opcode_o2i);
	}


	public void emitISum() throws IOException {
		list("isum");
		put_byte(opcode_isum);
	}

	public void emitIMul() throws IOException {
		list("imul");
		put_byte(opcode_imul);
	}

	/** Integer subtract upper from lower. */
	public void emitISubUL() throws IOException {
		list("isubul");
		put_byte(opcode_isubul);
	}

	/** Integer subtract lower from upper. */
	public void emitISubLU() throws IOException {
		list("isublu");
		put_byte(opcode_isublu);
	}

	public void emitIDivUL() throws IOException {
		list("idivul");
		put_byte(opcode_idivul);
	}

	public void emitIDivLU() throws IOException {
		list("idivlu");
		put_byte(opcode_idivlu);
	}

	public void emitIRemLU() throws IOException {
		list("iremlu");
		put_byte(opcode_iremlu);
	}


	public void emitIShiftLeft() throws IOException {
		list("ishl");
		put_byte(opcode_ishl);
	}

	/** Signed shift right 
	 * @throws IOException */
	public void emitIShiftRight() throws IOException {
		list("ishr");
		put_byte(opcode_ishr);
	}

	/** Unsigned shift right 
	 * @throws IOException */
	public void emitUShiftRight() throws IOException {
		list("ushr");
		put_byte(opcode_ushr);
	}



	public void emitDebug(byte type, String text) throws IOException {
		list("debug");

		put_byte(opcode_debug);
		type &= 0x7F;
		if( text != null ) type |= 0x80;
		put_byte(type);
		if( text != null ) put_string_bin( text );
	}

	/**
	 * Emits a call instruction. n_param objects from o stack are parameters, one more is 
	 * 'this' for a call. On return result is on o stack.
	 * @param method_index Ordinal (VMT offset) of method to call.
	 * @param n_param Number of parameters.
	 * @throws IOException
	 * @throws PlcException parameters
	 */
	public void emitCall(int method_index, int n_param ) throws IOException, PlcException {

		if( method_index < 0 )
		{
			//SootMain.say("negative method index");
			//throw new IOException("negative method index");
			throw new PlcException("emitCall", "negative method index");
		}

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
	public void emitNew() throws IOException 	{  list("new"); put_byte(opcode_new); }

	public void emitCopy() throws IOException 	{  list("copy"); put_byte(opcode_copy); }


	public void emit_ior()  throws IOException	{  list("ior"); put_byte(opcode_ior); }
	public void emit_iand() throws IOException	{  list("iand"); put_byte(opcode_iand); }
	public void emit_ixor() throws IOException	{  list("ixor"); put_byte(opcode_ixor); }
	public void emit_inot() throws IOException	{  list("inot"); put_byte(opcode_inot); }

	public void emitLogOr()  throws IOException {  list("logor ||"); put_byte(opcode_log_or); }
	public void emitLogAnd() throws IOException {  list("logand &&"); put_byte(opcode_log_and); }
	public void emitLogXor() throws IOException {  list("logxor"); put_byte(opcode_log_xor); }
	public void emitLogNot() throws IOException {  list("lognot"); put_byte(opcode_log_not); }

	public void emit_igt() throws IOException	{  list("igt >"); put_byte(opcode_igt);}
	public void emit_ilt() throws IOException	{  list("ilt <"); put_byte(opcode_ilt); }
	public void emit_ige() throws IOException	{  list("ige >="); put_byte(opcode_ige); }
	public void emit_ile() throws IOException	{  list("ile <="); put_byte(opcode_ile); }

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

		assert(name.length() > 0);

		//list("summon class '"+name+"'");
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

	//@Deprecated
	public void emit_pull( int depth ) throws IOException
	{
		list("pull depth="+depth);
		put_byte( opcode_os_pull32 );
		put_int32(depth);
	}

	/**
	 * Load object from given position of object stack, push to top of stack.
	 * Local variables access.
	 * @param pos Stack position to get from.
	 * @throws IOException
	 */
	public void emitGet( int pos ) throws IOException
	{
		list("get o stk pos="+pos);
		put_byte( opcode_os_get32 );
		put_int32(pos);
	}

	/**
	 * Pop object, store to selected position of object stack. 
	 * Local variables access.
	 * @param pos
	 * @throws IOException
	 */
	public void emitSet( int pos ) throws IOException
	{
		list("set o stk pos="+pos);
		put_byte( opcode_os_set32 );
		put_int32(pos);
	}

	/**
	 * Load object from given position of object stack, push to top of stack.
	 * Local variables access.
	 * @param pos Stack position to get from.
	 * @throws IOException
	 */
	public void emitIGet( int pos ) throws IOException
	{
		list("get i stk pos="+pos);
		put_byte( opcode_is_get32 );
		put_int32(pos);
	}

	/**
	 * Pop object, store to selected position of object stack. 
	 * Local variables access.
	 * @param pos
	 * @throws IOException
	 */
	public void emitISet( int pos ) throws IOException
	{
		list("set i stk pos="+pos);
		put_byte( opcode_is_set32 );
		put_int32(pos);
	}



	public void emit_ptr_eq() throws IOException
	{
		list("ptr eq");
		put_byte(opcode_os_eq);
	}

	public void emit_ptr_neq() throws IOException
	{
		list("ptr neq");
		put_byte(opcode_os_neq);
	}

	public void emitPushNull() throws IOException
	{
		list("summon null");
		//put_byte(opcode_os_push_null);
		//list("push null");
		put_byte(opcode_summon_null);
	}

	public void emitIsNull() throws IOException
	{
		list("isnull");
		put_byte(opcode_os_isnull);
	}

	public void emitLock() throws IOException {
		list("lock");
		put_byte(opcode_general_lock);
	}


	public void emitUnLock() throws IOException {
		list("unlock");
		put_byte(opcode_general_unlock);
	}


	//public void emitDynamicCall(int n_param) throws IOException {
	public void emitDynamicCall() throws IOException {
		list("dynamic invoke");
		put_byte(opcode_dynamic_invoke);
		//put_int32(n_param);
	}

	public void emitStaticCall(int ordinal, int n_param) throws IOException {
		list(String.format("static invoke ord=%d n_param=%d", ordinal, n_param));
		put_byte(opcode_static_invoke);
		put_int32(ordinal);
		put_int32(n_param);
	}

	/**
	 * emit prefix byte supposing next int stack operation is of this type
	 * @param c
	 * @throws PlcException 
	 * @throws IOException 
	 */
	public void emitNumericPrefix(PhantomType t) throws PlcException, IOException {
		if( t.is_double() )
		{
			list(":double");
			put_byte(opcode_prefix_double);
		}
		else if( t.is_float() )		
		{
			list(":float");
			put_byte(opcode_prefix_float);
		}
		else if( t.is_long() )		
		{
			list(":long");
			put_byte(opcode_prefix_long);
		}
		else if( !t.is_int())
			throw new PlcException("unknown numeric type: "+t+", possibly untyped variable in numeric context.");
	}

	public void emitNumericCast(PhantomType from, PhantomType to) throws PlcException, IOException
	{
		if( !to.can_be_assigned_from(from) )
			throw new PlcException("can't cast from "+from+" to "+to);

		if( !from.is_on_int_stack() )
			throw new PlcException("not on int stack "+from);

		if( !to.is_on_int_stack() )
			throw new PlcException("not on int stack "+to);

		// generate destination type prefix
		emitNumericPrefix(to);

		if( from.is_double() )			
		{
			list("from double");
			put_byte(opcode_fromd);
		}
		else if( from.is_float() )		
		{
			list("from float");
			put_byte(opcode_fromf);
		}
		else if( from.is_long() )			
		{
			list("from long");
			put_byte(opcode_froml);
		}
		else if( from.is_int())			
		{
			list("from int");
			put_byte(opcode_fromi);
		}
		else
			throw new PlcException("unknown type of "+to);
	}

	public void emit_arg_count_check(int n_args) throws IOException, PlcException {
		list("arg_count "+n_args);

		if(n_args > Byte.MAX_VALUE) 
			throw new PlcException("emit_arg_count_check", "n_args > 127", Integer.toString(n_args));

		put_byte(opcode_arg_count);
		put_byte((byte)n_args);
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
			e.printStackTrace();			
		}

	}


	public Map<Long, Integer> getIpToLine() {
		return IpToLine;
	}





}
