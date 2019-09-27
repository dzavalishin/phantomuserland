#define opcode_nop   0x00 
#define opcode_debug   0x01 
//id(opcode_skipz,0x02)  // not impl - and will not be...
//id(opcode_skipnz,0x03) // not impl - and will not be...
#define opcode_djnz   0x04 
#define opcode_jz   0x05 
#define opcode_jmp   0x06 
#define opcode_switch   0x07 
#define opcode_ret   0x08 
#define opcode_short_call_0   0x09  // 0 parameters shortcut calls
#define opcode_short_call_1   0x0A 
#define opcode_short_call_2   0x0B 
#define opcode_short_call_3   0x0C 
#define opcode_call_8bit   0x0D 
#define opcode_call_32bit   0x0E 
#define opcode_sys_8bit   0x0F 
#define opcode_is_dup   0x10 
#define opcode_is_drop   0x11 
#define opcode_os_dup   0x12 
#define opcode_os_drop   0x13 
#define opcode_os_load8   0x14  // load (push) this object's field on stack top
#define opcode_os_save8   0x15  // save (pop) stack top to this object's field
#define opcode_os_load32   0x16 
#define opcode_os_save32   0x17 
#define opcode_new   0x18  // create new object, class must be on stack
#define opcode_copy   0x19  // create new object, copy of stack top (just copy of data area as is) TODO do we need it?
#define opcode_os_compose32   0x1A  // n objects from ostack combine into the object. topmost is a class
#define opcode_os_decompose   0x1B  // decompose topmost object on stack
// deprecated? no, we use it
#define opcode_os_pull32   0x1C  // copy opbject n steps down the ostack on top. pull 0 is dup;
// deprecated and was not implemented
//id(opcode_os_assign32,0x1D) // copy stack top opbject n steps down the ostack. pull 0 is nop 
// this is for local vars
#define opcode_os_get32   0x1E  // get value from stack absolute-addressed slot, push on top
#define opcode_os_set32   0x1F  // pop stack top, set value in stack absolute-addressed slot
#define opcode_iconst_0   0x20 
#define opcode_iconst_1   0x21 
#define opcode_iconst_8bit   0x22 
#define opcode_iconst_32bit   0x23 
#define opcode_sconst_bin   0x24 
#define opcode_iconst_64bit   0x25 
// this is for integer local vars
#define opcode_is_get32   0x26  // get value from stack absolute-addressed slot, push on top
#define opcode_is_set32   0x27  // pop stack top, set value in stack absolute-addressed slot
#define opcode_const_pool   0x28  // int32 follows - get constant with corresponding index from object constant pool of this class
#define opcode_stack_reserve   0x29  // int8 object stack size to reserve (push zeroes), int8 integer stack size to reserve
// ? unclear how to init objects
//id(opcode_stack_init,0x2A)  // int32 const pool id, int8 object stack size to init (push data from const pool??), int8 integer stack size to init (rest of constant data)
#define opcode_cast   0x2B  // pop class, pop object, cast, push object
#define opcode_push_catcher   0x2D  // jump address folows, top of o stack - class of objects to catch
#define opcode_pop_catcher   0x2E 
#define opcode_throw   0x2F  // thow top of stack, if stack is empty - will throw special system-wide object, if on top of call stack - will kill thread in a bad way
#define opcode_summon_thread   0x30 
#define opcode_summon_this   0x31 
// 32-36
#define opcode_summon_null   0x37  // null object
#define opcode_summon_class_class   0x38 
#define opcode_summon_int_class   0x39 
#define opcode_summon_string_class   0x3A 
#define opcode_summon_interface_class   0x3B 
#define opcode_summon_code_class   0x3C 
#define opcode_summon_array_class   0x3D 
#define opcode_summon_by_name   0x3F  // string with class (or what?) name follows
#define opcode_i2o   0x40 
#define opcode_o2i   0x41 
#define opcode_isum   0x42 
#define opcode_imul   0x43 
#define opcode_isubul   0x44 
#define opcode_isublu   0x45 
#define opcode_idivul   0x46 
#define opcode_idivlu   0x47 
#define opcode_ior   0x48 
#define opcode_iand   0x49 
#define opcode_ixor   0x4A 
#define opcode_inot   0x4B 
#define opcode_log_or   0x4C 
#define opcode_log_and   0x4D 
#define opcode_log_xor   0x4E 
#define opcode_log_not   0x4F 
// TODO: iload/isave
#define opcode_is_load8   0x50  // load (push) this object's field on stack top
#define opcode_is_save8   0x51  // save (pop) stack top to this object's field
#define opcode_ige   0x52  // >=
#define opcode_ile   0x53  // <=
#define opcode_igt   0x54  // >
#define opcode_ilt   0x55  // <
#define opcode_iremul   0x56  // %
#define opcode_iremlu   0x57 
// Compare two object pointers
#define opcode_os_eq   0x58  // pointers are equal
#define opcode_os_neq   0x59  // pointers are not equal
#define opcode_os_isnull   0x5A  // pointer is null
// BUG! Duplicate!
//id(opcode_os_push_null, 0x5B)	// push null on stack
// Prefixes - modify next op operands type
#define opcode_prefix_long   0x5C  // next operation works on longs (uses 2x slots on int stack)
#define opcode_prefix_float   0x5D  // next operation works on floats (uses 1x slots on int stack)
#define opcode_prefix_double   0x5E  // next operation works on doubles (uses 2x slots on int stack)
//id(opcode_lock_this, 0x60)		// mutex in 'this' is locked, automatic unlock on return
//id(opcode_unlock_this, 0x61)	// mutex in 'this' is unlocked
#define opcode_general_lock   0x62  // mutex is locked on stack top. 
#define opcode_general_unlock   0x63  // mutex is unlocked on stack top. 
// 64-6e
#define opcode_static_invoke   0x6E  // arg int32 method ordinal, int32 n_args. stack (from top): class ptr, this, args
#define opcode_dynamic_invoke   0x6F  // no args. stack (from top): string method name, this (or null for static), n_args, args
#define opcode_ishl   0x70  // shift left
#define opcode_ishr   0x71  // shift right signed
#define opcode_ushr   0x72  // shift right unsigned
#define opcode_arg_count   0x73  // byte with number of args this func waits for - compares with int on istack, throws if not eq
#define opcode_fromi   0x74  // cast int from int stack to current (as defined by prefix) type on int stack
#define opcode_froml   0x75  // cast from long
#define opcode_fromf   0x76  // cast from float
#define opcode_fromd   0x77  // cast from double
// 73-7f
// We can't have sys shortcuts because of syscall restart needs fixed sycall instr size for restart 
//id(opcode_sys_0,0x80) // shortcut for syscall 0
//id(opcode_sys_1,0x81)
//id(opcode_sys_2,0x82)
//id(opcode_sys_3,0x83)
//id(opcode_sys_4,0x84)
//id(opcode_sys_5,0x85)
//id(opcode_sys_6,0x86)
//id(opcode_sys_7,0x87)
//id(opcode_sys_8,0x88)
//id(opcode_sys_9,0x89)
//id(opcode_sys_A,0x8A)
//id(opcode_sys_B,0x8B)
//id(opcode_sys_C,0x8C)
//id(opcode_sys_D,0x8D)
//id(opcode_sys_E,0x8E)
//id(opcode_sys_F,0x8F)
#define opcode_call_00   0xA0  // shortcut for call 0
#define opcode_call_01   0xA1 
#define opcode_call_02   0xA2 
#define opcode_call_03   0xA3 
#define opcode_call_04   0xA4 
#define opcode_call_05   0xA5 
#define opcode_call_06   0xA6 
#define opcode_call_07   0xA7 
#define opcode_call_08   0xA8 
#define opcode_call_09   0xA9 
#define opcode_call_0A   0xAA 
#define opcode_call_0B   0xAB 
#define opcode_call_0C   0xAC 
#define opcode_call_0D   0xAD 
#define opcode_call_0E   0xAE 
#define opcode_call_0F   0xAF 
#define opcode_call_10   0xB0  // shortcut for call 16
#define opcode_call_11   0xB1 
#define opcode_call_12   0xB2 
#define opcode_call_13   0xB3 
#define opcode_call_14   0xB4 
#define opcode_call_15   0xB5 
#define opcode_call_16   0xB6 
#define opcode_call_17   0xB7 
#define opcode_call_18   0xB8 
#define opcode_call_19   0xB9 
#define opcode_call_1A   0xBA 
#define opcode_call_1B   0xBB 
#define opcode_call_1C   0xBC 
#define opcode_call_1D   0xBD 
#define opcode_call_1E   0xBE 
#define opcode_call_1F   0xBF 
// c0-cf
// d0-df
// e0-ef
// f0-ff
