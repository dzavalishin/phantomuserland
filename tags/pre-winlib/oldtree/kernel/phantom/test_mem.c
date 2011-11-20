/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - memory
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 8
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <phantom_libc.h>
#include <errno.h>
#include "test.h"

#include <hal.h>

// -----------------------------------------------------------------------
// Common tools
// -----------------------------------------------------------------------

static void *memnotchar(void *addr, int c, size_t size)
{
    unsigned char *p = (unsigned char *)addr;

    while(size) {
        if(*p != c)
            return (void *)p;
        p++;
        size--;
    }
    return 0;
}


struct alloced
{
    long        pos;
    int         size;
};

typedef struct alloced al_t;

// -----------------------------------------------------------------------
// Malloc
// -----------------------------------------------------------------------


int do_test_malloc(const char *test_parm)
{
    (void) test_parm;

    const int max = 200;
    char *p[max];
    int sz[max];

    int i;

    for( i = 0; i < max; i++ )
    {
        sz[i] = 13*i;
        p[i] = malloc( sz[i] );
        if( p[i] == 0 )
            return ENOMEM;

        memset( p[i], i, sz[i] );
    }

    for( i = 0; i < max; i += 2 )
    {
        free( p[i] );
        if( memnotchar( p[i+1], i+1, sz[i+1]) )
            return EINVAL;
    }

    for( i = 1; i < max; i += 2 )
    {
        free( p[i] );
    }

    return 0;
}

// -----------------------------------------------------------------------
// memcpy p2v & v2p
// -----------------------------------------------------------------------


#define MSIZE (4096*2)

int do_test_physmem(const char *test_parm)
{
    (void) test_parm;
#if !defined(ARCH_arm)
    void *va;
    physaddr_t pa;

    char buf[MSIZE];

    hal_pv_alloc( &pa, &va, MSIZE );

    test_check_true( va != 0 );
    test_check_true( pa != 0 );

    memset( va, 0, MSIZE );
    memcpy_p2v( buf, pa, MSIZE );
    if( memnotchar( buf, 0, MSIZE ) )
        test_fail_msg( EINVAL, "not 0");


    memset( buf, 0xFF, MSIZE );
    memcpy_v2p( pa, buf, MSIZE );
    if( memnotchar( va, 0xFF, MSIZE ) )
        test_fail_msg( EINVAL, "not 1");

    memset( va, 0, MSIZE );

    memcpy_v2p( pa, "AAA", 3 );
    if( memnotchar( va, 'A', 3 ) )
        test_fail_msg( EINVAL, "not A");

    if( memnotchar( va+3, 0, MSIZE-3 ) )
        test_fail_msg( EINVAL, "not A0");


    memset( va, 0, MSIZE );

    memcpy_v2p( pa+10, "BBB", 3 );
    if( memnotchar( va+10, 'B', 3 ) )
        test_fail_msg( EINVAL, "not B");

    if( memnotchar( va, 0, 10 ) )
        test_fail_msg( EINVAL, "not B0-");

    if( memnotchar( va+13, 0, MSIZE-13 ) )
        test_fail_msg( EINVAL, "not B0+");


    // Cross page
    memset( va, 0, MSIZE );
#define SH (4096-4)

    memcpy_v2p( pa+SH, "EEEEEEEE", 8 );
    if( memnotchar( va+SH, 'E', 8 ) )
        test_fail_msg( EINVAL, "not E");

    if( memnotchar( va, 0, SH ) )
        test_fail_msg( EINVAL, "not E0-");

    if( memnotchar( va+SH+8, 0, MSIZE-SH-8 ) )
        test_fail_msg( EINVAL, "not E0+");



#if 0 // not impl
    memset( va, 0, MSIZE );

    memset( va+20, 'C', 3 );
    memcpy_p2v( buf, pa+20, 3 );
    if( memnotchar( buf, 'C', 3 ) )
        test_fail_msg( EINVAL, "not C");
#endif

    hal_pv_free( pa, va, MSIZE );
#endif //!defined(ARCH_arm)

    return 0;
}



// -----------------------------------------------------------------------
// Physalloc (generic test)
// -----------------------------------------------------------------------

#include <kernel/physalloc.h>

#define PA_PAGES (1024*1024)

static physalloc_t   	pm_map;


#define IGS 200

static al_t     	igot[IGS];
static int      	igp = 0;

#if 0
static void clr_alloc_list()
{
    int cnt = IGS;
    igp = 0;

    while(cnt--)
    {
        igot[++igp].size = 0;
    }
    igp = 0;
}
#endif

static void get_some(int times)
{
    if( times == 0 ) return;
    get_some(times-1);

    int sz = random() % 20;
    int cnt = IGS;

	if( sz <= 0 ) sz = 1;

    while(cnt--)
    {
        if( igp >= IGS-1 ) igp = -1;

        if( igot[++igp].size )
            continue;

        physalloc_item_t 	ret;
        if( phantom_phys_alloc_region( &pm_map, &ret, sz ) )
            return;

        igot[igp].size = sz;
        igot[igp].pos = ret;

        SHOW_FLOW( 10, "get %7d @ %7d", igot[igp].size, igot[igp].pos);

        return;
    }
}

static void put_some(int times)
{
    if( times == 0 ) return;
    put_some(times-1);

    int cnt = IGS;

    while(cnt--)
    {
        if( igp >= IGS-1 ) igp = -1;

        if( igot[++igp].size == 0 )
            continue;

        SHOW_FLOW( 10, "put %7d @ %7d", igot[igp].size, igot[igp].pos);

        phantom_phys_free_region( &pm_map, igot[igp].pos, igot[igp].size );
        igot[igp].size = 0;

        return;
    }
}


static int nfree()
{
    return pm_map.allocable_size - pm_map.n_used_pages;
}

static void __cfree(int tobe, int ln)
{
    int real = nfree();

    if( tobe == real )
        return;

    SHOW_ERROR( 0, "free is %d, must be %d, diff = %d  @line %d", real, tobe, real-tobe, ln );

    test_fail_msg( EINVAL, "free count is wrong" );
}

#define cfree(___s) __cfree(___s,__LINE__)

int do_test_physalloc_gen(const char *test_parm)
{
    (void) test_parm;

    physalloc_item_t 	ret;

    memset( &igot, 0, sizeof(igot) );

    SHOW_FLOW( 0, "physalloc test arena %d, initial free %d", PA_PAGES, PA_PAGES/2 );

    phantom_phys_alloc_init( &pm_map, PA_PAGES ); 

    SHOW_FLOW( 0, "physalloc test allocable_size %d, n_used_pages %d", pm_map.allocable_size, pm_map.n_used_pages );
    // Free half at strange position

    pm_map.allocable_size = PA_PAGES/2;
    pm_map.n_used_pages = PA_PAGES/2; // init sets it to PA_PAGES
    phantom_phys_free_region(&pm_map, 100, PA_PAGES/2);

    SHOW_FLOW( 0, "physalloc test allocable_size %d, n_used_pages %d", pm_map.allocable_size, pm_map.n_used_pages );

    cfree(PA_PAGES/2);

    test_check_false( phantom_phys_alloc_region( &pm_map, &ret, 20 ) );

    cfree(PA_PAGES/2-20);

    phantom_phys_free_region( &pm_map, ret, 20 );

    cfree(PA_PAGES/2);

    SHOW_FLOW0( 0, "loop mem a/f" );

    get_some(10);
    put_some(5);
    get_some(10);
    put_some(5);
    get_some(10);
    put_some(10);

    put_some(25); // cleanup

    cfree(PA_PAGES/2);

    return 0;
}

































