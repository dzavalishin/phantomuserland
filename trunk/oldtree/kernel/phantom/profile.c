/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel profiler.
 *
 *
**/

#define DEBUG_MSG_PREFIX "profile"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/profile.h>
#include <kernel/init.h>
#include <kernel/debug.h>
#include <stdio.h>

static int profiler_inited = 0;

static profiler_entry_t *map;
static int total_count = 0;


static void profiler_dump_map_cmd(int ac, char **av)
{
    (void) ac;
    (void) av;

    profiler_dump_map();
}


static void profiler_init(void)
{
    physaddr_t pa;
    hal_pv_alloc( &pa, (void **)&map, PROFILER_MAP_SIZE*sizeof(profiler_entry_t) );

    profiler_inited = 1;

    dbg_add_command(&profiler_dump_map_cmd, "profile", "Display current profiler output");
}

// Init it later to skip profiling kernel startup, which is not too representative
INIT_ME(0,0,profiler_init)


void profiler_register_interrupt_hit( addr_t ip )
{
    if(!profiler_inited)
        return;

    ip /= PROFILER_MAP_DIVIDER;

    if( ip > PROFILER_MAP_SIZE )
    {
        static int warn_over = 0;

        if(!warn_over)
        {
            warn_over = 1;
            SHOW_ERROR( 0, "profiler ip overflow %p", ip*PROFILER_MAP_DIVIDER );
        }

        return;
    }

    map[ip]++;
    total_count++;

    if( (total_count % 10000) == 0 )
        profiler_dump_map();

}

// TODO must sum counts for map entries which correspond to one func

void profiler_dump_map( void )
{
    printf("Profile (%d events total):\n", total_count );
    int i = 0;
    for( i = 0; i < PROFILER_MAP_SIZE; i++ )
    {
        if( map[i] == 0 )
            continue;

        addr_t ip = i*PROFILER_MAP_DIVIDER;

        char *name = "?";
        if(phantom_symtab_getname)
            name = phantom_symtab_getname( (void *)ip );

        int count = map[i];

		if( (int) ( ((100*(long long)count)/total_count)) )
        printf(
               "%6p: %6d (%2d%%) - %s\n",
               ip, count, (int) ( ((100*(long long)count)/total_count) ),
               name
              );

    }
}





