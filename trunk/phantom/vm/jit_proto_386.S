#ifdef ARCH_ia32

#include <ia32/asm.h>

ENTRY(jit_proto_get_null)
    movl $0, %eax
    movl $0, %edx
ENTRY(jit_proto_get_null_end)


ENTRY(jit_proto_get_this)
    //movl %ebx, %eax
    //movl %edi, %edx
    //call EXT(jit_helper_get_iface)
    //mov %eax, %edx

    // NB! This is C object structure hardcode!
    movl    	0x10(%ebx),%edx
    addl    	$0x20,%edx
    movl    	0x8(%edx),%edx

    movl 	%ebx, %eax
ENTRY(jit_proto_get_this_end)

ENTRY(jit_proto_isnull_ax)
	ret
ENTRY(jit_proto_isnull_ax_end)

/*
ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)

ENTRY(jit_proto_)
	ret
ENTRY(jit_proto__end)
*/

ENTRY(jit_proto_get_class_class)
	ret
ENTRY(jit_proto_get_class_class_end)

ENTRY(jit_proto_get_iface_class)
	ret
ENTRY(jit_proto_get_iface_class_end)

ENTRY(jit_proto_get_code_class)
	ret
ENTRY(jit_proto_get_code_class_end)

ENTRY(jit_proto_get_int_class)
	ret
ENTRY(jit_proto_get_int_class_end)

ENTRY(jit_proto_get_string_class)
	ret
ENTRY(jit_proto_get_string_class_end)

ENTRY(jit_proto_get_array_class)
	ret
ENTRY(jit_proto_get_array_class_end)
        


#endif // ARCH_ia32
