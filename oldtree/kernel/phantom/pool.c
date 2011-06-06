/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Pool of kernel structs accessed with handle.
 *
**/


#include <kernel/pool.h>
#include <kernel/libkern.h>
#include <malloc.h>
#include <string.h>




static pool_arena_t * alloc_arenas( int arena_size, int narenas, pool_t * init );
static void free_arenas( int narenas, pool_arena_t *a );

#define HANDLE_2_ARENA(h) ( (h>>16) & 0xFFFF )
#define HANDLE_2_ELEM(h) ( h & 0xFFFF )
#define MK_HANDLE(a,e) ( ((a & 0xFFFF)<<16) | (e & 0xFFFF) )


pool_t *create_pool() { return create_pool_ext( 512, 256 ); }

pool_t *create_pool_ext( int inital_elems, int arena_size )
{
    if( inital_elems < arena_size ) inital_elems = arena_size;

    assert(arena_size > 0);

    pool_t *p = calloc( sizeof(pool_t), 1 );
    assert( p );

    hal_mutex_init( &p->mutex, "arena" );

    hal_mutex_lock( &p->mutex );

    p->narenas = 1+((inital_elems-1)/arena_size);
    p->arenas = alloc_arenas( arena_size, p->narenas, 0 );

    p->flag_autodestroy = 1;

    hal_mutex_unlock( &p->mutex );
    return p;
}


// Warning - if shrink, topmost arena elements will be lost!
void resize_pool( pool_t *p, int narenas )
{
    assert(narenas > 0);
    assert(narenas > p->narenas); // Just grow
    assert( p );

    hal_mutex_lock( &p->mutex );

    pool_arena_t *na = alloc_arenas( p->arenas[0].arena_size, narenas, p );
    assert( na );
    free_arenas( p->narenas, p->arenas );
    p->arenas = na;

    hal_mutex_unlock( &p->mutex );
}




static pool_arena_t * alloc_arenas( int arena_size, int narenas, pool_t *init )
{
    pool_arena_t *ret = calloc( sizeof(pool_arena_t), narenas );

    int i;
    for( i = 0; i < narenas; i++  )
    {
        ret[i].arena_size = arena_size;
        ret[i].nused = 0;
        ret[i].ptrs = calloc( sizeof(void*), arena_size );
        ret[i].refc = calloc( sizeof(int), arena_size );
        assert(ret[i].ptrs);

        if(init && i < init->narenas)
        {
            int sz = umin(arena_size,init->arenas[i].arena_size);
            memcpy( ret[i].ptrs, init->arenas[i].ptrs, sizeof(void*) * sz );
        }
    }

    return ret;
}


static void free_arenas( int narenas, pool_arena_t *a )
{
    assert(a);

    int i;
    for( i = 0; i < narenas; i++ )
    {
        assert( a[i].ptrs );
        free( a[i].ptrs );
        assert( a[i].refc );
        free( a[i].refc );
    }

    free(a);
}






//! Called in lock!
static void do_destroy_el( pool_t *pool, pool_arena_t *a, int ne );
//! Called in lock!
static pool_handle_t find_free_el( pool_t *pool );


#define CHECK_ARENA(na) if( ((na) > pool->narenas) || ((na) < 0)) panic("arena id is wrong")

#define GET_ARENA(na) ({ CHECK_ARENA(na); pool->arenas+(na); })

#define CHECK_EL(_a,_ne) ({ int __ne = (_ne); if( ((__ne) > (_a)->arena_size) || ((__ne) < 0) || ((_a)->ptrs[(__ne)] == 0) ) panic("arena el is wrong"); })
#define CHECK_REF(_a,_ne) ({ int __ne = (_ne); if( ((_a)->refc[(__ne)] <= 0) ) panic("arena el ref <= 0"); })


void *pool_get_el( pool_t *pool, pool_handle_t handle )
{
    hal_mutex_lock( &pool->mutex );

    int na = HANDLE_2_ARENA(handle);
    int ne = HANDLE_2_ELEM(handle);
    pool_arena_t *a = GET_ARENA(na);
    CHECK_EL(a,ne);
    CHECK_REF(a,ne);

    a->refc[ne]++;
    hal_mutex_unlock( &pool->mutex );
    return a->ptrs[ne];
}


void pool_release_el( pool_t *pool, pool_handle_t handle )
{
    hal_mutex_lock( &pool->mutex );

    int na = HANDLE_2_ARENA(handle);
    int ne = HANDLE_2_ELEM(handle);
    pool_arena_t *a = GET_ARENA(na);
    CHECK_EL(a,ne);
    CHECK_REF(a,ne);

    a->refc[ne]--;
    if(pool->flag_autodestroy)
        do_destroy_el( pool, a, ne );

    hal_mutex_unlock( &pool->mutex );
}


void pool_destroy_el( pool_t *pool, pool_handle_t handle )
{
    hal_mutex_lock( &pool->mutex );

    int na = HANDLE_2_ARENA(handle);
    int ne = HANDLE_2_ELEM(handle);
    pool_arena_t *a = GET_ARENA(na);
    CHECK_EL(a,ne);
    if( (a->refc[ne] > 0) ) panic("arena el ref > 0 on destroy");

    do_destroy_el( pool, a, ne );

    hal_mutex_unlock( &pool->mutex );
}


pool_handle_t pool_create_el( pool_t *pool, void *arg )
{
    hal_mutex_lock( &pool->mutex );
    pool_handle_t ret = -1;

    // TODO grow

    ret = find_free_el( pool );

    int na = HANDLE_2_ARENA(ret);
    int ne = HANDLE_2_ELEM(ret);

    void *e;

    if(pool->init)
        e = pool->init( arg );
    else
        e = arg;

    assert(e);

    pool_arena_t *a = GET_ARENA(na);

    a->refc[ne]++;
    a->ptrs[ne] = e;

    hal_mutex_unlock( &pool->mutex );

    if( (ret < 0) || pool->flag_nofail )
        panic("out of mem in pool");

    return ret;
}





static void do_destroy_el( pool_t *pool, pool_arena_t *a, int ne )
{
    assert(a->nused > 0);
    assert(a->nused < a->arena_size );
    assert(a->refc[ne] == 0);

    if( pool->destroy )
        pool->destroy( a->ptrs[ne] );

    a->ptrs[ne] = 0;
    a->nused--;
}

static pool_handle_t find_free_el( pool_t *pool )
{
    pool_handle_t h = pool->last_handle;
    pool_arena_t * as = pool->arenas;
    int wrapped = 0;

rewrap:
    // Wrap
    if( HANDLE_2_ARENA(h) > pool->narenas )
    {
        h = 0;
        wrapped++;
        if( wrapped > 1 ) return -1;
    }

    if( HANDLE_2_ELEM(h) > as[HANDLE_2_ARENA(h)].arena_size )
    {
        int na;
    next_arena:
        na = 1 + HANDLE_2_ARENA(h);
        h = MK_HANDLE(na,0);
        goto rewrap;
    }

    if( as[HANDLE_2_ARENA(h)].nused > as[HANDLE_2_ARENA(h)].arena_size )
        goto next_arena;

    // Now we have arena with some free place

    int na = HANDLE_2_ARENA(h);
    int i;
    for( i = 0; i < as[na].arena_size; i++ )
    {
        if( as[na].ptrs[i] == 0 )
        {
            pool->last_handle = MK_HANDLE(na,i);
            return pool->last_handle;
        }

    }
    goto rewrap;

    return -1;
}









