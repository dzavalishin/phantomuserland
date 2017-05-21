#ifndef ASM_E2K_CMPXCHG_H
#define ASM_E2K_CMPXCHG_H

//#include <linux/compiler.h>
#include <asm/machdep.h>
#include <asm/e2k_api.h>

/*
 * Non-existant functions to indicate usage errors at link time
 * (or compile-time if the compiler implements __compiletime_error().
 */
extern void __xchg_wrong_size(void)
	__compiletime_error("Bad argument size for xchg");
extern void __cmpxchg_wrong_size(void)
	__compiletime_error("Bad argument size for cmpxchg");


static inline unsigned long
__xchg_b(unsigned long x, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_xchg8_return(x, ptr);
	return prev;
}
static inline unsigned long
__xchg_h(unsigned long x, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_xchg16_return(x, ptr);
	return prev;
}
static inline unsigned long
__xchg_w(unsigned long x, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_xchg32_return(x, ptr);
	return prev;
}
static inline unsigned long
__xchg_d(unsigned long x, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_xchg64_return(x, ptr);
	return prev;
}

static inline unsigned long
__xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 1: return __xchg_b(x, ptr);
		case 2: return __xchg_h(x, ptr);
		case 4: return __xchg_w(x, ptr);
		case 8: return __xchg_d(x, ptr);
		default: __xchg_wrong_size();
	}
	return 0;
}

static inline unsigned long
__cmpxchg_b(unsigned long old, unsigned long new, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_cmpxchg8_return(old, new, ptr);
	return prev;
}
static inline unsigned long
__cmpxchg_h(unsigned long old, unsigned long new, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_cmpxchg16_return(old, new, ptr);
	return prev;
}
static inline unsigned long
__cmpxchg_w(unsigned long old, unsigned long new, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_cmpxchg32_return(old, new, ptr);
	return prev;
}
static inline unsigned long
__cmpxchg_d(unsigned long old, unsigned long new, volatile void * ptr)
{
	unsigned long	prev;
	prev = __api_cmpxchg64_return(old, new, ptr);
	return prev;
}

static inline unsigned long
__cmpxchg(volatile void *ptr, unsigned long old, unsigned long new, int size)
{
	switch (size) {
		case 1: return __cmpxchg_b(old, new, ptr);
		case 2: return __cmpxchg_h(old, new, ptr);
		case 4: return __cmpxchg_w(old, new, ptr);
		case 8: return __cmpxchg_d(old, new, ptr);
		default: __cmpxchg_wrong_size();
	}
	return 0;
}

#define xchg(ptr,v) \
	((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))

#define __HAVE_ARCH_CMPXCHG 1
#define cmpxchg(ptr,o,n) \
	((__typeof__(*(ptr)))__cmpxchg((ptr),(unsigned long)(o), \
			(unsigned long)(n),sizeof(*(ptr))))

#define cmpxchg64(ptr, o, n) \
({ \
	BUILD_BUG_ON(sizeof(*(ptr)) != 8); \
	cmpxchg((ptr), (o), (n)); \
})

static inline u64 cmpxchg64_relaxed(volatile void *ptr, u64 old, u64 new)
{
	return __api_cmpxchg_dword_return_relaxed(old, new, ptr);
}

#endif	/* ASM_E2K_CMPXCHG_H */
