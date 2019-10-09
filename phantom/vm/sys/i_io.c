/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * IO class, generic kernel/vm communications point. DEPRECATED - redesign?
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.io"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>

#include <vm/syscall.h>
#include <vm/object.h>
#include <vm/p2c.h>
#include <vm/alloc.h>
#include <vm/internal.h>

#include <kernel/vm.h>
#include <kernel/snap_sync.h>

static int debug_print = 0;





#define LOCKME(meda)
#define UNLOCKME(meda)




static int io_0_construct( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;

    DEBUG_INFO;
    SYSCALL_RETURN_NOTHING;
}

static int io_1_destruct( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;

    DEBUG_INFO;
    SYSCALL_RETURN_NOTHING;
}

/*
static int io_2_class( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    //ref_inc_o( this_obj->_class );  //increment if class is refcounted
    SYSCALL_RETURN(this_obj->_class);
}*/


static int io_3_clone( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "io clone called" );
}

static int io_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    CHECK_PARAM_COUNT(1);

    pvm_object_t him = args[0];

    int iret = (me == him);

    SYS_FREE_O(him);

    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}

static int io_5_tostring( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(io)" ));
}

static int io_6_toXML( pvm_object_t o, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "<io/>" ));
}

static int io_7_fromXML( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    (void)me;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(io)" ));
}



static int io_15_hashcode( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object(
       ((int)me)^0x8536A634^((int)&io_15_hashcode)
                                        ));
}







static int io_8_open( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    CHECK_PARAM_COUNT(1);
    (void) meda;

    //int is_reset = AS_INT(args[0]);

    //LOCKME(meda);
    //meda->reset = is_reset;
    //SYSCALL_WAKE_THREAD_UP();
    //UNLOCKME(meda);

    SYSCALL_RETURN_NOTHING;
}





static int io_9_close( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );
    (void) meda;

    CHECK_PARAM_COUNT(1);

    //int is_reset = AS_INT(args[0]);

    //LOCKME(meda);
    //meda->reset = is_reset;
    //SYSCALL_WAKE_THREAD_UP();
    //UNLOCKME(meda);

    SYSCALL_RETURN_NOTHING;
}












static int io_10_read( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );
    (void) meda;

    CHECK_PARAM_COUNT(0);
    /*
    LOCKME(meda);

    if( meda->in_count <= 0 )
    {
        UNLOCKME(meda);
        SYSCALL_RETURN_NOTHING;
    }

    pvm_object_t ret = meda->ibuf[0];

    meda->ibuf[0] = meda->ibuf[1];
    meda->ibuf[1] = meda->ibuf[2];
    meda->ibuf[2] = meda->ibuf[3];
    meda->in_count--;

    UNLOCKME(meda);
    */
    //SYSCALL_RETURN( ret );
    SYSCALL_RETURN_NOTHING;
}



/*
errno_t io_object_put(pvm_object_t ioo, pvm_object_t data )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( ioo, io );

    LOCKME(meda);

    if( meda->in_count >= IO_DA_BUFSIZE )
    {
        UNLOCKME(meda);
        return ENOMEM;
    }

    meda->ibuf[meda->in_count++] = data;

    if(meda->in_sleep_count > 0)
    {
        pvm_object_t wakeup = meda->in_sleep_chain; // todo check type
        struct data_area_4_io *wda = pvm_object_da( wakeup, thread );

        meda->in_sleep_chain = wda->in_sleep_chain;
        meda->in_sleep_count--;

        SYSCALL_WAKE_THREAD_UP(wda);
    }

    UNLOCKME(meda);

    return 0;
}
*/









static int io_11_write( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );
    (void) meda;

    CHECK_PARAM_COUNT(1);
    /*
    pvm_object_t him = args[0];

    //SYS_FREE_O(him);

    LOCKME(meda);

    if( meda->out_count >= IO_DA_BUFSIZE )
    {
        UNLOCKME(meda);
        SYSCALL_RETURN( pvm_create_int_object( 1 ) );
    }

    meda->obuf[meda->out_count++] = him;
    UNLOCKME(meda);
    */

    SYSCALL_RETURN( pvm_create_int_object( 0 ) );
}



/*
errno_t io_object_get(pvm_object_t ioo, pvm_object_t *data  )
{
    struct data_area_4_io *meda = pvm_object_da( ioo, io );

    LOCKME(meda);

    if( meda->out_count <= 0 )
    {
        UNLOCKME(meda);
        return ENOMEM;
    }

    *data = meda->obuf[0];

    meda->obuf[0] = meda->obuf[1];
    meda->obuf[1] = meda->obuf[2];
    meda->obuf[2] = meda->obuf[3];
    meda->out_count--;

    UNLOCKME(meda);

    return 0;
}
*/
















// set some var to some value - parameters control, like ioctl
static int io_12_setvar( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );
    (void) meda;

    CHECK_PARAM_COUNT(2);

    pvm_object_t var = args[1];
    pvm_object_t val = args[0];

    SYS_FREE_O(var);
    SYS_FREE_O(val);

    LOCKME(meda);
    // TODO impl
    UNLOCKME(meda);

    SYSCALL_RETURN_NOTHING;
}


// get some var's value - parameters readout, like ioctl
static int io_13_getvar( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );
    (void) meda;

    CHECK_PARAM_COUNT(1);

    pvm_object_t var = args[0];

    SYS_FREE_O(var);

    LOCKME(meda);
    // TODO impl
    UNLOCKME(meda);

    SYSCALL_RETURN( pvm_create_int_object( 0 ) );
}



// TODO need some way to destroy communications and unblock all the waiting parties


static int io_14_reset( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );
    (void) meda;

    CHECK_PARAM_COUNT(1);

    //int is_reset = AS_INT(args[0]);

    //LOCKME(meda);
    //meda->reset = is_reset;
    //SYSCALL_WAKE_THREAD_UP();
    //UNLOCKME(meda);

    SYSCALL_RETURN_NOTHING;
}







syscall_func_t  syscall_table_4_io[16] =
{
    &io_0_construct,                &io_1_destruct,
    &si_void_2_class,                    &io_3_clone,
    &io_4_equals,                   &io_5_tostring,
    &io_6_toXML,                    &io_7_fromXML,
    // 8
    &io_8_open,                     &io_9_close,
    &io_10_read,                    &io_11_write,
    &io_12_setvar,                  &io_13_getvar,
    &io_14_reset,                   &io_15_hashcode

};
DECLARE_SIZE(io);












void pvm_internal_init_io(pvm_object_t os)
{
    struct data_area_4_io      *da = (struct data_area_4_io *)os->da;

    (void) da;

    //da->connected = 0;

}

pvm_object_t     pvm_create_io_object(void)
{
    return pvm_create_object( pvm_get_io_class() );
}

void pvm_gc_iter_io(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_io      *da = (struct data_area_4_io *)os->da;

    (void) da;

    //gc_fcall( func, arg, ot );
    //gc_fcall( func, arg, da->p_kernel_state_object );
    //gc_fcall( func, arg, da->callback );
}


void pvm_gc_finalizer_io( pvm_object_t  os )
{
    // is it called?
    struct data_area_4_io *da = (struct data_area_4_io *)os->da;
    (void) da;
}


void pvm_restart_io( pvm_object_t o )
{
    struct data_area_4_io *da = pvm_object_da( o, io );
    (void) da;

    //da->connected = 0;
    //if( da->connected )
    {
        printf("restarting IO - unimpl!");
    }

}




