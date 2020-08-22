#include <arm/asm.h>

#warning hack! untested, correct? 

ENTRY(raise)
	mov	r0, #8			/* SIGFPE */
	bl	PIC_SYM(_C_LABEL(raise), PLT)	/* raise it */
	mov	r0, #0
	RET
