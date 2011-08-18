#ifndef _I386_BITOPS_H
#define _I386_BITOPS_H

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif


/*
 * Copyright 1992, Linus Torvalds.
 */

/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

//#ifdef __SMP__
#define LOCK_PREFIX "lock ; "
//#else
//#define LOCK_PREFIX ""
//#endif

/*
 * Some hacks to defeat gcc over-optimizations..
 */
struct __dummy { unsigned long a[100]; };
#define ADDR (*(struct __dummy *) addr)
#define CONST_ADDR (*(const struct __dummy *) addr)

extern __inline__ int set_bit(int nr, void * addr)
{
	int oldbit;

	__asm__ __volatile__(LOCK_PREFIX
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"ir" (nr));
	return oldbit;
}

extern __inline__ int clear_bit(int nr, void * addr)
{
	int oldbit;

	__asm__ __volatile__(LOCK_PREFIX
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"ir" (nr));
	return oldbit;
}

extern __inline__ int change_bit(int nr, void * addr)
{
	int oldbit;

	__asm__ __volatile__(LOCK_PREFIX
		"btcl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"ir" (nr));
	return oldbit;
}

/*
 * This routine doesn't need to be atomic.
 */
extern __inline__ int test_bit(int nr, const void * addr)
{
	return 1UL & (((const unsigned int *) addr)[nr >> 5] >> (nr & 31));
}

/*
 * Find-bit routines..
 */
extern __inline__ int find_first_zero_bit(void * addr, unsigned size)
{
	int res;

	if (!size)
		return 0;
	__asm__("
		cld
		movl $-1,%%eax
		xorl %%edx,%%edx
		repe; scasl
		je 1f
		xorl -4(%%edi),%%eax
		subl $4,%%edi
		bsfl %%eax,%%edx
1:		subl %%ebx,%%edi
		shll $3,%%edi
		addl %%edi,%%edx"
		:"=d" (res)
		:"c" ((size + 31) >> 5), "D" (addr), "b" (addr)
		:"ax", "cx", "di");
	return res;
}

extern __inline__ int find_next_zero_bit (void * addr, int size, int offset)
{
	unsigned long * p = ((unsigned long *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;
	
	if (bit) {
		/*
		 * Look for zero in first byte
		 */
		__asm__("
			bsfl %1,%0
			jne 1f
			movl $32, %0
1:			"
			: "=r" (set)
			: "r" (~(*p >> bit)));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No zero yet, search remaining full bytes for a zero
	 */
	res = find_first_zero_bit (p, size - 32 * (p - (unsigned long *) addr));
	return (offset + set + res);
}

/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
extern __inline__ unsigned long ffz(unsigned long word)
{
	__asm__("bsfl %1,%0"
		:"=r" (word)
		:"r" (~word));
	return word;
}




extern __inline__ void ATOMIC_INCR(void * addr)
{
	__asm__ __volatile__(
		"incl %0"
		:"=m" (ADDR));
}

extern __inline__ void ATOMIC_DECR(void * addr)
{
	__asm__ __volatile__(
		"decl %0"
		:"=m" (ADDR));
}


#endif /* _I386_BITOPS_H */
