//#include <mips/asm.h>

.text
.align 2
setjmp:
.global __setjmp
.ent	__setjmp
__setjmp:
	li	$5, 0		/* don't save sigmask */
.end	__setjmp
.global __sigsetjmp
.ent	__sigsetjmp
__sigsetjmp:
	/* where to jump later */
	//la	$25, __sigjmp_save
	/* callee save floating point save registers (double) */
	s.d	$f20, 56($4)	/* fs0 */
	s.d	$f22, 64($4)	/* fs1 */
	s.d	$f24, 72($4)	/* fs2 */
	s.d	$f26, 80($4)	/* fs3 */
	s.d	$f28, 88($4)	/* fs4 */
	s.d	$f30, 96($4)	/* fs6 */
	/* save coprocessor status register */
	cfc1	 $2, $31
	/* callee save register */
	sw	$gp, 44($4)
	sw	 $2, 48($4)
	sw	$31,  0($4)	/* ra */
	sw	$29,  4($4)	/* sp */
	sw	$16,  8($4)	/* s0 */
	sw	$17, 12($4)	/* s1 */
	sw	$18, 16($4)	/* s2 */
	sw	$19, 20($4)	/* s3 */
	sw	$20, 24($4)	/* s4 */
	sw	$21, 28($4)	/* s5 */
	sw	$22, 32($4)	/* s6 */
	sw	$23, 36($4)	/* s7 */
	sw	$30, 40($4)	/* fp */

    // return 0
    addiu $2, $0, 0

	jr	$31 // return
.end	__sigsetjmp
.weak	setjmp
