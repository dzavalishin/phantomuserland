/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * GC refcount saturated inc/dec.
 *
 * Kernel ready: yes
 * Preliminary: no
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

#endif // _ASM_SATURATE
