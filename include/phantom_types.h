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



typedef u_int32_t 		disk_page_no_t;

// physical mem page no
typedef u_int32_t       	phys_page_t;

typedef void *  		vmem_ptr_t;




#endif // PAGER_TYPES_H
