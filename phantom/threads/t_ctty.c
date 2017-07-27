/**
 *
 * Phantom OS multithreading library.
 *
 * Copyright (C) 2009-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Thread controlling terminal support, pool of terminal structures.
 *
 * Licensed under CPL 1.0, see LICENSE file.
 *
**/


#define DEBUG_MSG_PREFIX "t_ctty"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10



#include <thread_private.h>
#include <kernel/pool.h>
#include <malloc.h>


#define DEFAULT_CTTY_QUEUE_SIZE 128


static pool_t   *ctty_pool;


static void * 	do_ctty_create(void *arg);
static void  	do_ctty_destroy(void *arg);


void t_init_ctty_pool(void)
{
    ctty_pool = create_pool();

    ctty_pool->init    = do_ctty_create;
    ctty_pool->destroy = do_ctty_destroy;

    ctty_pool->flag_autodestroy = 1;
}


static void * 	do_ctty_create(void *arg)
{
    assert(arg == 0);

    ctty_t *ret = calloc( 1, sizeof(ctty_t) );

    ret->wtty = wtty_init( WTTY_SMALL_BUF ); // DEFAULT_CTTY_QUEUE_SIZE
    if( ret->wtty == 0 )
        return 0;

    return ret;
}

static void  	do_ctty_destroy(void *arg)
{
    ctty_t *ct = arg;
    //SHOW_FLOW( 1, "delete part %s", p->name );

    wtty_destroy( ct->wtty );
}

//! Internal for threads lib, inherit or make a new ctty for thread we create
errno_t t_inherit_ctty( phantom_thread_t *t )
{
    pool_handle_t ih = GET_CURRENT_THREAD()->ctty_h;

    if( ih == 0 )
        return t_make_ctty( t );

    t->ctty_h = ih;
    //t->ctty = pool_get_el( ctty_pool, t->ctty_h );
    ctty_t *c = pool_get_el( ctty_pool, t->ctty_h );
    t->ctty_w = c->wtty;

    return 0;
}



//! Internal for threads lib, just make a new ctty for thread we create, if no one exist
errno_t t_make_ctty( phantom_thread_t *t )
{
    assert( t != 0 );
    
    // Already have one?
    if( t->ctty_h )
        return 0;

    pool_handle_t h = pool_create_el( ctty_pool, 0 );
    if( h == INVALID_POOL_HANDLE )
        return ENOMEM;

    t->ctty_h = h;

    ctty_t *c = pool_get_el( ctty_pool, t->ctty_h );
    t->ctty_w = c->wtty;

    // create and get both increase refcount
    if( pool_release_el( ctty_pool, t->ctty_h ) )
            panic( "can't release ctty el" );

    return 0;
}


//! Internal for threads lib, dec refcount or destroy ctty for thread we kill
errno_t t_kill_ctty( phantom_thread_t *t )
{
    // No ctty?
    if( !t->ctty_h )
        return 0;

    if( pool_release_el( ctty_pool, t->ctty_h ) )
            panic( "can't release ctty el" );

    t->ctty_h = 0;
    t->ctty_w = 0;

    return 0;
}









