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

#define DEBUG_MSG_PREFIX "profiler"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/profile.h>
#include <kernel/init.h>
#include <kernel/debug.h>
#include <stdio.h>
#include <hal.h>

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
#if 1
    // TODO wrong - loop through all CPUs or do it on percpu (on ia32 == APIC) timer.
    // TODO In fact we must do it on regular timer interrupt - this one costs too much
    int cpu = GET_CPU_ID();

    int idle = percpu_idle_status[cpu];

    percpu_idle_count[cpu][idle ? 1 : 0]++;

    int sum = percpu_idle_count[cpu][0] + percpu_idle_count[cpu][1];

    if( sum > 100 )
    {
        int load_percent = percpu_idle_count[cpu][0] * 100 / sum;
        percpu_idle_count[cpu][0] = percpu_idle_count[cpu][1] = 0;
        percpu_cpu_load[cpu] = load_percent;

        percpu_idle_count[cpu][0] = percpu_idle_count[cpu][1] = 0;
    }
#endif
    if(!profiler_inited)
        return;

    ip /= PROFILER_MAP_DIVIDER;

    if( ip > PROFILER_MAP_SIZE )
    {
        static int warn_over = 0;

        if(!warn_over)
        {
            warn_over = 1;
            SHOW_ERROR( 0, "profiler ip overflow %p", (void*)(ip*PROFILER_MAP_DIVIDER) );
        }

        return;
    }

    map[ip]++;
    total_count++;

#if PROFILER_REGULAR_DUMP_TO_LOG
    if( (total_count % 10000) == 0 )
        profiler_dump_map();
#endif
}

// TODO must sum counts for map entries which correspond to one func

void profiler_dump_map( void )
{
    printf("Profile (%d events total):\n", total_count );

    long map_corrupt = 0;
    int total_percent = 0;

    if( total_count == 0 ) return;

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
        int percentage = (int) ( ((100*(long long)count)/total_count) );

#if PROFILER_SKIP_0_PERCENT
        if(percentage == 0) continue;
#endif
        total_percent += percentage;
        if( (percentage < 0) || (percentage > 100) )
        {
            map_corrupt++;
            continue;
        }

        if(total_percent > 200)
        {
            printf("% > 200, drop\n");
            break;
        }

        printf(
               "%6p: %6d (%2d%%) - %s\n",
               (void *)ip, count, percentage,
               name
              );

    }

    if( map_corrupt )
        SHOW_ERROR( 0, "map corrupt %ld times", map_corrupt );
}



void phantom_dump_profiler_buf(char *buf, int len)
{
    int rc;
    rc = snprintf(buf, len, "Profile (%d events total):\n\x1b[35m IP      Count    %%  Name\x1b[37m\n", total_count);
    buf += rc;
    len -= rc;

    if( total_count == 0 ) return;

    long map_corrupt = 0;
    int total_percent = 0;

    int i = 0;
    for( i = 0; i < PROFILER_MAP_SIZE; i++ )
    {
        if( map[i] == 0 )
            continue;

        if( len < 40 )
        {
            rc = snprintf(buf, len, "Out of buffer\n" );
            buf += rc;
            len -= rc;
            return;
        }

        addr_t ip = i*PROFILER_MAP_DIVIDER;

        char *name = "?";
        if(phantom_symtab_getname)
            name = phantom_symtab_getname( (void *)ip );

        int count = map[i];
        int percentage = (int) ( ((100*(long long)count)/total_count) );

//#if PROFILER_SKIP_0_PERCENT
        if(percentage == 0) continue;
//#endif
        total_percent += percentage;
        if( (percentage < 0) || (percentage > 100) )
        {
            map_corrupt++;
            continue;
        }

        if(total_percent > 200)
        {
            //printf("% > 200, drop\n");
            break;
        }


        rc = snprintf(buf, len,
                      "%6p: %6d (%2d%%) %s\n",
                      (void *)ip, count, percentage,
                      name
                     );
        buf += rc;
        len -= rc;

    }

    //if( map_corrupt )        SHOW_ERROR( 0, "map corrupt %ld times", map_corrupt );

}

















