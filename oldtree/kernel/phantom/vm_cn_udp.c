/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Connection to UDP.
 *
**/

#define DEBUG_MSG_PREFIX "vmcn.udp"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <vm/connect.h>
#include <vm/alloc.h>
#include <vm/p2c.h>
#include <vm/syscall.h>


#include <kernel/net/udp.h>



errno_t cn_udp_do_operation( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *t, pvm_object_t o )
{
    (void) t;
    (void) c;
    (void) o;

    SHOW_ERROR( 1, "no op %d", op_no );
    return EINVAL;
}

static pvm_object_t cn_udp_blocking_syscall_worker( pvm_object_t conn, struct data_area_4_thread *tc, int nmethod, pvm_object_t arg )
{
    (void) conn;
    (void) tc;
    (void) arg;

    struct data_area_4_connection *c = pvm_object_da( conn, connection );
    struct cn_udp_volatile *vp = c->v_kernel_state;



    errno_t e = ENOSYS;
    int ret = 0;


    if( !vp->udp_endpoint )
    {
        e = ENOTCONN;
        goto ret;
    }


    switch( nmethod )
    {
    default:
    case CONN_OP_READ:
    case CONN_OP_WRITE:
        e = ENOSYS;
        break;

#if HAVE_NET

    case CONN_OP_BIND:
        {
            if(!IS_PHANTOM_INT(arg))
            {
                SHOW_ERROR0( 1, "bind arg not int" );
                e = EINVAL;
                break;
            }

            i4sockaddr a;

            a.port = pvm_get_int(arg);

            if( udp_bind(vp->udp_endpoint, &a) )
                e = EISCONN;
            else
                e = 0;
            break;
        }
#endif // HAVE_NET

    }


ret:
    if( e )
    {
        SHOW_ERROR( 1, "err %d", e );
        return pvm_create_int_object(-e);
    }


    return pvm_create_int_object(ret);
}


// Init connection
errno_t cn_udp_init( struct data_area_4_connection *c, struct data_area_4_thread *t, const char *suffix )
{
    (void) t;

    c->blocking_syscall_worker = cn_udp_blocking_syscall_worker;

    SHOW_FLOW( 1, "connect udp %s", suffix );

    struct cn_udp_volatile *vp = c->v_kernel_state;

#if HAVE_NET
    int rc = udp_open( &vp->udp_endpoint );
    if( rc )
#endif // HAVE_NET
        return ENOMEM;


    //printf("Init stats");
    return 0;
}


// TODO make sure it is called on object destruction
// Finish connection
errno_t cn_udp_disconnect( struct data_area_4_connection *c )
{
    (void) c;

    SHOW_FLOW0( 1, "disconnect" );

    struct cn_udp_volatile *vp = c->v_kernel_state;

    if( !(vp->udp_endpoint) )
        return 0;

#if HAVE_NET
    int rc = udp_close( vp->udp_endpoint );
    if( rc )
#endif // HAVE_NET
        return EINVAL;

    return 0;
}



