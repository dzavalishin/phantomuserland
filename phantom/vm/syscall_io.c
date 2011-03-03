/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * IO class, generic kernel/vm communications point.
 *
**/

#include <phantom_libc.h>

#include <vm/syscall.h>
#include <vm/object.h>
#include <vm/p2c.h>
#include <vm/alloc.h>


static int debug_print = 0;




static int io_0_construct(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;

    DEBUG_INFO;
    SYSCALL_RETURN_NOTHING;
}

static int io_1_destruct(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;

    DEBUG_INFO;
    SYSCALL_RETURN_NOTHING;
}

static int io_2_class(struct pvm_object this_obj, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    //ref_inc_o( this_obj.data->_class );  //increment if class is refcounted
    SYSCALL_RETURN(this_obj.data->_class);
}

static int io_3_clone(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_THROW_STRING( "io clone called" );
}

static int io_4_equals(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    struct pvm_object him = POP_ARG;

    int ret = (me.data == him.data);

    SYS_FREE_O(him);

    SYSCALL_RETURN(pvm_create_int_object( ret ) );
}

static int io_5_tostring(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(io)" ));
}

static int io_6_toXML(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "<io/>" ));
}

static int io_7_fromXML(struct pvm_object o, struct data_area_4_thread *tc )
{
    (void)o;
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_string_object( "(io)" ));
}



static int io_15_hashcode(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    SYSCALL_RETURN(pvm_create_int_object(
       ((int)me.data)^0x8536A634^((int)&io_15_hashcode)
                                        ));
}

#if !WEAKREF_SPIN || !defined(WEAKREF_SPIN)
#  error so what?
#endif

// TODO we can replace spin with mutex if wire_page_for_addr will suport addr ranges
// TODO in fact spinlcoks need wire_page_for_addr with addr ranges too
// TODO hal_spin_wired(1), and wire automatically in hal_spin_lock

#define LOCKME(___s) \
    wire_page_for_addr( &((___s)->lock) ); \
    int ie = hal_save_cli(); \
    hal_spin_lock( &((___s)->lock) );

#define LOCKMEAGAIN(___s) \
    wire_page_for_addr( &((___s)->lock) ); \
    ie = hal_save_cli(); \
    hal_spin_lock( &((___s)->lock) );


#define UNLOCKME(___s) \
    hal_spin_unlock( &((___s)->lock) ); \
    if( ie ) hal_sti(); \
    unwire_page_for_addr( &((___s)->lock) );


#define NAIVE_SLEEP 50




// No param, no result, blocks if nothing to get
// Please read comments to io_11_pre_put()
static int io_10_pre_get(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 0);

    //LOCKME(me);

    // TODO list of waiting threads?
    // Kernel must call SYSCALL_WAKE_THREAD_UP(tc);
    if( meda->in_count <= 0 )
        SYSCALL_PUT_THIS_THREAD_ASLEEP();

    //UNLOCKME(me);

    SYSCALL_RETURN_NOTHING;
}



static int io_8_get(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 0);

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

    SYSCALL_RETURN( ret );
}



// Very naive implementation, can be fixed with cond_wait with spinlock as second parameter
errno_t io_object_put_wait(struct pvm_object ioo, struct pvm_object data )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( ioo, io );

    LOCKME(meda);

    while( meda->in_count >= IO_DA_BUFSIZE )
    {
        UNLOCKME(meda);
        hal_sleep_msec(NAIVE_SLEEP);
        LOCKMEAGAIN(meda);
    }

    meda->ibuf[meda->in_count++] = data;
    UNLOCKME(meda);

    return 0;
}

errno_t io_object_put(struct pvm_object ioo, struct pvm_object data )
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
    UNLOCKME(meda);

    return 0;
}













// No param, no result, blocks if buffer full
static int io_11_pre_put(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 0);

    //LOCKME(me);

    // We don't need mutex style locks here because SYSCALL_PUT_THIS_THREAD_ASLEEP
    // and SYSCALL_WAKE_THREAD_UP are semaphore style (keep sleep count), so calling
    // SYSCALL_WAKE_THREAD_UP before SYSCALL_PUT_THIS_THREAD_ASLEEP is perfectly ok.
    // We just have to be sure that each sleep request will have corresponding wakeup.

    // TODO list of waiting threads?
    // Kernel must call SYSCALL_WAKE_THREAD_UP();
    if( meda->out_count >= IO_DA_BUFSIZE )
        SYSCALL_PUT_THIS_THREAD_ASLEEP();

    //UNLOCKME(me);


    SYSCALL_RETURN_NOTHING;
}



static int io_9_put(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    struct pvm_object him = POP_ARG;

    //SYS_FREE_O(him);

    LOCKME(meda);

    if( meda->out_count >= IO_DA_BUFSIZE )
    {
        UNLOCKME(meda);
        SYSCALL_RETURN( pvm_create_int_object( 1 ) );
    }

    meda->obuf[meda->out_count++] = him;
    UNLOCKME(meda);


    SYSCALL_RETURN( pvm_create_int_object( 0 ) );
}



// Very naive implementation, can be fixed with cond_wait with spinlock as second parameter
errno_t io_object_get_wait(pvm_object_t ioo, pvm_object_t *data  )
{
    struct data_area_4_io *meda = pvm_object_da( ioo, io );

    LOCKME(meda);

    while( meda->out_count <= 0 )
    {
        UNLOCKME(meda);
        hal_sleep_msec(NAIVE_SLEEP);
        LOCKMEAGAIN(meda);
    }

    *data = meda->obuf[0];

    meda->obuf[0] = meda->obuf[1];
    meda->obuf[1] = meda->obuf[2];
    meda->obuf[2] = meda->obuf[3];
    meda->out_count--;

    UNLOCKME(meda);

    return 0;
}


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

















// TODO set some var to some value - parameters control, like ioctl
static int io_12_setvar(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 2);

    struct pvm_object var = POP_ARG;
    struct pvm_object val = POP_ARG;

    SYS_FREE_O(var);
    SYS_FREE_O(val);

    LOCKME(meda);
    // TODO impl
    UNLOCKME(meda);

    SYSCALL_RETURN_NOTHING;
}


// TODO get some var's value - parameters readout, like ioctl
static int io_13_getvar(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    struct pvm_object var = POP_ARG;

    SYS_FREE_O(var);

    LOCKME(meda);
    // TODO impl
    UNLOCKME(meda);

    SYSCALL_RETURN( pvm_create_int_object( 0 ) );
}



// TODO need some way to destroy communications and unblock all the waiting parties


static int io_14_reset(struct pvm_object me, struct data_area_4_thread *tc )
{
    DEBUG_INFO;
    struct data_area_4_io *meda = pvm_object_da( me, io );

    int n_param = POP_ISTACK;
    CHECK_PARAM_COUNT(n_param, 1);

    int is_reset = POP_INT();

    //LOCKME(meda);
    meda->reset = is_reset;
    //SYSCALL_WAKE_THREAD_UP();
    //UNLOCKME(meda);

    SYSCALL_RETURN_NOTHING;
}







syscall_func_t	syscall_table_4_io[16] =
{
    &io_0_construct,                &io_1_destruct,
    &io_2_class,                    &io_3_clone,
    &io_4_equals,                   &io_5_tostring,
    &io_6_toXML,                    &io_7_fromXML,
    // 8
    &io_8_get,                      &io_9_put,
    &io_10_pre_get,                 &io_11_pre_put,
    &io_12_setvar,                  &io_13_getvar,
    &io_14_reset,                   &io_15_hashcode

};
DECLARE_SIZE(io);











