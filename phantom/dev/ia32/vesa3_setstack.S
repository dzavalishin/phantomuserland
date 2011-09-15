#include <ia32/asm.h>

// void vesa3_stack_switch(int ss, int esp)
ENTRY(vesa3_stack_switch)
	movl    (%esp),%edx         // return addr
	lss     4(%esp),%esp        // load new stack
	jmp     *%edx               // ret

