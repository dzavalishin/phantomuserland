#if 0

/* int atomic_add(int *val, int incr) */


/* int atomic_and(int *val, int incr) */

/* int atomic_or(int *val, int incr) */

/* int atomic_set(int *val, int set_to) */

atomic_set_32(volatile uint32_t *address, uint32_t setmask)
{
	__with_interrupts_disabled(*address |= setmask);
	
}


/* int test_and_set(int *val, int set_to, int test_val) */









#include <sys/types.h>

#ifndef I32_bit
#define I32_bit (1 << 7)        /* IRQ disable */
#endif
#ifndef F32_bit
#define F32_bit (1 << 6)        /* FIQ disable */
#endif

#define __with_interrupts_disabled(expr) \
	do {						\
		u_int cpsr_save, tmp;			\
							\
		__asm __volatile(			\
			"mrs  %0, cpsr;"		\
			"orr  %1, %0, %2;"		\
			"msr  cpsr_all, %1;"		\
			: "=r" (cpsr_save), "=r" (tmp)	\
			: "I" (I32_bit | F32_bit)		\
		        : "cc" );		\
		(expr);				\
		 __asm __volatile(		\
			"msr  cpsr_all, %0"	\
			: /* no output */	\
			: "r" (cpsr_save)	\
			: "cc" );		\
	} while(0)




#endif
