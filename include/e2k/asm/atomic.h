#ifndef _E2K_ATOMIC_
#define _E2K_ATOMIC_

//#include <linux/types.h>
#include <sys/types.h>

#include <asm/cmpxchg.h>
#include <asm/machdep.h>
#include <asm/e2k_api.h>

/*
 * Requires support in arch/e2k/lib/atomic.c
 */

#define ATOMIC_INIT(i)		{ (i) }
#define ATOMIC64_INIT(i)	{ (i) }

#define atomic_read(v)		((v)->counter)
#define atomic64_read(v)	((v)->counter)

#define atomic_set(v, i)	(((v)->counter) = i)
#define atomic64_set(v, i)	(((v)->counter) = i)

extern int printk(const char *fmt, ...);

static inline void __atomic_add(int incr, atomic_t *val)
{
	__api_atomic32_add(incr, &val->counter);
}

static inline void __atomic64_add(__s64 incr, atomic64_t *val)
{
	__api_atomic64_add(incr, &val->counter);
}

static inline void __atomic_sub(int incr, atomic_t *val)
{
	__api_atomic32_sub(incr, &val->counter);
}

static inline void __atomic64_sub(__s64 incr, atomic64_t *val)
{
	__api_atomic64_sub(incr, &val->counter);
}


static inline int __atomic_add_return(int incr, atomic_t *val)
{
	int counter;
	counter = __api_atomic32_add_return(incr, &val->counter);
	return counter;
	
}

static inline long __atomic64_add_return(__s64 incr, atomic64_t *val)
{
	long counter;
	counter = __api_atomic64_add_return(incr, &val->counter);
	return counter;
}

static inline int __atomic_sub_return(int incr, atomic_t *val)
{
	int counter;
	counter = __api_atomic32_sub_return(incr, &val->counter);
	return counter;
}

static inline long __atomic64_sub_return(__s64 incr, atomic64_t *val)
{
	long counter;
	counter = __api_atomic64_sub_return(incr, &val->counter);
	return counter;
}

static inline void
__atomic_set_mask(unsigned long mask, atomic_t *val)
{
	__api_atomic32_set_mask(mask, &val->counter);
}

static inline void
__atomic_clear_mask(unsigned long mask, atomic_t *val)
{
	__api_atomic32_clear_mask(mask, &val->counter);
}


#define atomic_add(i, v) __atomic_add(i, v)
#define atomic64_add(i, v) __atomic64_add(i, v)

#define atomic_sub(i, v) __atomic_sub(i, v)
#define atomic64_sub(i, v) __atomic64_sub(i, v)

#define atomic_dec_return(v) __atomic_sub_return(1, v)
#define atomic64_dec_return(v) __atomic64_sub_return(1, v)

#define atomic_inc_return(v) __atomic_add_return(1, v)
#define atomic64_inc_return(v) __atomic64_add_return(1, v)
#define atomic_inc_and_test(v) (atomic_add_return(1, (v)) == 0)
#define atomic64_inc_and_test(v) (atomic64_add_return(1, (v)) == 0)

#define atomic_sub_and_test(i, v) (__atomic_sub_return(i, v) == 0)
#define atomic64_sub_and_test(i, v) (__atomic64_sub_return(i, v) == 0)

#define atomic_dec_and_test(v) (__atomic_sub_return(1, v) == 0)
#define atomic64_dec_and_test(v) (__atomic64_sub_return(1, v) == 0)

#define atomic_inc(v) __atomic_add(1, v)
#define atomic64_inc(v) __atomic64_add(1, v)

#define atomic_dec(v) __atomic_sub(1, v)
#define atomic64_dec(v) __atomic64_sub(1, v)

#define atomic_add_return(i, v)	__atomic_add_return(i, v)
#define atomic64_add_return(i, v) __atomic64_add_return(i, v)
#define atomic_sub_return(i, v) __atomic_sub_return(i, v)
#define atomic64_sub_return(i, v) __atomic64_sub_return(i, v)

#define atomic_set_mask(i, v)	__atomic_set_mask(i, v)
#define atomic_clear_mask(i, v) __atomic_clear_mask(i, v)

#define atomic_cmpxchg(v, old, new) (cmpxchg(&((v)->counter), old, new))
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))

#define atomic64_cmpxchg(v, old, new) (cmpxchg(&((v)->counter), old, new))
#define atomic64_xchg(v, new) (xchg(&((v)->counter), new))

/*
 * Atomically add I to V and return TRUE if the resulting value is
 * negative.
 */
static __inline__ int
atomic_add_negative (int i, atomic_t *v)
{
	return __atomic_add_return(i, v) < 0;
}
static __inline__ int
atomic64_add_negative (__s64 i, atomic64_t *v)
{
	return __atomic64_add_return(i, v) < 0;
}

/**
 * __atomic_add_unless - add unless the number is already a given value
 * @v: pointer of type atomic_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as @v was not already @u.
 * Returns the old value of @v
 */
static inline int __atomic_add_unless(atomic_t *v, int a, int u)
{
	return __api_atomic32_add_unless_return(a, &v->counter, u);
}

#define atomic_inc_not_zero(v)	atomic_add_unless((v), 1, 0)

/**
 * atomic64_add_unless - add unless the number is a given value
 * @v: pointer of type atomic64_t
 * @a: the amount to add to v...
 * @u: ...unless v is equal to u.
 *
 * Atomically adds @a to @v, so long as it was not @u.
 * Returns non-zero if @v was not @u, and zero otherwise.
 */
static inline int atomic64_add_unless(atomic64_t *v, long a, long u)
{
	return u != __api_atomic64_add_unless_return(a, &v->counter, u);
}

#define atomic64_inc_not_zero(v) atomic64_add_unless((v), 1, 0)


/* Atomic operations are serializing since e2s */
#if !defined CONFIG_E2K_MACHINE || \
		defined CONFIG_E2K_E3M || \
		defined CONFIG_E2K_E3M_IOHUB || \
		defined CONFIG_E2K_E3S || \
		defined CONFIG_E2K_ES2_DSP || \
		defined CONFIG_E2K_ES2_RU
#define smp_mb__after_atomic_dec()	smp_wmb()
#define smp_mb__after_atomic_inc()	smp_wmb()
#else
#define smp_mb__after_atomic_dec()	barrier()
#define smp_mb__after_atomic_inc()	barrier()
#endif

#define smp_mb__before_atomic_dec()	barrier()
#define smp_mb__before_atomic_inc()	barrier()

#include <asm-generic/atomic-long.h>

#endif /* _E2K_ATOMIC_ */
