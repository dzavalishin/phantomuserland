package phantom.code;
public class opcode_ids {
protected static final byte opcode_nop = (byte)0x00;
protected static final byte opcode_debug = (byte)0x01;
//id(opcode_skipz,0x02)  // not impl - and will not be...
//id(opcode_skipnz,0x03) // not impl - and will not be...
protected static final byte opcode_djnz = (byte)0x04;
protected static final byte opcode_jz = (byte)0x05;
protected static final byte opcode_jmp = (byte)0x06;
protected static final byte opcode_switch = (byte)0x07;
protected static final byte opcode_ret = (byte)0x08;
protected static final byte opcode_short_call_0 = (byte)0x09; // 0 parameters shortcut calls
protected static final byte opcode_short_call_1 = (byte)0x0A;
protected static final byte opcode_short_call_2 = (byte)0x0B;
protected static final byte opcode_short_call_3 = (byte)0x0C;
protected static final byte opcode_call_8bit = (byte)0x0D;
protected static final byte opcode_call_32bit = (byte)0x0E;
protected static final byte opcode_sys_8bit = (byte)0x0F;
protected static final byte opcode_is_dup = (byte)0x10;
protected static final byte opcode_is_drop = (byte)0x11;
protected static final byte opcode_os_dup = (byte)0x12;
protected static final byte opcode_os_drop = (byte)0x13;
protected static final byte opcode_os_load8 = (byte)0x14; // load (push) this object's field on stack top
protected static final byte opcode_os_save8 = (byte)0x15; // save (pop) stack top to this object's field
protected static final byte opcode_os_load32 = (byte)0x16;
protected static final byte opcode_os_save32 = (byte)0x17;
protected static final byte opcode_new = (byte)0x18; // create new object, class must be on stack
protected static final byte opcode_copy = (byte)0x19; // create new object, copy of stack top (just copy of data area as is)
protected static final byte opcode_os_compose32 = (byte)0x1A; // n objects from ostack combine into the object. topmost is a class
protected static final byte opcode_os_decompose = (byte)0x1B; // decompose topmost object on stack
// deprecated?
protected static final byte opcode_os_pull32 = (byte)0x1C; // copy opbject n steps down the ostack on top. pull 0 is dup;
// deprecated and was not implemented
//id(opcode_os_assign32,0x1D) // copy stack top opbject n steps down the ostack. pull 0 is nop;
// this is for local vars
protected static final byte opcode_os_get32 = (byte)0x1E; // get value from stack absolute-addressed slot, push on top
protected static final byte opcode_os_set32 = (byte)0x1F; // pop stack top, set value in stack absolute-addressed slot
protected static final byte opcode_iconst_0 = (byte)0x20;
protected static final byte opcode_iconst_1 = (byte)0x21;
protected static final byte opcode_iconst_8bit = (byte)0x22;
protected static final byte opcode_iconst_32bit = (byte)0x23;
protected static final byte opcode_sconst_bin = (byte)0x24;
protected static final byte opcode_iconst_64bit = (byte)0x25;
// this is for integer local vars
protected static final byte opcode_is_get32 = (byte)0x26; // get value from stack absolute-addressed slot, push on top
protected static final byte opcode_is_set32 = (byte)0x27; // pop stack top, set value in stack absolute-addressed slot
protected static final byte opcode_const_pool = (byte)0x28; // int32 follows - get constant with corresponding index from object constant pool of this class
protected static final byte opcode_cast = (byte)0x2B; // pop class, pop object, cast, push object
protected static final byte opcode_push_catcher = (byte)0x2D; // jump address folows, top of o stack - class of objects to catch
protected static final byte opcode_pop_catcher = (byte)0x2E;
protected static final byte opcode_throw = (byte)0x2F; // thow top of stack, if stack is empty - will throw special system-wide object, if on top of call stack - will kill thread in a bad way
protected static final byte opcode_summon_thread = (byte)0x30;
protected static final byte opcode_summon_this = (byte)0x31;
// 32-36
protected static final byte opcode_summon_null = (byte)0x37; // null object
protected static final byte opcode_summon_class_class = (byte)0x38;
protected static final byte opcode_summon_int_class = (byte)0x39;
protected static final byte opcode_summon_string_class = (byte)0x3A;
protected static final byte opcode_summon_interface_class = (byte)0x3B;
protected static final byte opcode_summon_code_class = (byte)0x3C;
protected static final byte opcode_summon_array_class = (byte)0x3D;
protected static final byte opcode_summon_by_name = (byte)0x3F; // string with class (or what?) name follows
protected static final byte opcode_i2o = (byte)0x40;
protected static final byte opcode_o2i = (byte)0x41;
protected static final byte opcode_isum = (byte)0x42;
protected static final byte opcode_imul = (byte)0x43;
protected static final byte opcode_isubul = (byte)0x44;
protected static final byte opcode_isublu = (byte)0x45;
protected static final byte opcode_idivul = (byte)0x46;
protected static final byte opcode_idivlu = (byte)0x47;
protected static final byte opcode_ior = (byte)0x48;
protected static final byte opcode_iand = (byte)0x49;
protected static final byte opcode_ixor = (byte)0x4A;
protected static final byte opcode_inot = (byte)0x4B;
protected static final byte opcode_log_or = (byte)0x4C;
protected static final byte opcode_log_and = (byte)0x4D;
protected static final byte opcode_log_xor = (byte)0x4E;
protected static final byte opcode_log_not = (byte)0x4F;
// TODO: iload/isave
protected static final byte opcode_is_load8 = (byte)0x50; // load (push) this object's field on stack top
protected static final byte opcode_is_save8 = (byte)0x51; // save (pop) stack top to this object's field
protected static final byte opcode_ige = (byte)0x52; // >=
protected static final byte opcode_ile = (byte)0x53; // <=
protected static final byte opcode_igt = (byte)0x54; // >
protected static final byte opcode_ilt = (byte)0x55; // <
protected static final byte opcode_iremul = (byte)0x56; // %
protected static final byte opcode_iremlu = (byte)0x57;
// Compare two object pointers
protected static final byte opcode_os_eq = (byte)0x58; // pointers are equal
protected static final byte opcode_os_neq = (byte)0x59; // pointers are not equal
protected static final byte opcode_os_isnull = (byte)0x5A; // pointer is null
// BUG! Duplicate!
//id(opcode_os_push_null, 0x5B)	// push null on stack
// Prefixes - modify next op operands type
protected static final byte opcode_prefix_long = (byte)0x5C; // next operation works on longs (uses 2x slots on int stack)
protected static final byte opcode_prefix_float = (byte)0x5D; // next operation works on floats (uses 1x slots on int stack)
protected static final byte opcode_prefix_double = (byte)0x5E; // next operation works on doubles (uses 2x slots on int stack)
//id(opcode_lock_this, 0x60)		// mutex in 'this' is locked, automatic unlock on return
//id(opcode_unlock_this, 0x61)	// mutex in 'this' is unlocked
protected static final byte opcode_general_lock = (byte)0x62; // mutex is locked on stack top. 
protected static final byte opcode_general_unlock = (byte)0x63; // mutex is unlocked on stack top. 
// 64-6e
protected static final byte opcode_static_invoke = (byte)0x6E; // arg int32 method ordinal. stack (from top): class ptr, this, n_args, args
protected static final byte opcode_dynamic_invoke = (byte)0x6F; // no args. stack (from top): string method name, this (or null for static), n_args, args
protected static final byte opcode_ishl = (byte)0x70; // shift left
protected static final byte opcode_ishr = (byte)0x71; // shift right signed
protected static final byte opcode_ushr = (byte)0x72; // shift right unsigned
// no 73 yet
protected static final byte opcode_fromi = (byte)0x74; // cast int from int stack to current (as defined by prefix) type on int stack
protected static final byte opcode_froml = (byte)0x75; // cast from long
protected static final byte opcode_fromf = (byte)0x76; // cast from float
protected static final byte opcode_fromd = (byte)0x77; // cast from double
// 73-7f
// TODO kill shortcuts for we will have JIT anyway and bytecode size does not matter
protected static final byte opcode_sys_0 = (byte)0x80; // shortcut for syscall 0
protected static final byte opcode_sys_1 = (byte)0x81;
protected static final byte opcode_sys_2 = (byte)0x82;
protected static final byte opcode_sys_3 = (byte)0x83;
protected static final byte opcode_sys_4 = (byte)0x84;
protected static final byte opcode_sys_5 = (byte)0x85;
protected static final byte opcode_sys_6 = (byte)0x86;
protected static final byte opcode_sys_7 = (byte)0x87;
protected static final byte opcode_sys_8 = (byte)0x88;
protected static final byte opcode_sys_9 = (byte)0x89;
protected static final byte opcode_sys_A = (byte)0x8A;
protected static final byte opcode_sys_B = (byte)0x8B;
protected static final byte opcode_sys_C = (byte)0x8C;
protected static final byte opcode_sys_D = (byte)0x8D;
protected static final byte opcode_sys_E = (byte)0x8E;
protected static final byte opcode_sys_F = (byte)0x8F;
protected static final byte opcode_call_00 = (byte)0xA0; // shortcut for call 0
protected static final byte opcode_call_01 = (byte)0xA1;
protected static final byte opcode_call_02 = (byte)0xA2;
protected static final byte opcode_call_03 = (byte)0xA3;
protected static final byte opcode_call_04 = (byte)0xA4;
protected static final byte opcode_call_05 = (byte)0xA5;
protected static final byte opcode_call_06 = (byte)0xA6;
protected static final byte opcode_call_07 = (byte)0xA7;
protected static final byte opcode_call_08 = (byte)0xA8;
protected static final byte opcode_call_09 = (byte)0xA9;
protected static final byte opcode_call_0A = (byte)0xAA;
protected static final byte opcode_call_0B = (byte)0xAB;
protected static final byte opcode_call_0C = (byte)0xAC;
protected static final byte opcode_call_0D = (byte)0xAD;
protected static final byte opcode_call_0E = (byte)0xAE;
protected static final byte opcode_call_0F = (byte)0xAF;
protected static final byte opcode_call_10 = (byte)0xB0; // shortcut for call 16
protected static final byte opcode_call_11 = (byte)0xB1;
protected static final byte opcode_call_12 = (byte)0xB2;
protected static final byte opcode_call_13 = (byte)0xB3;
protected static final byte opcode_call_14 = (byte)0xB4;
protected static final byte opcode_call_15 = (byte)0xB5;
protected static final byte opcode_call_16 = (byte)0xB6;
protected static final byte opcode_call_17 = (byte)0xB7;
protected static final byte opcode_call_18 = (byte)0xB8;
protected static final byte opcode_call_19 = (byte)0xB9;
protected static final byte opcode_call_1A = (byte)0xBA;
protected static final byte opcode_call_1B = (byte)0xBB;
protected static final byte opcode_call_1C = (byte)0xBC;
protected static final byte opcode_call_1D = (byte)0xBD;
protected static final byte opcode_call_1E = (byte)0xBE;
protected static final byte opcode_call_1F = (byte)0xBF;
// c0-cf
// d0-df
// e0-ef
// f0-ff
}
