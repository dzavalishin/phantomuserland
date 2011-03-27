#ifndef ARCH_LIMITS_H
#define ARCH_LIMITS_H

//#include <endian.h>

#define CHAR_BIT 8

#define SCHAR_MIN (-128)
#define SCHAR_MAX 0x7f
#define UCHAR_MAX 0xff

#define SHRT_MIN (-SHRT_MAX-1)
#define SHRT_MAX 0x7fff
#define USHRT_MAX 0xffff

#define INT_MIN (-INT_MAX-1)
#define INT_MAX 0x7fffffff
#define UINT_MAX 0xffffffff

#if __WORDSIZE == 64
#define LONG_MAX 9223372036854775807L
#define ULONG_MAX 18446744073709551615UL
#else
#define LONG_MAX 2147483647L
#define ULONG_MAX 4294967295UL
#endif
#define LONG_MIN (-LONG_MAX - 1L)

#define LLONG_MAX 9223372036854775807LL
#define LLONG_MIN (-LLONG_MAX - 1LL)

	/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0.)  */
#define ULLONG_MAX 18446744073709551615ULL
#define ULONG_LONG_MAX ULLONG_MAX

#define SSIZE_MIN INT_MIN
#define SSIZE_MAX INT_MAX


#if !defined(_ANSI_SOURCE)
#define	SSIZE_MAX	INT_MAX		/* max value for a ssize_t */

#if !defined(_POSIX_SOURCE)
#define	SIZE_T_MAX	UINT_MAX	/* max value for a size_t */

/* GCC requires that quad constants be written as expressions. */
#define	UQUAD_MAX	((u_quad_t)0-1)	/* max value for a uquad_t */
					/* max value for a quad_t */
#define	QUAD_MAX	((quad_t)(UQUAD_MAX >> 1))
#define	QUAD_MIN	(-QUAD_MAX-1)	/* min value for a quad_t */

#endif /* !_POSIX_SOURCE */
#endif /* !_ANSI_SOURCE */



#endif // ARCH_LIMITS_H

