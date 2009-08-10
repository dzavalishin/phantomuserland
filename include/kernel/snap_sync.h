/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#include <vm/internal_da.h>


void phantom_thread_wait_4_snap( void );
void phantom_snapper_wait_4_threads( void );
void phantom_snapper_reenable_threads( void );

void phantom_snap_threads_interlock_init( void );

extern volatile int     phantom_virtual_machine_snap_request;
extern volatile int     phantom_virtual_machine_stop_request; // Is one (with the phantom_virtual_machine_snap_request) when threads are asked to do harakiri


void phantom_finish_all_threads(void);
void activate_all_threads();


void phantom_thread_sleep_worker( struct data_area_4_thread *thda );
// Can be called from SYS code only
void phantom_thread_put_asleep( struct data_area_4_thread *thda );
void phantom_thread_wake_up( struct data_area_4_thread *thda );


typedef struct userland_sleep
{
    int         is_sleeping;
    // mutex to interlock sema on/off/check
} userland_sleep_t;

