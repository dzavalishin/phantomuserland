/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * This source file implements Windows based wrapper for VM to
 * run in Windows-hosted environment.
 *
 * NB! Used in Windows and Linux builds, TODO rename to non_kernel_hal.c
 *
**/

#define DEBUG_MSG_PREFIX "vm.winhal"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <stdarg.h>
#include <threads.h>

#include "event.h"
#include "gcc_replacements.h"
#include "vm/internal_da.h"
#include "vm/alloc.h"
#include "hal.h"
#include "main.h"
#include "vm/alloc.h"
       
#include "winhal.h"



struct hardware_abstraction_level    	hal;

int phantom_is_a_real_kernel() { return 0; }



void hal_init( vmem_ptr_t va, long vs )
{
    printf("Win32 HAL init @%p\n", va);

    //InitializeCriticalSection(&default_critical);

    win_hal_init();

    hal.object_vspace = va;
    hal.object_vsize = vs;

    pvm_alloc_init( va, vs );


    //int rc =
    //CreateThread( 0, 0, (void *) &winhal_debug_srv_thread, 0, 0, 0);
    //if( rc) printf("Win32 can't run debugger thread\n");

    hal_start_kernel_thread( (void*)&winhal_debug_srv_thread );
}

void hal_disable_preemption()
{
    win_hal_disable_preemption();
}

void hal_enable_preemption()
{
    win_hal_enable_preemption();
}




vmem_ptr_t hal_object_space_address() { return hal.object_vspace; }


tid_t hal_start_thread( void (*thread)(void *arg), void *arg, int flags )
{
    flags &= ~THREAD_FLAG_VM; // ok with it
    assert(!flags);

    unsigned long tid = win_hal_start_thread( thread, arg );

    if( 0 == tid )
        panic("can't start thread");

    return tid;
}



void   hal_start_kernel_thread(void (*thread)(void))
{
    unsigned long tid = win_hal_start_thread( (void *)thread, 0 );

    if( 0 == tid )
        panic("can't start thread");
}

void hal_set_current_thread_name( const char *name )
{
    (void) name;
}

errno_t t_current_set_name( const char *name )
{
    (void) name;
    return EINVAL;
}

errno_t         t_set_owner( tid_t tid, void *owner )
{
    return EINVAL;
}


errno_t hal_set_current_thread_priority(int p)
{
    (void) p;
    return EINVAL;
}

errno_t t_current_set_priority(int p)
{
    (void) p;
    return EINVAL;
}

void    hal_halt()
{
    //fflush(stderr);
    printf("\n\nhal halt called, exiting.\n");
    getchar();
    exit(1);
}

extern int sleep(int);
void hal_sleep_msec( int miliseconds )
{
    //usleep(1000*miliseconds);
    //sleep( ((miliseconds-1)/1000)+1 );
    //Sleep(miliseconds);
    win_hal_sleep_msec( miliseconds );
}





// -----------------------------------------------------------------------
// Spinlocks


#define	_spin_unlock(p) \
	({  register int _u__ ; \
	    __asm__ volatile("xorl %0, %0; \n\
			  xchgl %0, %1" \
			: "=&r" (_u__), "=m" (*(p)) ); \
	    0; })

// ret 1 on success
#define	_spin_try_lock(p)\
	(!({  register int _r__; \
	    __asm__ volatile("movl $1, %0; \n\
			  xchgl %0, %1" \
			: "=&r" (_r__), "=m" (*(p)) ); \
	    _r__; }))


void hal_spin_init(hal_spinlock_t *sl) { sl->lock = 0; }

void hal_spin_lock(hal_spinlock_t *sl)
{
    while( !  _spin_try_lock( &(sl->lock)  ) )
        while( sl->lock )
            ;
}

void hal_spin_unlock(hal_spinlock_t *sl)
{
    _spin_unlock(&(sl->lock));
}

// -----------------------------------------------------------------------

// alloc wants it
int phantom_virtual_machine_threads_stopped = 0;







// no snaps here

volatile int phantom_virtual_machine_snap_request = 0;

void phantom_thread_wait_4_snap()
{
    // Just return
}


//void phantom_activate_thread()
//{
//    // Threads do not work in this mode
//}


void hal_exit_kernel_thread()
{
	panic("hal_exit_kernel_thread");
}

void phantom_snapper_wait_4_threads()
{
    // Do nothing in non-kernel version
    // Must be implemented if no-kernel multithread will be done
}


void phantom_snapper_reenable_threads()
{
    //
}

#include <vm/stacks.h>

int vm_syscall_block( pvm_object_t this, struct data_area_4_thread *tc, pvm_object_t (*syscall_worker)( pvm_object_t this, struct data_area_4_thread *tc ) )
{
    // push zero to obj stack

    pvm_ostack_push( tc->_ostack, pvm_create_string_object("no sync in hosted env") );
    return 0; // throw!
}

/*
int phantom_dev_keyboard_getc(void)
{
    return getchar();
}
*/




/*
//#if OLD_VM_SLEEP
void phantom_thread_sleep_worker( struct data_area_4_thread *thda )
{
    / *if(phantom_virtual_machine_stop_request)
    {
        if(DEBUG) printf("Thread will die now\n");
        pthread_exit(0);
    }* /


    //phantom_virtual_machine_threads_stopped++;

#if OLD_VM_SLEEP
    while(thda->sleep_flag)
        sleep(1);
#else
#warning sleep?
    sleep(1);
#endif
    //phantom_virtual_machine_threads_stopped--;

}
*/
/* dz off

void phantom_thread_put_asleep( struct data_area_4_thread *thda )
{
    thda->sleep_flag++;
    // NB! This will work if called from SYS only! That's
    // ok since no other bytecode instr can call this.
    // Real sleep happens in phantom_thread_sleep_worker
}


void phantom_thread_wake_up( struct data_area_4_thread *thda )
{
    thda->sleep_flag--;
}
#endif
*/

void phantom_wakeup_after_msec(long msec)
{
    hal_sleep_msec(msec);
}


phantom_thread_t * get_current_thread() { return 0; }


void *get_thread_owner( phantom_thread_t *t ) { return 0; }

//int get_current_tid() { return -1; }
errno_t t_get_owner( int tid, void ** owner ) { return ENOENT; }

errno_t t_get_ctty( tid_t tid, struct wtty **ct ) { return ENOENT; }


void panic(const char *fmt, ...)
{
    va_list vl;

    // CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
    printf("\nPanic: ");
    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);

    //save_mem(mem, size);
    getchar();
    // CI: this word is being watched by CI scripts. Do not change -- or change CI appropriately
    printf("\nPress Enter from memcheck...");
    pvm_memcheck();
    //printf("\nPress Enter...");	getchar();
    exit(1);
}



static void *dm_mem, *dm_copy;
static int dm_size = 0;
void setDiffMem( void *mem, void *copy, int size )
{
    dm_mem = mem;
    dm_copy = copy;
    dm_size = size;
}

void checkDiffMem()
{
#if 0
    char *mem = dm_mem;
    char *copy = dm_copy;
    char *start = dm_mem;
    int prevdiff = 0;

    int i = dm_size;
    while( i-- )
    {
        if( *mem != *copy )
        {
            if( !prevdiff )
            {
                printf(", d@ 0x%04x", mem - start );
            }
            prevdiff = prevdiff ? 2 : 1;
            *copy = *mem;
        }
        else
        {
            if( prevdiff == 2 )
            {
                printf( "-%04x", mem - start -1 );
                prevdiff = 0;
            }
        }
        mem++;
        copy++;
    }

    printf(" Press Enter...");
    getchar();
#endif
}


void event_q_put_global( ui_event_t *e ) {}

void event_q_put_any( ui_event_t *e ) {}


void event_q_put_win( int x, int y, int info, struct drv_video_window *   focus )
{
}

int drv_video_window_get_event( drv_video_window_t *w, struct ui_event *e, int wait )
{
    printf("\nGetEvent!?\n");
    w->events_count--;
    assert(!wait);
    return 0;
}


int hal_save_cli() { return 1; }
void hal_sti() {}
void hal_cli() {}


void wire_page_for_addr( void *addr, size_t len ) {}

void unwire_page_for_addr( void *addr, size_t len ) {}


struct wtty *get_thread_ctty( struct phantom_thread *t )
{
    return 0;
}

/*
errno_t wtty_putc_nowait( struct wtty *wt, int ch )
{
    putchar(ch);
    return 0;
}
*/

#if 0
int dbg_add_command(void (*func)(int, char **), const char *name, const char *desc)
{
    return 0;
}

#else
int GET_CPU_ID() { return 0; }

void hal_cpu_reset_real() { exit(33); }

void run_test( void )
{
	printf("sorry, not in hosted env\n");
}

#endif





// -----------------------------------------------------------------------
// debug_ext.h support
// -----------------------------------------------------------------------

void console_set_error_color() { printf("\x1b[31m"); }
void console_set_normal_color() { printf("\x1b[37m"); }
void console_set_message_color(void) { printf("\x1b[34m"); }
void console_set_warning_color(void) { printf("\x1b[33m"); }

int debug_max_level_error = ~0;
int debug_max_level_info = ~0;
int debug_max_level_flow = ~0;



void phantom_check_threads_pass_bytecode_instr_boundary( void )
{
    printf("!phantom_check_threads_pass_bytecode_instr_boundary unimpl!\n");
}





// -----------------------------------------------------------------------
// TODO - implement mutex/sema code for win sim environment



/*struct phantom_cond_impl
{
    //CONDITION_VARIABLE cv;
    const char *name;
};*/


int hal_mutex_init(hal_mutex_t *m, const char *name)
{
    m->impl = gen_hal_mutex_init(name);
    assert( m->impl );
    return 0;
}

int hal_mutex_lock(hal_mutex_t *m)
{
    assert(m->impl);
    return gen_hal_mutex_lock(m->impl);
}

int hal_mutex_unlock(hal_mutex_t *m)
{
    assert(m->impl);
    return gen_hal_mutex_unlock(m->impl);
}


int hal_mutex_is_locked(hal_mutex_t *m)
{
    assert(m->impl);
    return gen_hal_mutex_is_locked(m->impl);
}


errno_t hal_mutex_destroy(hal_mutex_t *m)
{
    //struct phantom_mutex_impl *mi = m->impl;

    //if(mi->owner != 0)        panic("locked mutex killed");
    //free(mi);
    // TODO
    m->impl = 0;

    return 0;
}






int hal_cond_init( hal_cond_t *c, const char *name )
{
    c->impl = gen_hal_cond_init(name);
    assert( c->impl );
    return 0;

    //c->impl = calloc(1, sizeof(struct phantom_cond_impl)+16); // to prevent corruption if kernel hal mutex func will be called
    //InitializeConditionVariable( &(c->impl.cv) );
    //c->impl->name = name;
    //return 0;
}


errno_t hal_cond_wait( hal_cond_t *c, hal_mutex_t *m )
{
    assert(c->impl);
    gen_hal_cond_wait( c->impl, m->impl );
/*    
hal_mutex_unlock(m);
    hal_sleep_msec(100);
    //SleepConditionVariableCS( &(c->impl.cv), &(m->impl->cs), 0 );
hal_mutex_lock(m);
*/
    return 0;
}

errno_t hal_cond_timedwait( hal_cond_t *c, hal_mutex_t *m, long msecTimeout )
{
    assert(c->impl);
    //hal_sleep_msec(msecTimeout);
    //SleepConditionVariableCS( &(c->impl.cv), &(m->impl->cs), msecTimeout );
    gen_hal_cond_twait( c->impl, m->impl, msecTimeout );

    return 0;
}


errno_t hal_cond_signal( hal_cond_t *c )
{
    assert(c->impl);
    //WakeConditionVariable( &(c->impl.cv) );
    gen_hal_cond_signal( c->impl );
    return 0;
}

errno_t hal_cond_broadcast( hal_cond_t *c )
{
    assert(c->impl);
    //WakeAllConditionVariable( &(c->impl->cv) );
    gen_hal_cond_broadcast( c->impl );
    return 0;
}

errno_t hal_cond_destroy(hal_cond_t *c)
{
    //if(m->impl.owner != 0)        panic("locked mutex killed");
    free(c->impl);
    gen_hal_cond_destroy( c->impl );
    c->impl=0;

    return 0;
}









/*

int hal_sem_acquire( hal_sem_t *s )
{
    (void) s;
    hal_sleep_msec(10);
    return 0;
}

void hal_sem_release( hal_sem_t *s )
{
    (void) s;
}

int hal_sem_init( hal_sem_t *s, const char *name )
{
    (void) s;
    (void) name;
    return 0;
}


*/


void console_set_fg_color( struct rgba_t c )
{
}


void vm_map_page_mark_unused( addr_t page_start)
{
    //printf("asked to mark page unused\n");
}







time_t time(time_t *);

//time_t fast_time(void)
long fast_time(void)
{
    return time(0);
}


#warning stub
struct _key_event;
void phantom_dev_keyboard_get_key( struct _key_event *out )
{
    while(1) hal_sleep_msec(10000);
}

void phantom_set_console_getchar( int (*_getchar_impl)(void) )
{
}



// -----------------------------------------------------------
// output
// -----------------------------------------------------------


#if 0

void debug_console_putc(int c)
{
    if( kout_f ) fputc( c, kout_f );
    else putchar(c);
}




#include <kernel/debug.h>


// Print to log file only
void lprintf(char const *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if( klog_f )
        vfprintf( klog_f, fmt, ap);
    else
        vprintf(fmt, ap);
    va_end(ap);
}


#endif

// -----------------------------------------------------------
// -----------------------------------------------------------




//known call: int set_net_timer( void ) //&e, 10000, stat_update_persistent_storage, 0, 0 );

int set_net_timer(net_timer_event *e, unsigned int delay_ms, net_timer_callback callback, void *args, int flags)
{
    (void) e;
    (void) delay_ms;
    (void) callback;
    (void) args;
    (void) flags;

    //panic("set_net_timer");
    lprintf("set_net_timer called, backtrace (\"gdb bt\") me\n");

    return -1; // ERR_GENERAL - todo - errno_t
}

static int dummy_snap_catch;

volatile int * snap_catch_va = &dummy_snap_catch;

#include <exceptions.h>

errno_t t_kill_thread(tid_t tid)
{
#if CONF_USE_E4C
    const e4c_exception *e = e4c_get_exception();
    e4c_print_exception(e);
#endif // CONF_USE_E4C
    panic("t_kill_thread(%d) called", tid);
}

errno_t t_current_set_death_handler( void (*handler)(phantom_thread_t *tp) )
{
#warning ignored?
    return 0;
}


void check_global_lock_entry_count(void) {}



void vm_lock_persistent_memory() {}
void vm_unlock_persistent_memory() {}

