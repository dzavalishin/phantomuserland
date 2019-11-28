/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * Test: allocator.
 *
 * Check allocation and arenas.
 *
**/


#define DEBUG_MSG_PREFIX "test.alloc"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#define YIELD() hal_sleep_msec(40)
#define MAX_ALLOC 19373

#define MIN_OBJ_SIZE 3
#define MAX_OBJ_SIZE 7357

#define N_LOOPS 1000

#define N_THREADS 30

#include <vm/object.h>
#include <vm/alloc.h>

#include <threads.h>


static void alloc_test_thread(void *a)
{
    int i, loop;
    pvm_object_t allocated[MAX_ALLOC];

    // Allocate 
    for( i = 0; i < MAX_ALLOC; i++ )
    {
        size_t sz = (MIN_OBJ_SIZE+random()) % MAX_OBJ_SIZE;
        //LOG_FLOW( 0, "Alloc %d", sz );
        printf( "Alloc %d ", sz );
        allocated[i] = pvm_create_binary_object( sz , 0 );
        if( pvm_is_null(allocated[i]) )
        {
            //SHOW_ERROR( 0, "Unable to alloc %d", sz );
            printf( "ERROR: Unable to alloc %d\n", sz );
            exit(33);
        }
    }

    // Free and allocate 
    for( loop = 0; loop < N_LOOPS; loop++ )
    {
        for( i = 0; i < MAX_ALLOC; i++ )
        {
            if( allocated[i]->_ah.refCount > 1 )
            { 
                //SHOW_ERROR( 0, "ref != 1 @ %d", i );
                printf( "ERROR: ref != 1 @ %d\n", i );
            }
            ref_dec_o( allocated[i] );

            size_t sz = (MIN_OBJ_SIZE+random()) % MAX_OBJ_SIZE;
            //LOG_FLOW( 0, "ReAlloc %d", sz );
            printf( "ReAlloc %d\n", sz );
            allocated[i] = pvm_create_binary_object( sz , 0 );

            YIELD();
        }
    }
}



void test_allocator( void )
{
    int i;

    for( i = 0; i < N_THREADS; i++ )
    {
        //LOG_FLOW( 0, "Start thread %d", i );
        printf( "Start thread %d\n", i );
        hal_start_thread( alloc_test_thread, 0, 0 );
    }

    pvm_memcheck();
}