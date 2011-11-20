/**
 *
 * Phantom OS - Phantom Kernel
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel statistics counters.
 *
 *
 **/


#include <kernel/init.h>
#include <kernel/stats.h>
#include <kernel/debug.h>
#include <time.h>


//! Counts value during one second
int			*stat_sec_counters;

//! Keeps last per-second counters
int			*stat_per_sec_counters;

//! Being zeroed
int			*to_zero_counters;


//! Keeps totals per this OS run
long                    stat_total_counters[MAX_STAT_COUNTERS];

int			stat_total_seconds = 0;

// TODO persistent counters per all OS runs?

#define C_SIZE (sizeof(int) * MAX_STAT_COUNTERS)


void stat_dump_all( int av, char **ac );

static void phantom_init_stat_counters(void)
{
    stat_sec_counters		= calloc( sizeof(int), MAX_STAT_COUNTERS );
    stat_per_sec_counters	= calloc( sizeof(int), MAX_STAT_COUNTERS );
    to_zero_counters		= calloc( sizeof(int), MAX_STAT_COUNTERS );

    memset( stat_sec_counters, 0, C_SIZE );
    memset( stat_per_sec_counters, 0, C_SIZE );
    memset( to_zero_counters, 0, C_SIZE );

    memset(stat_total_counters, 0, sizeof(stat_total_counters));
}


static void phantom_init_stat_counters2(void)
{
    dbg_add_command(&stat_dump_all, "stats", "dump kernel event statistics");
}

INIT_ME( phantom_init_stat_counters, 0, phantom_init_stat_counters2 )



void stat_increment_counter( int nCounter )
{
    assert( nCounter < MAX_STAT_COUNTERS );
    stat_sec_counters[nCounter]++;
}

static void addall( long *stat_total_counters, int *last_sec );

void stat_update_second_stats(void)
{
    int *last_sec = stat_per_sec_counters;

    stat_per_sec_counters = stat_sec_counters;
    stat_sec_counters = to_zero_counters;

    stat_total_seconds++;
    addall( stat_total_counters, last_sec );

    memset( last_sec, 0, C_SIZE );

    to_zero_counters = last_sec;
}


static void addall( long *out, int *in )
{
    int i;
    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        out[i] += in[i];
    }
}


const char *stat_counter_name[MAX_STAT_COUNTERS] =
{
    "Interrupts",
    "Block IOs",
    "Pageins",
    "Pageouts",
    "Snapshots",
    "SoftIRQ",

    "TCP recv",
    "TCP send",
    "UDP recv",
    "UDP send",

    "PhysMem Alloc Pages",
    "PhysMem Free Pages",
    "VirtAddr Alloc Pages",
    "VirtAddr Free Pages",
    "LoMem Alloc Pages",
    "LoMem Free Pages",

    "DNS requests",
    "DNS answers",

    "Thread switches",
    "Thread reselects",

    "Wire page requests",
    "Wire page pageins",

    "Block IO Q size",
    "Block sync IO",

    "Disk page alloc",
    "Disk page free",
    "",
    "",
    "",
    "",
    "Thread block",
    "Switch 2 idle thread",

    "Deferred refdec runs",
    "Deferred refdec reqs",
    "Deferred refdec lost",

    "Object alloc",
    "Object free",
    "Object saturate",

};


void stat_dump_all( int av, char **ac )
{
    (void) ac;
    (void) av;

    printf("Statistics      \t\tper sec\t     total\ttotal/sec\n");
    int i;
    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        if(*stat_counter_name[i] == 0)
            continue;

        if(stat_counter_name[i] == 0)
            break;

        printf("%-20s\t%7d\t%10ld\t%10ld\n",
               stat_counter_name[i],
               stat_per_sec_counters[i],
               stat_total_counters[i],
               stat_total_counters[i]/stat_total_seconds
              );
    }
}

void phantom_dump_stats_buf(char *buf, int len)
{
    int rc;
    rc = snprintf(buf, len, "Statistics      \t\tper sec\ttotal/sec\t     total");
    buf += rc;
    len -= rc;


    int i;
    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        if(*stat_counter_name[i] == 0)
            continue;

        // Out of space - skip zero lines
        if(stat_total_counters[i] == 0)
            continue;

        if(stat_counter_name[i] == 0)
            break;

        char *scol = "\x1b[37m";

        if(stat_per_sec_counters[i])
            scol = "\x1b[33m";

        rc = snprintf(buf, len, "\n%s%-20s\t%7d\t%10ld\t%10ld",
                      scol,
                      stat_counter_name[i],
                      stat_per_sec_counters[i],
                      stat_total_counters[i]/stat_total_seconds,
                      stat_total_counters[i]
                     );
        buf += rc;
        len -= rc;
    }

    if(len--)
        *buf++ = 0;
}



errno_t get_stats_record( int id, struct kernel_stats *out )
{
    if( id >= MAX_STAT_COUNTERS )
        return EINVAL;

    if( stat_counter_name[id] == 0 )
        return EINVAL;

    strlcpy( out->name, stat_counter_name[id], KERNEL_STATS_MAX_NAME );

    out->current_per_second	= stat_per_sec_counters[id];
    out->average_per_second	= stat_total_counters[id]/stat_total_seconds;
    out->total              = stat_total_counters[id];
 
    return 0;
}





