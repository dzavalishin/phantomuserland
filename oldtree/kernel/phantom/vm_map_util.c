/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel paging/snapping subsystem helpers.
 *
 *
**/

#define DEBUG_MSG_PREFIX "pager"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include "vm_map.h"


void foreach_pause_counter_init(foreach_pause_counter *me) { me->now = foreach_pause_counter_limit; }
int  foreach_pause(foreach_pause_counter *me)
{
    me->now--;
    if( me->now <= 0 )
    {
        me->now = foreach_pause_counter_limit;
        return 1;
    }
    else return 0;
}

