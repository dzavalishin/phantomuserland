/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * uOS compatibility.
 *
**/

#ifndef __COMPAT_UOS_H__
#define __COMPAT_UOS_H__



typedef int bool_t;


#define outb_reverse( val, port ) outb((port), (val))
#define outw_reverse( val, port ) outw((port), (val))
#define outl_reverse( val, port ) outl((port), (val))


#endif // __COMPAT_UOS_H__