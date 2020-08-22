/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Connection to HTTP in curl style - connect, get.
 *
**/

#define DEBUG_MSG_PREFIX "vmcn.url"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <vm/connect.h>
#include <vm/alloc.h>
#include <vm/p2c.h>
#include <vm/syscall.h>

#include <kernel/net/udp.h>



errno_t cn_url_do_operation( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *t, pvm_object_t o )
{
    (void) t;
    (void) c;
    (void) o;

    SHOW_ERROR( 1, "no op %d", op_no );
    return EINVAL;
}

static pvm_object_t cn_url_blocking_syscall_worker( pvm_object_t conn, struct data_area_4_thread *tc, int nmethod, pvm_object_t arg )
{
    (void) conn;
    (void) tc;
    (void) arg;

    errno_t e = 0;
    char buf[4096];

    vm_lock_persistent_memory();

    struct data_area_4_connection *c = pvm_object_da( conn, connection );

    //struct cn_url_volatile   *vp = c->v_kernel_state;
    struct cn_url_persistent *pp = c->p_kernel_state;

    char *url = strdup( pp->url ); // todo move to volatile state

    if( 0 == url )
    {
        //SHOW_ERROR( 1, "out of mem for url %s", pp->url );
        e = ENOMEM;
    }

    vm_unlock_persistent_memory();

    if( e == 0 )
    {
        switch( nmethod )
        {
        default:
        case CONN_OP_READ:
            e = net_curl( url, buf, sizeof(buf), 0 );
            break;

        case CONN_OP_WRITE:
        case CONN_OP_BIND:
            e = ENOSYS;
            break;
        }
    }

    free(url);

//ret:
    if( e )
    {
        SHOW_ERROR( 1, "err %d", e );

        vm_lock_persistent_memory();
        pvm_object_t ret = pvm_create_string_object("");
        vm_unlock_persistent_memory();

        return ret;
    }

    char *bp = buf;

    // Skip HTTP headers

    for( ; *bp ; bp++ )
    {
        if( (bp[0] == '\r') && (bp[1] == '\n') && (bp[2] == '\r') && (bp[3] == '\n') )
        {
            bp += 4;
            goto done;
        }

        if( (bp[0] == '\n') && (bp[1] == '\n') )
        {
            bp += 2;
            goto done;
        }
    }
    // fall through if no headers, ignore?
    SHOW_ERROR( 1, "no headers in '%s'", buf );
    // goto err_ret;

done:

    vm_lock_persistent_memory();
    pvm_object_t ret = pvm_create_string_object(bp);
    vm_unlock_persistent_memory();

    return ret;
}


// Init connection
errno_t cn_url_init( struct data_area_4_connection *c, struct data_area_4_thread *t, const char *suffix )
{
    (void) t;

    c->blocking_syscall_worker = cn_url_blocking_syscall_worker;

    SHOW_FLOW( 1, "connect url %s", suffix );

    //struct cn_url_volatile   *vp = c->v_kernel_state;
    struct cn_url_persistent *pp = c->p_kernel_state;

    strlcpy( pp->url, suffix, sizeof(pp->url) );
    /*
#if HAVE_NET
    int rc = url_open( &vp->url_endpoint );
    if( rc )
#endif // HAVE_NET
        return ENOMEM;
*/

    return 0;
}


// TODO make sure it is called on object destruction
// Finish connection
errno_t cn_url_disconnect( struct data_area_4_connection *c )
{
    (void) c;

    SHOW_FLOW0( 1, "disconnect" );
/*
    struct cn_url_volatile *vp = c->v_kernel_state;

    if( !(vp->url_endpoint) )
        return 0;

#if HAVE_NET
    int rc = url_close( vp->url_endpoint );
    if( rc )
#endif // HAVE_NET
        return EINVAL;
*/
    return 0;
}



