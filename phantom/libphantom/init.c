/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Static constructors runner. General init/stop functions runner.
 *
 *
**/

#define DEBUG_MSG_PREFIX "init"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10


#include <sys/types.h>
#include <phantom_libc.h>
#include <kernel/init.h>

// See kernel/boot.h
unsigned int	arch_flags = 0; 


/* These magic symbols are provided by the linker.  */
extern void (*_preinit_array_start []) (void) __attribute__((weak));
extern void (*_preinit_array_end []) (void) __attribute__((weak));
extern void (*_init_array_start []) (void) __attribute__((weak));
extern void (*_init_array_end []) (void) __attribute__((weak));
extern void (*_fini_array_start []) (void) __attribute__((weak));
extern void (*_fini_array_end []) (void) __attribute__((weak));

//extern void _init (void);
//extern void _fini (void);

/* Iterate over all the init routines.  */
void __phantom_run_constructors (void)
{
    size_t count;
    size_t i;
/*
    count = __preinit_array_end - __preinit_array_start;
    for (i = 0; i < count; i++)
        __preinit_array_start[i] ();
*/
    //    _init ();

#if 1
    count = _init_array_end - _init_array_start;
/*
    printf("%d c'tors (%p - %p) @ (%p - %p)\n", 
		count,
		_init_array_start, _init_array_end,
		&_init_array_start, &_init_array_end
		);
*/
    for (i = 0; i < count; i++)
    {
        //printf("c'tor %p\n", _init_array_start[i]);
        _init_array_start[i] ();
    }
#endif
}

/* Run all the cleanup routines.  * /
void
__libc_fini_array (void)
{
    size_t count;
    size_t i;

    count = __fini_array_end - __fini_array_start;
    for (i = count; i > 0; i--)
        __fini_array_start[i-1] ();

//    _fini ();
}
*/



// -----------------------------------------------------------------------
// General init code
// -----------------------------------------------------------------------

static struct init_record *init_list_root = 0;
void register_init_record( struct init_record *ir )
{
    ir->next = init_list_root;
    init_list_root = ir;
}

static void run_next_init( int level, struct init_record *ir )
{
    if( ir == 0 )
        return;

    SHOW_FLOW( 6, "init %d ir %p (%p,%p,%p)", level, ir, ir->init_1, ir->init_2, ir->init_3 );

    switch( level )
    {
    case INIT_LEVEL_PREPARE:
        if(ir->init_1) ir->init_1();
        break;

    case INIT_LEVEL_INIT:
        if(ir->init_2) ir->init_2();
        break;

    case INIT_LEVEL_LATE:
        if(ir->init_3) ir->init_3();
        break;

    default:
        SHOW_ERROR( 0, "wrong level %d", level );
        break;
    }

    //if( ir->next )
    run_next_init( level, ir->next );
}

void run_init_functions( int level )
{
    run_next_init( level, init_list_root );
}


static struct init_record *stop_list_root = 0;
void register_stop_record( struct init_record *ir )
{
    ir->next = stop_list_root;
    stop_list_root = ir;
}

static void run_next_stop( int level, struct init_record *ir )
{
    if( ir == 0 )
        return;

    switch( level )
    {
    case INIT_LEVEL_PREPARE:
        if(ir->init_1) ir->init_1();
        break;

    case INIT_LEVEL_INIT:
        if(ir->init_2) ir->init_2();
        break;

    case INIT_LEVEL_LATE:
        if(ir->init_3) ir->init_3();
        break;

    default:
        SHOW_ERROR( 0, "wrong level %d", level );
        break;
    }

    //if( ir->next )
    run_next_init( level, ir->next );
}

volatile int  phantom_stop_level = 0;


void run_stop_functions( int level )
{
    phantom_stop_level = level;
    run_next_stop( level, stop_list_root );
}



