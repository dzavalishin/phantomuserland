#define DEBUG_MSG_PREFIX "vmcn.stats"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <vm/connect.h>
#include <vm/alloc.h>
#include <vm/p2c.h>
#include <vm/syscall.h>

#include <kernel/stats.h>

#define CN_STS_OP_GET_CURR	0
#define CN_STS_OP_GET_AVG       1
#define CN_STS_OP_GET_THIS      2
#define CN_STS_OP_GET_PREV      3
#define CN_STS_OP_GET_ALLR      4

#define CN_STS_OP_LAST          5




errno_t cn_stats_do_operation( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *t, pvm_object_t o )
{
    (void) t;
    (void) c;
    (void) o;

    SHOW_ERROR( 1, "no op %d", op_no );
    return EINVAL;
}

static pvm_object_t cn_stats_blocking_syscall_worker( pvm_object_t conn, struct data_area_4_thread *tc, int nmethod, pvm_object_t arg )
{
    (void) conn;
    (void) tc;
    (void) arg;

    if( nmethod >= CN_STS_OP_LAST )
    {
        SHOW_ERROR( 1, "wrong op %d", nmethod );
        return pvm_create_int_object(-1);
    }

    if(!IS_PHANTOM_INT(arg))
    {
        SHOW_ERROR0( 1, "arg not int" );
        return pvm_create_int_object(-1);
    }

    int n_stat_counter = pvm_get_int(arg);
    //ref_dec_o(o); wrapper does

    if( n_stat_counter >= MAX_STAT_COUNTERS )
    {
        SHOW_ERROR( 1, "counter num %d > max", n_stat_counter );
        return pvm_create_int_object(-1);
    }

    struct kernel_stats out;

    errno_t e = get_stats_record( n_stat_counter, &out );

    if( e )
    {
        SHOW_ERROR( 1, "err %d", e );
        return pvm_create_int_object(-e);
    }

    int ret = 0;
    switch(nmethod)
    {
    case CN_STS_OP_GET_CURR:
        ret = out.current_per_second;
        break;

    case CN_STS_OP_GET_AVG:
        ret = out.average_per_second;
        break;

    case CN_STS_OP_GET_THIS:
        ret = out.total;
        break;


    case CN_STS_OP_GET_PREV:
        ret = out.total_prev_runs;
        break;

    case CN_STS_OP_GET_ALLR:
        ret = out.total_prev_and_this_runs;
        break;
    }

    return pvm_create_int_object(ret);
}


// Init connection
errno_t cn_stats_init( struct data_area_4_connection *c, struct data_area_4_thread *t, const char *suffix )
{
    (void) t;
    (void) suffix;

    c->blocking_syscall_worker = cn_stats_blocking_syscall_worker;

    //printf("Init stats");
    return 0;
}


// Finish connection
errno_t cn_stats_disconnect( struct data_area_4_connection *c )
{
    (void) c;

    //c->kernel = 0;

    //SHOW_FLOW0( 1, "stats disconnected" );

    return 0;
}



