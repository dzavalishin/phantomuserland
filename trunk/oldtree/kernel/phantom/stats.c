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
long		stat_total_counters[MAX_STAT_COUNTERS];

int			stat_total_seconds = 0;

// TODO persistent counters per all OS runs?

#define C_SIZE (sizeof(int) * MAX_STAT_COUNTERS)


void stat_dump_all( int av, char **ac );

void phantom_init_stat_counters(void)
{
    stat_sec_counters		= calloc( sizeof(int), MAX_STAT_COUNTERS );
    stat_per_sec_counters	= calloc( sizeof(int), MAX_STAT_COUNTERS );
    to_zero_counters		= calloc( sizeof(int), MAX_STAT_COUNTERS );

    memset( stat_sec_counters, 0, C_SIZE );
    memset( stat_per_sec_counters, 0, C_SIZE );
    memset( to_zero_counters, 0, C_SIZE );

    memset(stat_total_counters, 0, sizeof(stat_total_counters));
}

void phantom_init_stat_counters2(void)
{
    dbg_add_command(&stat_dump_all, "stats", "dump kernel event statistics");
}


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
    "Interrupts          ",
    "Block IOs           ",
    "Pageins             ",
    "Pageouts            ",
    "Snapshots           ",
    "SoftIRQ             ",

    "TCP recv            ",
    "TCP send            ",
    "UDP recv            ",
    "UDP send            ",

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

};


void stat_dump_all( int av, char **ac )
{
    (void) ac;
    (void) av;

    printf("Statistics      \t\tper sec\t     total\ttotal/sec\n");
    int i;
    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
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

