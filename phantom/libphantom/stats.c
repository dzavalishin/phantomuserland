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
#include <kernel/snap_sync.h>
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

#if COMPILE_PERSISTENT_STATS
static struct persistent_kernel_stats *pdata = 0;

static void stat_update_persistent_storage( void *ign );
#endif



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

#if COMPILE_PERSISTENT_STATS
    stat_update_persistent_storage(0);
#endif

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
    "OS Reboots",
    "OS Snapshots",
    "? ",
    "? ",

    // 4
    "? 4",
    "? 5",

    "TCP recv",
    "TCP send",
    "UDP recv",
    "UDP send",

    // 10
    "PhysMem Allc Pgs",
    "PhysMem Free Pgs",
    "VirtAddr Allc Pgs",
    "VirtAddr Free Pgs",
    "LoMem Alloc Pages",
    "LoMem Free Pages",

    "DNS requests",
    "DNS answers",

    // 18
    "? 18",
    "? 19",

    "Wire page req",
    "Wire page pageins",

    "Block IO Q size",
    "Block sync IO",

    "Disk page alloc",
    "Disk page free",

    // 26
    "Block IOs",
    "Pageins",
    "Pageouts",
    "? 29",

    // 30
    "? 30",
    "? 31",

    "Defrd refdec runs",
    "Defrd refdec reqs",
    "Defrd refdec lost",

    "Object alloc",
    "Object free",
    "Object saturate",

    // 38
    "? 38",
    "? 39",
    "? 40",

    // 41
    "Thread switches",
    "Thread reselects",

    "Thread block",
    "Switch 2 idle thr",

    // 45
    "? 45",
    "? 46",

    "Interrupts",
    "SoftIRQ",
};


void stat_dump_all( int av, char **ac )
{
    (void) ac;
    (void) av;

    vm_lock_persistent_memory();
#if COMPILE_PERSISTENT_STATS
    printf(" Statistics            /sec t/sec total/run total/life\n");
#else
    printf(" Statistics        \t\tper sec\t     total\ttotal/sec\n");
#endif
    int i;
    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        if(stat_counter_name[i] == 0)
            break;

        if(*stat_counter_name[i] == 0)
            continue;

#if COMPILE_PERSISTENT_STATS
        printf(" %-20s %5d %5ld %10ld %10ld\n",
               stat_counter_name[i],
               stat_per_sec_counters[i],
               stat_total_counters[i]/stat_total_seconds,
               stat_total_counters[i],
               (long)pdata[i].total_prev_and_this_runs
              );
#else
        printf(" %-20s\t%7d\t%10ld\t%10ld\n",
               stat_counter_name[i],
               stat_per_sec_counters[i],
               stat_total_counters[i]/stat_total_seconds,
               stat_total_counters[i],
              );
#endif
    }
    vm_unlock_persistent_memory();
}

static struct persistent_kernel_stats dumb_pst = { 0, 0 };


static void do_phantom_dump_stats_buf(char *buf, int len)
{
    int rc;
#if COMPILE_PERSISTENT_STATS
    rc = snprintf(buf, len, "\x1b[35m Statistics    curr/sec t/sec      this run    life\x1b[37m");
#else
    rc = snprintf(buf, len, " Statistics       \tper sec\ttotal/sec\t     total");
#endif
    buf += rc;
    len -= rc;


    int i;
    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        if(stat_counter_name[i] == 0)
            break;

        if(*stat_counter_name[i] == 0)
            continue;
#if COMPILE_PERSISTENT_STATS
        struct persistent_kernel_stats *ps = pdata ? pdata + i : &dumb_pst;

        // Out of space - skip zero lines
        if(
           (0 == stat_total_counters[i]) &&
           (0 == (long)ps->total_prev_and_this_runs)
          )
            continue;
#else
        // Out of space - skip zero lines
        if(stat_total_counters[i] == 0)
            continue;
#endif


        char *scol = "\x1b[37m";

        if(stat_per_sec_counters[i])
            scol = "\x1b[33m";

#if COMPILE_PERSISTENT_STATS
        rc = snprintf(buf, len, "\n %s%-17s%5d %5ld %10ld %10ld",
                      scol,
                      stat_counter_name[i],
                      stat_per_sec_counters[i],
                      stat_total_counters[i]/stat_total_seconds,
                      stat_total_counters[i],
                      //(long)pdata[i].total_prev_and_this_runs
                      (long)ps->total_prev_and_this_runs
                     );
#else
        rc = snprintf(buf, len, "\n %s%-17s\t%7d\t%10ld\t%10ld",
                      scol,
                      stat_counter_name[i],
                      stat_per_sec_counters[i],
                      stat_total_counters[i]/stat_total_seconds,
                      stat_total_counters[i]
                     );
#endif
        buf += rc;
        len -= rc;
    }

    if(len--)
        *buf++ = 0;
}

void phantom_dump_stats_buf(char *buf, int len)
{
    vm_lock_persistent_memory();
    do_phantom_dump_stats_buf(buf, len);
    vm_unlock_persistent_memory();
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
    out->total                  = stat_total_counters[id];

#if COMPILE_PERSISTENT_STATS
    vm_lock_persistent_memory();
    out->total_prev_and_this_runs = pdata[id].total_prev_and_this_runs;
    out->total_prev_runs          = pdata[id].total_prev_runs;
    vm_unlock_persistent_memory();
#else
    out->total_prev_and_this_runs = 0;
    out->total_prev_runs = 0;
#endif

    return 0;
}



void stat_set_persistent_storage( struct persistent_kernel_stats *_pdata )
{
#if COMPILE_PERSISTENT_STATS
    int i;

    pdata = _pdata;
    assert(pdata); // TODO check that addr is in persistent area

    vm_lock_persistent_memory();

    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        pdata[i].total_prev_runs = pdata[i].total_prev_and_this_runs;
    }

    vm_unlock_persistent_memory();
#endif
}

static void stat_update_persistent_storage( void *ign )
{
    (void) ign;
#if COMPILE_PERSISTENT_STATS
    int i;

    static net_timer_event e;

    {
        int rc = set_net_timer( &e, 10000, stat_update_persistent_storage, 0, 0 );
        if( rc )
            printf( "can't set timer, persistent stats are dead" );
            //SHOW_ERROR0( 0, "can't set timer, persistent stats are dead" );
    }

    //assert(pdata); // TODO check that addr is in persistent area
    if( !pdata )
        return;

    vm_lock_persistent_memory();
    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        pdata[i].total_prev_and_this_runs =
            pdata[i].total_prev_runs +
            stat_total_counters[i];
    }
    vm_unlock_persistent_memory();
#endif
}






// -----------------------------------------------------------------------
//
// Dump stats to JSON
//
// -----------------------------------------------------------------------

#include <kernel/json.h>




static void json_dump_stat_record( json_output *jo, int i )
{
#if COMPILE_PERSISTENT_STATS
    struct persistent_kernel_stats *ps = pdata ? pdata + i : &dumb_pst;
#endif

    json_out_string( jo, "name", stat_counter_name[i] );
    json_out_delimiter( jo );

    json_out_int( jo, "per_sec", stat_per_sec_counters[i] );
    json_out_delimiter( jo );

    json_out_int( jo, "total_per_sec", stat_total_counters[i]/stat_total_seconds );
    json_out_delimiter( jo );

    json_out_int( jo, "total", stat_total_counters[i] );

#if COMPILE_PERSISTENT_STATS
    json_out_delimiter( jo );

    json_out_int( jo, "all_runs", (long)ps->total_prev_and_this_runs );
    //json_out_delimiter( jo );
#endif

}



void json_dump_stats( json_output *jo )
{
    int i;
    int count = 0;

    json_out_open_array( jo, "stats" );

    for( i = 0; i < MAX_STAT_COUNTERS; i++ )
    {
        if(stat_counter_name[i] == 0)
            break;

        if(*stat_counter_name[i] == 0)
            continue;

        if( count++ > 0 )
            json_out_delimiter( jo );

        json_out_open_anon_struct( jo );
        json_dump_stat_record( jo, i );
        json_out_close_struct( jo );
    }

    json_out_close_array( jo );
}

