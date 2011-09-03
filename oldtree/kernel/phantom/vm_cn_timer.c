#define DEBUG_MSG_PREFIX "vmcn.timer"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <vm/connect.h>
#include <vm/alloc.h>
#include <vm/p2c.h>
#include <kernel/timedcall.h>
#include <net_timer.h>
#include <time.h>

#define CN_TM_NET 1


static void cn_timer_timedcall_func( void *a )
{
    struct data_area_4_connection *c = a;
    time_t t = fast_time();

    SHOW_FLOW( 1, "timer callback", t );
    phantom_connection_callback_int( c, t );
}


errno_t cn_timer_do_operation( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *t, pvm_object_t o )
{
    (void) t;

    if( op_no > 0 )
    {
        SHOW_ERROR( 1, "wrong timer op %d", op_no );
        return EINVAL;
    }

    if(!IS_PHANTOM_INT(o))
    {
        SHOW_ERROR0( 1, "timer arg not int" );
        return EINVAL;
    }

    int iarg = pvm_get_int(o);
    //ref_dec_o(o); wrapper does

    SHOW_FLOW( 1, "timer requested in %d msec", iarg );

#if CN_TM_NET
    net_timer_event *e = c->v_kernel_state;

    int rc = set_net_timer(e, iarg, cn_timer_timedcall_func, c, 0);
    return rc ? EINVAL : 0;
#else
    timedcall_t *tc = c->v_kernel_state;
    phantom_undo_timed_call(tc);

    tc->f = cn_timer_timedcall_func;
    tc->arg = c;
    tc->msecLater = iarg;

    phantom_request_timed_call( tc, 0 );
    return 0;
#endif
}


// Init connection
errno_t cn_timer_init( struct data_area_4_connection *c, struct data_area_4_thread *t )
{
    (void) t;
    (void) c;

    // nope
    printf("Init timer");
    return 0;
}


// Finish connection
errno_t cn_timer_disconnect( struct data_area_4_connection *c, struct data_area_4_thread *t )
{
    (void) t;

    c->kernel = 0;

    SHOW_FLOW0( 1, "timer disconnected" );

#if CN_TM_NET
    net_timer_event *e = c->v_kernel_state;

    //int rc =
    cancel_net_timer(e);
    //return rc ? EINVAL : 0;
#else

    timedcall_t *tc = c->v_kernel_state;
    phantom_undo_timed_call(tc);
#endif
    return 0;
}



