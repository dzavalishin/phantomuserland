/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Integer-derived types.
 *
 *
**/



#ifndef PHANTOM_TYPES_H
#define PHANTOM_TYPES_H

#include <arch/arch-types.h>


// FreeBSD code wants these
typedef u_int64_t       uintmax_t;
typedef int64_t         intmax_t;
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;


typedef	u_int64_t   	u_quad_t;	/* quads (deprecated) */
typedef	int64_t		quad_t;


#ifndef __cplusplus
typedef u_int8_t        bool;
#endif


typedef u_int32_t 	disk_page_no_t;

// physical mem page no - used as physaddr! fix?
typedef u_int32_t       phys_page_t;






// next are for Unix emulation env
typedef unsigned int 	_dev_t;
typedef unsigned int 	dev_t;
typedef unsigned int 	_ino_t;
typedef unsigned int 	ino_t;
typedef int		_pid_t;
typedef _pid_t		pid_t;
typedef int		tid_t;
typedef unsigned short 	_mode_t;
typedef _mode_t		mode_t;
typedef int		_sigset_t;
typedef _sigset_t	sigset_t;




// time

typedef u_int64_t   	bigtime_t;

#ifndef _TIME_T
#ifndef _TIME_T_DECLARED

typedef	u_int64_t	time_t;

#define	_TIME_T_DECLARED
#define	_TIME_T

#endif
#endif






#endif // PAGER_TYPES_H
