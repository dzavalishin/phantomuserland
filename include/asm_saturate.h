/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * GC refcount saturated inc/dec.
 *
 *
 *
**/

#ifndef _ASM_SATURATE
#define _ASM_SATURATE

// inc or dec int value if it is not 'all ones' - used in GC refcount

#define ASM_SATURATE_INC(iv)     asm( "addl $1, %0 \n\tsbbl $0, %0 " : "+m" (iv) : : "cc")
#define ASM_SATURATE_DEC(iv)     asm( "addl $1, %0 \n\tsubl $1, %0\n\tcmc\n\tsbbl $0, %0 " : "+m" (iv) : : "cc");

/* shorter dec from spamsink:
addl $1, %0
adcl $0, %0
subl $2, %0
*/

// newer arms have saturated add
// look for saturated add in SSE?

/*

Alternative 1:


mov refcounter, %eax
again:
mov %eax, %ebx
set carry
rcl %ebx
// if (refcounter == %eax) refcounter = %ebx else %eax = refcounter
cmpxchg %ebx, refcounter
jnz again


mov refcounter, %eax
again:
mov %eax, %ebx
sar %ebx
// if (refcounter == %eax) refcounter = %ebx else %eax = refcounter
cmpxchg %ebx, refcounter
jnz again


*/





/*

Alternative 2:

lock inc refcounter
lock and 0x0000FFFF refcounter -> refcounter


reg <- refcounter
and reg, 0xFFFFFF00
lock dec refcounter
lock or reg refcounter -> refcounter


*/

#endif // _ASM_SATURATE
