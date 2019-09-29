#include <arm/asm.h>


// TODO float?!
#warning float
/*
 * setjump + longjmp
 */
ENTRY(setjmp_machdep)
	/* Store fp registers * /
	sfm	f4, 4, [r0], #48
	// Store fpsr 
	rfs	r1
	str	r1, [r0], #0x0004
	*/

	stmia	r0, {r4-r14}
	mov	r0, #0x00000000
	RET

ENTRY(longjmp_machdep)
	/* Restore fp registers * /
	lfm	f4, 4, [r0], #48
	// Restore fpsr 
	ldr	r4, [r0], #0x0004
	wfs	r4
	*/

	ldmia	r0, {r4-r14}

	mov	r0, r1
	teq	r0, #0x00000000
	moveq	r0, #0x00000001

	RET
