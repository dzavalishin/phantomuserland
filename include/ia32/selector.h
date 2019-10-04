/*
** Copyright 2002, Michael Noisternig. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _IA32_SELECTOR_H_
#define _IA32_SELECTOR_H_

//#include <arch/i386/types.h>
#include <sys/types.h>

//typedef u_int32_t selector_id;
typedef u_int16_t selector_id;

selector_id alloc_ldt_selector(void);
void free_ldt_selector(selector_id sel);

#define NULL_SELECTOR   0


#define SELECTOR_LDT    0x4 
#define SELECTOR_USER   0x3

#define SELECTOR_IS_LDT(__sel) ( __sel & 0x4 )
#define SELECTOR_IS_USER(__sel) ( (__sel & 0x3) == 0x3 )



errno_t arch_ia32_modify_ds_limit( bool on_off);


#if 0
typedef u_int64_t selector_type;

// DATA segments are read-only
// CODE segments are execute-only
// both can be modified by using the suffixed enum versions
// legend:	w = writable
//			d = expand down
//			r = readable
//			c = conforming
enum segment_type {
	DATA = 0x8, DATA_w, DATA_d, DATA_wd, CODE, CODE_r, CODE_c, CODE_rc
};

#define SELECTOR(base,limit,type,mode32) \
	(((u_int64_t)(((((u_int32_t)base)>>16)&0xff) | (((u_int32_t)base)&0xff000000) | ((type)<<9) | ((mode32)<<22) | (1<<15))<<32) \
	| ( (limit) >= (1<<20) ? (((limit)>>12)&0xffff) | ((u_int64_t)(((limit)>>12)&0xf0000)<<32) | ((u_int64_t)1<<(23+32)) : ((limit)&0xffff) | ((u_int64_t)((limit)&0xf0000)<<32) ) \
	| ((((u_int32_t)base)&0xffff)<<16))

void			i386_selector_init( void *gdt );
selector_id		i386_selector_add( selector_type selector );
void			i386_selector_remove( selector_id id );
selector_type           i386_selector_get( selector_id id );
#endif

#endif // _IA32_SELECTOR_H_

