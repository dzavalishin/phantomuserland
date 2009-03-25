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
typedef	int			  int32_t;
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

typedef	int32_t		ptrdiff_t;


typedef u_int8_t        bool;



typedef u_int32_t 		disk_page_no_t;

// physical mem page no
typedef u_int32_t       	phys_page_t;

typedef void *  		vmem_ptr_t;

#ifndef _SIZE_T
#define _SIZE_T
typedef int			size_t;
#endif //_SIZE_T

typedef	u_int32_t		uintptr_t;

// time

typedef u_int64_t bigtime_t;

#ifndef _TIME_T_DECLARED
typedef	u_int64_t	time_t;
#define	_TIME_T_DECLARED
#endif




#endif // PAGER_TYPES_H
