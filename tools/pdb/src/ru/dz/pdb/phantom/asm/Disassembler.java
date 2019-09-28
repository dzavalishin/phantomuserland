package ru.dz.pdb.phantom.asm;

import java.io.UnsupportedEncodingException;

import phantom.code.opcode_ids;

public class Disassembler extends opcode_ids {

	private final byte[] data;
	private int ip;

	byte current_prefix = 0; // long/float/double
	
	public Disassembler(byte [] data ) 
	{
		this.data = data;
		ip = 0;
	}
	
	public void jumpTo( int addr )
	{
		if(ip > data.length)
			throw new ArrayIndexOutOfBoundsException(addr);
		
		ip = addr;
	}
	
	public boolean hasNext() { return ip < data.length; }
	
	public PhantomInstruction next(byte prefix)
	{
		byte opcode = getByte();
		current_prefix = prefix;

		switch(opcode)
		{
		case opcode_prefix_long:
		case opcode_prefix_float:
		case opcode_prefix_double:
			byte new_prefix = opcode;
			if( prefix != 0)
				return simple("!! Double Prefix !!");
			return next(new_prefix);
			
			default:
				break;
		}
		
		switch(opcode)
		{
		
		case opcode_nop:				return simple("nop");
		
		case opcode_debug:
		{
			int type = 0xFF & getByte();
			String s = "";
			if( (type & 0x80) != 0 )
			{
				try {
					s = new String( getString(), "UTF-8" ) ;
				} catch (UnsupportedEncodingException e) {
					e.printStackTrace();
				}
			}
			return intarg("debug "+s, type );
		}
		
		//case opcode_skipz: 				return simple("skipz?");
		//case opcode_skipnz:				return simple("skipnz?");
		case opcode_djnz:				{ int ip = getIp(); return intarg("djnz", ip+getInt()); }
		case opcode_jz:					{ int ip = getIp(); return intarg("jz", ip+getInt()); }
		case opcode_jmp:				{ int ip = getIp(); return intarg("jmp", ip+getInt()); }
		case opcode_switch:				{ getInt(); getInt(); getInt(); return simple("switch"); }
		case opcode_ret:				return simple("ret");
		case opcode_short_call_0:		return simple("short_call_0");
		case opcode_short_call_1:		return simple("short_call_1");
		case opcode_short_call_2:		return simple("short_call_2");
		case opcode_short_call_3:		return simple("short_call_3");
		
		case opcode_call_8bit:			{ int m = getByte(); return int2arg("call", m, getInt()); }
		case opcode_call_32bit:			{ int m = getInt(); return int2arg("call", m, getInt()); }
		
		case opcode_sys_8bit:			return intarg("sys", getByte());
		                                                                
		case opcode_is_dup:				return simple("is_dup");
		case opcode_is_drop:			return simple("is_drop");
		case opcode_os_dup:				return simple("os_dup");
		case opcode_os_drop:			return simple("os_drop");
		case opcode_os_load8:			return intarg("os_load", getByte() );
		case opcode_os_save8:			return intarg("os_save", getByte() );
		case opcode_os_load32:			return intarg("os_load", getInt());
		case opcode_os_save32:			return intarg("os_save", getInt());
		case opcode_new:				return simple("new");
		case opcode_copy:				return simple("copy");
		case opcode_os_compose32:		return intarg("os_compose", getInt());
		case opcode_os_decompose:		return simple("os_decompose");
		                                                                
		                                                                
		case opcode_os_pull32:			return intarg("os_pull", getInt());
		                                                                
		case opcode_os_get32:			return intarg("os_get", getInt());
		case opcode_os_set32:			return intarg("os_set", getInt());
		     
		case opcode_const_pool:			return intarg("const pool", getInt());
		case opcode_stack_reserve:		{ int m = getByte(); return int2arg("stack reserve", m, getByte()); }
		
		case opcode_cast:				return simple("cast");
		                                                                
		case opcode_push_catcher:		return intarg("push_catcher", getInt());
		case opcode_pop_catcher:		return simple("pop_catcher");
		case opcode_throw:				return simple("throw");
		                                                                
		                                                                
		case opcode_iconst_0:			return intarg("iconst", 0);
		case opcode_iconst_1:			return intarg("iconst", 1);
		case opcode_iconst_8bit:		return intarg("iconst", getByte());
		case opcode_iconst_32bit:		return intarg("iconst", getInt());
		case opcode_sconst_bin:			return stringarg("sconst", getString());
		                                                                
		                                                                
		case opcode_is_get32:			return intarg("is_get", getInt());
		case opcode_is_set32:			return intarg("is_set", getInt());
		                                                                
		                                                                
		                                                                
		case opcode_summon_thread:		return simple("summon_thread");
		case opcode_summon_this:		return simple("summon_this");
		                                                                
		case opcode_summon_null:		return simple("summon_null");
		                                                                
		case opcode_summon_class_class:	return simple("summon_class_class");
		case opcode_summon_int_class:	return simple("summon_int_class");
		case opcode_summon_string_class:return simple("summon_string_class");
		case opcode_summon_interface_class:	return simple("summon_interface_class");
		case opcode_summon_code_class:	return simple("summon_code_class");
		case opcode_summon_array_class:	return simple("summon_array_class");
		                                                                
		//case opcode_summon_by_name:		return simple("summon_by_name");
		case opcode_summon_by_name:		return stringarg("summon_by_name",getString());
		                                                                
		                                                                
		case opcode_i2o:				return simple("i2o");
		case opcode_o2i:				return simple("o2i");
		case opcode_isum:				return simple("isum");
		case opcode_imul:				return simple("imul");
		case opcode_isubul:				return simple("isubul");
		case opcode_isublu:				return simple("isublu");
		case opcode_idivul:				return simple("idivul");
		case opcode_idivlu:				return simple("idivlu");
		case opcode_ior:				return simple("ior");
		case opcode_iand:				return simple("iand");
		case opcode_ixor:				return simple("ixor");
		case opcode_inot:				return simple("inot");
		case opcode_log_or:				return simple("log_or");
		case opcode_log_and:			return simple("log_and");
		case opcode_log_xor:			return simple("log_xor");
		case opcode_log_not:			return simple("log_not");
		                                                                
		                                                                
		                                                                
		case opcode_is_load8:			return intarg("is_load", getByte());
		case opcode_is_save8:			return intarg("is_save", getByte());
		                                                                
		case opcode_ige:				return simple("ige");
		case opcode_ile:				return simple("ile");
		case opcode_igt:				return simple("igt");
		case opcode_ilt:				return simple("ilt");
		                                                                
		case opcode_os_eq:				return simple("os_eq");
		case opcode_os_neq:				return simple("os_neq");
		case opcode_os_isnull:			return simple("os_isnull");
		
		case opcode_general_lock:		return simple("lock mutex");
		case opcode_general_unlock:		return simple("unlock mutex");
		
		case opcode_static_invoke:		{ int m = getInt(); return int2arg("static invoke", m, getInt()); }
		case opcode_dynamic_invoke:		return simple("dynamic invoke");
		
		case opcode_ishl:				return simple("opcode_ishl");
		case opcode_ishr:				return simple("opcode_ishr");
		case opcode_ushr:				return simple("opcode_ushr");
		case opcode_arg_count:			return intarg("arg count check", getByte());
		case opcode_fromi:				return simple("cast from int");
		case opcode_froml:				return simple("cast from long");
		case opcode_fromf:				return simple("cast from float");
		case opcode_fromd:				return simple("cast from double");
		
		//case opcode_os_push_null:		return simple("os_push_null");		                                                                
/*		                                                                
		case opcode_sys_0:				return intarg("sys", 0x0);
		case opcode_sys_1:				return intarg("sys", 0x1);
		case opcode_sys_2:				return intarg("sys", 0x2);
		case opcode_sys_3:				return intarg("sys", 0x3);
		case opcode_sys_4:				return intarg("sys", 0x4);
		case opcode_sys_5:				return intarg("sys", 0x5);
		case opcode_sys_6:				return intarg("sys", 0x6);
		case opcode_sys_7:				return intarg("sys", 0x7);
		case opcode_sys_8:				return intarg("sys", 0x8);
		case opcode_sys_9:				return intarg("sys", 0x9);
		case opcode_sys_A:				return intarg("sys", 0xA);
		case opcode_sys_B:				return intarg("sys", 0xB);
		case opcode_sys_C:				return intarg("sys", 0xC);
		case opcode_sys_D:				return intarg("sys", 0xD);
		case opcode_sys_E:				return intarg("sys", 0xE);
		case opcode_sys_F:				return intarg("sys", 0xF);
*/		                                                                
		                                                                
		case opcode_call_00:			return int2arg("call", 0x00, getByte() );
		case opcode_call_01:			return int2arg("call", 0x01, getByte() );
		case opcode_call_02:			return int2arg("call", 0x02, getByte() );
		case opcode_call_03:			return int2arg("call", 0x03, getByte() );
		case opcode_call_04:			return int2arg("call", 0x04, getByte() );
		case opcode_call_05:			return int2arg("call", 0x05, getByte() );
		case opcode_call_06:			return int2arg("call", 0x06, getByte() );
		case opcode_call_07:			return int2arg("call", 0x07, getByte() );
		case opcode_call_08:			return int2arg("call", 0x08, getByte() );
		case opcode_call_09:			return int2arg("call", 0x09, getByte() );
		case opcode_call_0A:			return int2arg("call", 0x0A, getByte() );
		case opcode_call_0B:			return int2arg("call", 0x0B, getByte() );
		case opcode_call_0C:			return int2arg("call", 0x0C, getByte() );
		case opcode_call_0D:			return int2arg("call", 0x0D, getByte() );
		case opcode_call_0E:			return int2arg("call", 0x0E, getByte() );
		case opcode_call_0F:			return int2arg("call", 0x0F, getByte() );
		                                                                
		                                                                
		case opcode_call_10:			return int2arg("call", 0x10, getByte() );
		case opcode_call_11:			return int2arg("call", 0x11, getByte() );
		case opcode_call_12:			return int2arg("call", 0x12, getByte() );
		case opcode_call_13:			return int2arg("call", 0x13, getByte() );
		case opcode_call_14:			return int2arg("call", 0x14, getByte() );
		case opcode_call_15:			return int2arg("call", 0x15, getByte() );
		case opcode_call_16:			return int2arg("call", 0x16, getByte() );
		case opcode_call_17:			return int2arg("call", 0x17, getByte() );
		case opcode_call_18:			return int2arg("call", 0x18, getByte() );
		case opcode_call_19:			return int2arg("call", 0x19, getByte() );
		case opcode_call_1A:			return int2arg("call", 0x1A, getByte() );
		case opcode_call_1B:			return int2arg("call", 0x1B, getByte() );
		case opcode_call_1C:			return int2arg("call", 0x1C, getByte() );
		case opcode_call_1D:			return int2arg("call", 0x1D, getByte() );
		case opcode_call_1E:			return int2arg("call", 0x1E, getByte() );
		case opcode_call_1F:			return int2arg("call", 0x1F, getByte() );
		
 
		
		}
		
		return intarg("? = ", opcode);
	}


	private PhantomInstruction int2arg(String name, int a1, int a2) {
		return new PhantomInstruction(current_prefix, name, a1, a2);
	}

	private PhantomInstruction stringarg(String name, byte[] arg) {
		return new PhantomInstruction(current_prefix, name, arg);
	}

	private PhantomInstruction intarg(String name, int arg) {
		return new PhantomInstruction(current_prefix, name, arg);
	}

	private PhantomInstruction simple(String name) {		
		return new PhantomInstruction(current_prefix, name);
	}

	/*private PhantomInstruction jump(String name, int arg) {
		return new PhantomInstruction(name, arg);
	}*/
	
	
	private byte getByte() {
		return data[ip++];
	}

	private int getInt() {
	    int v;

	    v = data[ip+3];
	    v |= ( (0xFF & (int)data[ip+2])) << 8;
	    v |= ( (0xFF & (int)data[ip+1])) << 16;
	    v |= ( (0xFF & (int)data[ip+0])) << 24;

	    ip += 4;
	    
	    return v;
	}

	private byte[] getString() {
		int size = getInt();
		
		byte[] ret = new byte[size];
		
		System.arraycopy(data, ip, ret, 0, size);
		
		ip += size;
		
		return ret;
	}

	private int getIp() {
		return ip;
	}

	
	public void dump()
	{
		jumpTo(0);
		while(hasNext())
		{
			System.out.print( String.format("%04X: ", getIp() ));
			System.out.println(next((byte)0).toString());
		}
	}


	public static void main(String[] args) {
		byte [] data = 
		{
				0x00, 0x00, 0x01, (byte) 0xF1, 0x00,
				0x00, 0x00, 0x0A, 0x61, 0x72, 0x72, 0x61, 0x79, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x00, 0x00, 0x00,
				0x10, 0x23, 0x00, 0x00, 0x00, 0x00, 0x45, 0x05, 0x00, 0x00, 0x00, 0x49, 0x24, 0x00, 0x00, 0x00,
				0x3F, 0x61, 0x72, 0x67, 0x20, 0x63, 0x6F, 0x75, 0x6E, 0x74, 0x3A, 0x20, 0x61, 0x72, 0x72, 0x61,
				0x79, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x20, 0x69, 0x6E, 0x20, 0x2E, 0x72, 0x75, 0x2E, 0x64, 0x7A,
				0x2E, 0x70, 0x68, 0x61, 0x6E, 0x74, 0x6F, 0x6D, 0x2E, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6D, 0x2E,
				0x72, 0x65, 0x67, 0x72, 0x65, 0x73, 0x73, 0x69, 0x6F, 0x6E, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x73,
				0x2F, 0x31, 0x24, 0x00, 0x00, 0x00, 0x13, 0x43, 0x68, 0x65, 0x63, 0x6B, 0x69, 0x6E, 0x67, 0x20,
				0x61, 0x72, 0x72, 0x61, 0x79, 0x73, 0x2E, 0x2E, 0x2E, 0x20, (byte) 0xB4, 0x01, 0x13, 0x3D, 0x18, 0x12,
				0x15, 0x04, 0x13, 0x14, 0x04, 0x24, 0x00, 0x00, 0x00, 0x04, 0x7A, 0x65, 0x72, 0x6F, 0x23, 0x00,
				0x00, 0x00, 0x00, 0x40, (byte) 0xAB, 0x02, 0x13, 0x14, 0x04, 0x24, 0x00, 0x00, 0x00, 0x03, 0x74, 0x77,
				0x6F, 0x23, 0x00, 0x00, 0x00, 0x02, 0x40, (byte) 0xAB, 0x02, 0x13, 0x14, 0x04, 0x24, 0x00, 0x00, 0x00,
				0x03, 0x6F, 0x6E, 0x65, 0x23, 0x00, 0x00, 0x00, 0x01, 0x40, (byte) 0xAB, 0x02, 0x13, 0x31, 0x24, 0x00,
				0x00, 0x00, 0x09, 0x42, 0x65, 0x65, 0x70, 0x69, 0x6E, 0x67, 0x3A, 0x20, (byte) 0xB4, 0x01, 0x13, 0x23,
				0x00, 0x00, 0x00, 0x03, 0x40, 0x12, 0x15, 0x02, 0x13, 0x14, 0x02, 0x41, 0x05, 0x00, 0x00, 0x00,
				0x3A, 0x14, 0x02, 0x41, 0x23, 0x00, 0x00, 0x00, 0x01, 0x45, 0x40, 0x12, 0x15, 0x02, 0x13, 0x31,
				0x24, 0x00, 0x00, 0x00, 0x05, 0x62, 0x65, 0x65, 0x70, 0x20, (byte) 0xB4, 0x01, 0x13, 0x31, 0x14, 0x04,
				0x14, 0x02, (byte) 0xAA, 0x01, (byte) 0xB4, 0x01, 0x13, 0x31, 0x24, 0x00, 0x00, 0x00, 0x02, 0x21, 0x20, (byte) 0xB4,
				0x01, 0x13, 0x06, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xC6, 0x31, 0x24, 0x00, 0x00, 0x00, 0x01, 0x0A, (byte) 0xB4, 0x01,
				0x13, 0x14, 0x04, 0x23, 0x00, 0x00, 0x00, 0x00, 0x40, 0x23, 0x00, 0x00, 0x00, 0x00, 0x40, (byte) 0xAB,
				0x02, 0x13, 0x14, 0x04, 0x23, 0x00, 0x00, 0x00, 0x02, 0x40, 0x23, 0x00, 0x00, 0x00, 0x02, 0x40,
				(byte) 0xAB, 0x02, 0x13, 0x14, 0x04, 0x23, 0x00, 0x00, 0x00, 0x01, 0x40, 0x23, 0x00, 0x00, 0x00, 0x01,
				0x40, (byte) 0xAB, 0x02, 0x13, 0x23, 0x00, 0x00, 0x00, 0x03, 0x40, 0x12, 0x15, 0x02, 0x13, 0x14, 0x02,				
		};
		
		new Disassembler(data).dump();
	}
	
}
