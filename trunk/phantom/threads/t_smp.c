/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Threads SMP related stuff.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/

#include <thread_private.h>
#include <hal.h>
#include <threads.h>
#include <stdio.h>

#define DEBUG_MSG_PREFIX "smp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

void t_smp_enable(int yn)
{
    SHOW_FLOW( 2, "SMP %s", yn ? "on" : "off" );
    // TODO SMP on/off
}


void t_migrate_to_boot_CPU(void)
{
    t_smp_enable(0);

    if( 0 == GET_CPU_ID())
        return;

    phantom_scheduler_yield();

    while( 0 == GET_CPU_ID())
    {
        SHOW_ERROR( 0, "on CPU %d, not 0", GET_CPU_ID() );
        //hal_sleep_msec(300);
        hal_sleep_msec(1);
    }

}

