/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Integer-derived types.
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#ifndef PHANTOM_TYPES_H
#define PHANTOM_TYPES_H

// Define standard bit-sized stuff

#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

typedef	__signed char		   int8_t;
typedef	unsigned char		 u_int8_t;
typedef	short			  int16_t;
typedef	unsigned short		u_int16_t;

#ifndef __INT32_DEFINED__
#define __INT32_DEFINED__
typedef	int			  int32_t;
#endif

typedef	unsigned int		u_int32_t;

typedef	long long		  int64_t;
typedef	unsigned long long	u_int64_t;

#endif

// FreeBSD code wants these
typedef u_int64_t uintmax_t;
typedef int64_t intmax_t;
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;


typedef	u_int64_t	u_quad_t;	/* quads (deprecated) */
typedef	int64_t		quad_t;


#ifndef __cplusplus
typedef u_int8_t        bool;
#endif


typedef u_int32_t 		disk_page_no_t;

// physical mem page no - used as physaddr! fix?
typedef u_int32_t       	phys_page_t;

// physical mem address
typedef u_int32_t       	physaddr_t;
// linear mem address
typedef u_int32_t       	linaddr_t;


typedef void *  		vmem_ptr_t;
typedef unsigned long 	addr_t;
//typedef void *  		addr_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int		size_t;
typedef int			ssize_t;
#endif //_SIZE_T

#ifndef _OFF_T
#define _OFF_T
typedef int			off_t;
typedef int			_off_t;
#endif //_OFF_T


// next are for Unix emulation env
typedef unsigned int _dev_t;
typedef unsigned int dev_t;
typedef unsigned int _ino_t;
typedef unsigned int ino_t;
typedef int	_pid_t;
typedef _pid_t	pid_t;
typedef unsigned short _mode_t;
typedef _mode_t	mode_t;
typedef int	_sigset_t;
typedef _sigset_t	sigset_t;



#ifndef _STDINT_H
typedef	u_int32_t		uintptr_t;
typedef	int32_t			ptrdiff_t;
typedef u_int32_t 		register_t;
#endif

// time

typedef u_int64_t bigtime_t;

#ifndef _TIME_T
#ifndef _TIME_T_DECLARED

typedef	u_int64_t	time_t;

#define	_TIME_T_DECLARED
#define	_TIME_T

#endif
#endif


// ------------ From Mach

/*
 * A natural_t is the type for the native
 * integer type, e.g. 32 or 64 or.. whatever
 * register size the machine has.  Unsigned, it is
 * used for entities that might be either
 * unsigned integers or pointers, and for
 * type-casting between the two.
 * For instance, the IPC system represents
 * a port in user space as an integer and
 * in kernel space as a pointer.
 */
typedef unsigned int        natural_t;


/*
 * A vm_offset_t is a type-neutral pointer,
 * e.g. an offset into a virtual memory space.
 */
typedef        natural_t        vm_offset_t;

/*
 * A vm_size_t is the proper type for e.g.
 * expressing the difference between two
 * vm_offset_t entities.
 */
typedef        natural_t        vm_size_t;



#endif // PAGER_TYPES_H
