/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * HelenOS compatibility. See http://www.helenos.org
 *
**/

#ifndef __COMPAT_HELENOS_H__
#define __COMPAT_HELENOS_H__

#include <assert.h>

#include <phantom_libc.h>
#include <phantom_types.h>
#include <hal.h>
#include <time.h>

#include <kernel/cond.h>
#include <kernel/mutex.h>


#define uint8_t u_int8_t
#define uint16_t u_int16_t
#define uint32_t u_int32_t
//#define uint64_t u_int64_t

typedef u_int64_t uint64_t;



// == error ok :)
#define EOK 0


#define true 1


#define fibril_mutex_t hal_mutex_t

#define fibril_mutex_lock   hal_mutex_lock
#define fibril_mutex_unlock hal_mutex_unlock

#define fibril_condvar_t hal_cond_t

#define fibril_condvar_wait hal_cond_wait
#define fibril_condvar_signal hal_cond_signal




typedef struct ddf_dev
{
}
ddf_dev_t;


typedef struct ddf_fun
{
}
ddf_fun_t;


#endif // __COMPAT_HELENOS_H__
