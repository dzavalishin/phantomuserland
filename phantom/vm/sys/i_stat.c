/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * .internal.stat class implementation
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.udp"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 1
#define debug_level_info 0


#include <phantom_libc.h>
#include <vm/syscall_net.h>
#include <vm/alloc.h>
#include <kernel/snap_sync.h>
#include <kernel/net.h>
#include <kernel/json.h>

#include <errno.h>


static int debug_print = 0;

//! Get current stat values
#define CN_STS_OP_GET_CURR	0
//! Get average stat values
#define CN_STS_OP_GET_AVG       1
//! Get this run stat values
#define CN_STS_OP_GET_THIS      2
//! Get prev run stat values
#define CN_STS_OP_GET_PREV      3
//! Get all runs stat values
#define CN_STS_OP_GET_ALLR      4
//! Get cpu idle percentage
#define CN_STS_OP_GET_CPU_IDLE  5

#define CN_STS_OP_LAST          6






static int si_stat_tostring_5( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_RETURN( pvm_create_string_object( "stat" ));
}








static int si_stat_stat_16( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_udp      *da = pvm_data_area( me, udp );

    DEBUG_INFO;

    CHECK_PARAM_COUNT(2);
    int kind = AS_INT(args[1]);
    int n_stat_counter = AS_INT(args[0]);

    if( n_stat_counter >= MAX_STAT_COUNTERS )
    {
        SHOW_ERROR( 1, "counter num %d > max", n_stat_counter );
        SYSCALL_THROW_STRING("invalid stat counter number");
    }

    struct kernel_stats out;

    //vm_unlock_persistent_memory();

    errno_t e = get_stats_record( n_stat_counter, &out );

    //vm_lock_persistent_memory();

    if( e )
    {
        SHOW_ERROR( 1, "err %d", e );
        SYSCALL_THROW_STRING("get_stats_record failed");
    }

    int iret = 0;
    switch(kind)
    {
    case CN_STS_OP_GET_CURR:        iret = out.current_per_second;
        break;

    case CN_STS_OP_GET_AVG:        iret = out.average_per_second;
        break;

    case CN_STS_OP_GET_THIS:        iret = out.total;
        break;


    case CN_STS_OP_GET_PREV:        iret = out.total_prev_runs;
        break;

    case CN_STS_OP_GET_ALLR:        iret = out.total_prev_and_this_runs;
        break;
    }

    SYSCALL_RETURN_INT(iret);
}







static int si_stat_idle_17( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void) me;
    //struct data_area_4_udp      *da = pvm_data_area( me, udp );

    DEBUG_INFO;
/*
    CHECK_PARAM_COUNT(2);
    int n_cpu = AS_INT(args[0]);

    if( n_stat_counter >= MAX_STAT_COUNTERS )
    {
        SHOW_ERROR( 1, "counter num %d > max", n_stat_counter );
        SYSCALL_THROW_STRING("invalid stat counter number");
    }
*/
        // CPU0 - use n_stat_counter to select other
    int iret = 100-percpu_cpu_load[0];

    SYSCALL_RETURN_INT(iret);
}





syscall_func_t  syscall_table_4_stat[18] =
{
    &si_void_0_construct,               &si_void_1_destruct,
    &si_void_2_class,                   &si_void_3_clone,
    &si_void_4_equals,                  &si_stat_tostring_5,
    &si_void_6_toXML,                   &si_void_7_fromXML,
    // 8
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &invalid_syscall,
    &invalid_syscall,                   &si_void_15_hashcode,
    // 16
    &si_stat_stat_16,                   &si_stat_idle_17,



};
DECLARE_SIZE(stat);

















void pvm_internal_init_stat(pvm_object_t os)
{
    struct data_area_4_stat      *da = (struct data_area_4_stat *)os->da;

    (void) da;

    //da->connected = 0;

}

pvm_object_t     pvm_create_stat_object(void)
{
    return pvm_create_object( pvm_get_stat_class() );
}

void pvm_gc_iter_stat(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_stat      *da = (struct data_area_4_stat *)os->da;

    (void) da;

    //gc_fcall( func, arg, ot );
    //gc_fcall( func, arg, da->p_kernel_state_object );
    //gc_fcall( func, arg, da->callback );
}


void pvm_gc_finalizer_stat( pvm_object_t  os )
{
    // is it called?
    struct data_area_4_stat *da = (struct data_area_4_stat *)os->da;
    (void) da;
}


void pvm_restart_stat( pvm_object_t o )
{
    struct data_area_4_stat *da = pvm_object_da( o, stat );
    (void) da;

}

