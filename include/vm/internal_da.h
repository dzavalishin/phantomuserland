/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Data area structures of internal classes.
 *
 *
**/

#ifndef PVM_INTERNAL_DA_H
#define PVM_INTERNAL_DA_H


#include <vm/object.h>
#include <vm/exception.h>

#include <video/window.h>

#include <hal.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <kernel/atomic.h>

#include <kernel/net_timer.h>
#include <stdint.h>




#if 0
#define pvm_object_da_tostring( type ) #type
/** Extract (typed) object data area pointer from object pointer. */
#define pvm_object_da( o, type ) ({ if(!pvm_object_class_exactly_is( o, pvm_get_##type##_class() )) pvm_exec_panic0("type err for " pvm_object_da_tostring( type ) ); ((struct data_area_4_##type *)&(o->da)); })
/** Extract (typed) object data area pointer from object pointer. */
#define pvm_data_area( o, type ) ((struct data_area_4_##type *)&(o->da))
#else
/** Extract (typed) object data area pointer from object pointer. */
#define pvm_object_da( o, type ) ((struct data_area_4_##type *)&(o->da))
/** Extract (typed) object data area pointer from object pointer. */
#define pvm_data_area( o, type ) ((struct data_area_4_##type *)&(o->da))
#endif


/** Num of slots in normal (noninternal) object. */
#define da_po_limit(o)   (((o)->_da_size)/sizeof(pvm_object_t ))
/** Slots access for noninternal object. */
#define da_po_ptr(da)  ((pvm_object_t *)&(da))

//pvm_object_t pvm_storage_to_object(pvm_object_storage_t *st);
#define pvm_storage_to_object( o ) (o)

static inline pvm_object_t pvm_da_to_object(void *da)
{
    const int off = __offsetof(pvm_object_storage_t,da);
    pvm_object_storage_t *st = da-off;

    return pvm_storage_to_object(st);
}

void pvm_fill_syscall_interface( pvm_object_t iface, int syscall_count );




#if SMP
#warning VM spinlocks
#else

//#  define VM_SPIN_LOCK(___var) ({ hal_disable_preemption(); atomic_add(&(___var),1); assert( (___var) == 1 ); })
//#  define VM_SPIN_UNLOCK(___var) ({ atomic_add(&(___var),-1); hal_enable_preemption(); assert( (___var) == 0 ); })

//#  define VM_SPIN_LOCK(___var) ({ hal_wired_spin_lock(&___var); })
//#  define VM_SPIN_UNLOCK(___var) ({ hal_wired_spin_unlock(&___var); })
//#  define VM_SPIN_TYPE hal_spinlock_t

#  define VM_SPIN_LOCK(___var) ({ pvm_spin_lock(&___var); })
#  define VM_SPIN_UNLOCK(___var) ({ pvm_spin_unlock(&___var); })
#  define VM_SPIN_TYPE pvm_spinlock_t
#endif




/**
 *
 * Internal objects data areas.
 *
**/

struct data_area_4_array
{
    pvm_object_t                        page;
    int                                 page_size; // current page size
    int                                 used_slots; // how many slots are used now
};



struct data_area_4_call_frame
{
    pvm_object_t                        istack; // integer (fast calc) stack
    pvm_object_t                        ostack; // main object stack
    pvm_object_t                        estack; // exception catchers

    //pvm_object_t                      cs;     // code segment, ref just for gc - OK without

    unsigned int                        IP_max; // size of code in bytes
    unsigned char *                     code;   // (byte)code itself

    unsigned int                        IP;

    pvm_object_t                        this_object;

    pvm_object_t                        prev; // where to return

    int                                 ordinal; // num of method we run

};



struct data_area_4_code
{
    unsigned int                        code_size; // bytes
    unsigned char                       code[];
};




struct data_area_4_int
{
    int                                 value;
};

#define pvm_get_int( o )  ( (const int) (((struct data_area_4_int *)&(o->da))->value))

struct data_area_4_long
{
    int64_t                             value;
};

#define pvm_get_long( o )  ( (const int64_t) (((struct data_area_4_long *)&(o->da))->value))


struct data_area_4_float
{
    float                               value;
};

#define pvm_get_float( o )  ( (const float) (((struct data_area_4_float *)&(o->da))->value))

struct data_area_4_double
{
    double                              value;
};

#define pvm_get_double( o )  ( (const double) (((struct data_area_4_double *)&(o->da))->value))



struct data_area_4_string
{
    int                                 length; // bytes! (chars are unicode?)
    unsigned char                       data[];
};


int pvm_strcmp(pvm_object_t s1, pvm_object_t s2);


// NB! See JIT assembly hardcode for object structure offsets
struct data_area_4_class
{
    unsigned int                        object_flags;                   // object of this class will have such flags
    unsigned int                        object_data_area_size;  // object of this class will have data area of this size
    pvm_object_t                        object_default_interface; // default one

    unsigned int                        sys_table_id; // See above - index into the kernel's syscall tables table

    pvm_object_t                        class_name;
    pvm_object_t                        class_parent;

    pvm_object_t                        static_vars; // array of static variables

    pvm_object_t                        ip2line_maps; // array of maps: ip->line number
    pvm_object_t                        method_names; // array of method names
    pvm_object_t                        field_names; // array of field names

    pvm_object_t                        const_pool; // array of object constants
};



struct pvm_code_handler
{
    const unsigned char *               code;
    unsigned int                        IP;   /* Instruction Pointer */
    unsigned int                        IP_max;
};







struct pvm_stack_da_common
{
    /**
     *
     * Root (first created) page of the stack.
     * rootda is shortcut pointer to root's data area (struct data_area_4_XXX_stack).
     *
     */
    pvm_object_t                        root;

    /**
     *
     * Current (last created) page of the stack.
     * This field is used in root stack page only.
     *
     * See curr_da below - it is a shortcut to curr object's data area.
     *
     * See also '#define set_me(to)' in stacks.c.
     *
     */
    pvm_object_t                        curr;

    /** Pointer to previous (older) stack page. */
    pvm_object_t                        prev;

    /** Pointer to next (newer) stack page. */
    pvm_object_t                        next;

    /** number of cells used. */
    unsigned int                        free_cell_ptr;

    /** Page array has this many elements. */
    unsigned int                        __sSize;
};


#define PVM_INTEGER_STACK_SIZE 64

struct data_area_4_integer_stack
{
    struct pvm_stack_da_common          common;

    struct data_area_4_integer_stack *  curr_da;

    int                                 stack[PVM_INTEGER_STACK_SIZE];
};




#define PVM_OBJECT_STACK_SIZE 64

struct data_area_4_object_stack
{
    struct pvm_stack_da_common          common;

    struct data_area_4_object_stack *   curr_da;

    pvm_object_t                        stack[PVM_OBJECT_STACK_SIZE];
};



#define PVM_EXCEPTION_STACK_SIZE 64

struct data_area_4_exception_stack
{
    struct pvm_stack_da_common          common;

    struct data_area_4_exception_stack *curr_da;

    struct pvm_exception_handler        stack[PVM_EXCEPTION_STACK_SIZE];
};


struct data_area_4_boot
{
    // Empty?
};



//#define PVM_DEF_TTY_XSIZE 800
//#define PVM_DEF_TTY_YSIZE 600
#define PVM_DEF_TTY_XSIZE 400
#define PVM_DEF_TTY_YSIZE 300
//#define PVM_MAX_TTY_PIXELS (1280*1024)
#define PVM_MAX_TTY_PIXELS (PVM_DEF_TTY_XSIZE*PVM_DEF_TTY_YSIZE)

#define PVM_MAX_TTY_TITLE 128

struct data_area_4_tty
{
#if NEW_WINDOWS
    window_handle_t                     w;
    rgba_t                              pixel[PVM_MAX_TTY_PIXELS];
#else
    drv_video_window_t                  w;
    /** this field extends w and works as it's last field. */
    rgba_t                              pixel[PVM_MAX_TTY_PIXELS];
#endif
    int                                 font_height; // Font is hardcoded yet - BUG - but we cant have ptr to kernel from object
    int                                 font_width;
    int                                 xsize, ysize; // in chars
    int                                 x, y; // in pixels
    rgba_t                              fg, bg; // colors

    char                                title[PVM_MAX_TTY_TITLE+1];
};




struct data_area_4_binary
{
    int                                 data_size; // GCC refuses to have only unsized array in a struct
    unsigned char                       data[];
};


struct data_area_4_closure
{
    /** Which object to call. */
    pvm_object_t                        object;

    /** Which method to call. */
    int                                 ordinal;
};


struct data_area_4_bitmap
{
    pvm_object_t                        image; // .internal.binary
    int                                 xsize;
    int                                 ysize;
};


struct data_area_4_tcp
{
    //int waiting for recv
    int                                 connected;
    uint32_t                            ipaddr;
    uint32_t                            port;
};

// move to ../phantom/vm/sys/i_udp.h and include
#include "../phantom/vm/sys/i_udp.h"
#include "../phantom/vm/sys/i_net.h"
#include "../phantom/vm/sys/i_http.h"
#include "../phantom/vm/sys/i_time.h"
#include "../phantom/vm/sys/i_stat.h"
#include "../phantom/vm/sys/i_io.h"
#include "../phantom/vm/sys/i_port.h"
#include "../phantom/vm/sys/i_ui_control.h"
#include "../phantom/vm/sys/i_ui_font.h"



struct data_area_4_world
{
    int                                 placeholder[8]; // for any case
};




#define WEAKREF_SPIN 1



struct data_area_4_weakref
{
    /** Object we point to */
    pvm_object_t                        object;
#if WEAKREF_SPIN
    pvm_spinlock_t                      lock;   // interlocks access tp object from GC finalizer and from getting ref
#else
    hal_mutex_t                         mutex;
#endif
};


// TODO BUG! Size hardcode! Redo with separate object!
#define PVM_MAX_WIN_PIXELS 1024*1024

struct data_area_4_window
{
    pvm_object_t                        o_pixels;       //< .i.binary object containing bitmap for window
    pvm_object_t                        connector;      // Used for callbacks - events

    uint32_t                            x, y; // in pixels
    rgba_t                              fg, bg; // colors

    //pvm_object_t                      event_handler; // connection!
    char                                title[PVM_MAX_TTY_TITLE+1]; // TODO wchar_t

    uint32_t                            autoupdate;

};




#define DIR_MUTEX_O 1 // TODO kill me

// Very dumb implementation, redo with hash map or bin search or tree
// Container entry has pvm_object_t at the beginning and the rest is 0-term name string
struct data_area_4_directory
{
    u_int32_t                           capacity;       // size of 1nd level arrays
    u_int32_t                           nEntries;       // number of actual entries stored

    pvm_object_t                        keys;           // Where we actually hold keys
    pvm_object_t                        values;         // Where we actually hold values
    u_int8_t                           *flags;          // Is this keys/values slot pointing to 2nd level array

    pvm_spinlock_t                      pvm_lock;

};

typedef struct data_area_4_directory hashdir_t;

// directory.c
errno_t hdir_add( hashdir_t *dir, const char *ikey, size_t i_key_len, pvm_object_t add );
errno_t hdir_find( hashdir_t *dir, const char *ikey, size_t i_key_len, pvm_object_t *out, int delete_found );
//! Get all keys as array
errno_t hdir_keys( hashdir_t *dir, pvm_object_t *out );







// -----------------------------------------------------------------------
//
// Sync related
//
// -----------------------------------------------------------------------








struct data_area_4_thread
{
    struct pvm_code_handler             code;           // Loaded by load_fast_acc from frame

    pvm_object_t                        call_frame;     // current

    pvm_object_t                        owner;
    pvm_object_t                        environment;

    //hal_spinlock_t                      spin;
    pvm_spinlock_t                      lock;           // used on manipulations with sleep_flag

#if NEW_VM_SLEEP
    volatile int                        sleep_flag;     // Is true if thread is put asleep in userland
    VM_SPIN_TYPE *                      spin_to_unlock; // This spin will be unlocked after putting thread asleep
    pvm_object_t                        cond_mutex;     // If we wait for cond, this mutex was unlocked by cond and has to be relocked
#endif
    //timedcall_t                         timer;          // Who will wake us
    //net_timer_event                     timer;          // Who will wake us
    //pvm_object_t                        sleep_chain;    // More threads sleeping on the same event, meaningless for running thread

    int                                 tid;            // Actual kernel thread id - reloaded on each kernel restart

// fast access copies

    pvm_object_t                        _this_object;   // Loaded by load_fast_acc from call_frame

    struct data_area_4_integer_stack *  _istack;        // Loaded by load_fast_acc from call_frame
    struct data_area_4_object_stack *   _ostack;        // Loaded by load_fast_acc from call_frame
    struct data_area_4_exception_stack *_estack;        // Loaded by load_fast_acc from call_frame


    // misc data
    int                                 stack_depth;    // number of frames
    //long                              memory_size;    // memory allocated - deallocated by this thread
};

typedef struct data_area_4_thread thread_context_t;










struct data_area_4_mutex
{
    pvm_spinlock_t                      lock;

        // We can't lock spin when we access objects or we have chance
        // to die because of page fault with spin locked.
        // We lock in_method instead and check if it is locked and
        // spin around in simple hal_sleep_msec 

        // TODO in_method can be paged out too, try accessing it before locking in spin!
        // TODO better move it to spinlock struct and write funcs for pvm_spin_lock/unlock
        //int                 in_method;

    struct data_area_4_thread *         owner_thread;

    pvm_object_t                        waiting_threads_array;
    int                                 nwaiting;
};

struct data_area_4_cond
{
    pvm_spinlock_t                      lock;

    //struct data_area_4_thread *         owner_thread;

    pvm_object_t                        waiting_threads_array;
    int                                 nwaiting;
};

struct data_area_4_sema
{
    pvm_spinlock_t                      lock;

    struct data_area_4_thread *         owner_thread;

    pvm_object_t                        waiting_threads_array;
    int                                 nwaiting;

    int                                 sem_value;
};











struct data_area_4_connection;

#define PVM_CONNECTION_WAKE 0

struct pvm_connection_ops
{
#if PVM_CONNECTION_WAKE
    // No block!

    // Check if op can be done, return 0 if so
    errno_t     (*check_operation)( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *tc );

    // request to wake me up when ready
    errno_t     (*req_wake)( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *tc );
#endif

    // Actually do op 
    errno_t     (*do_operation)( int op_no, struct data_area_4_connection *c, struct data_area_4_thread *tc, pvm_object_t o );

    // Init connection
    errno_t     (*init)( struct data_area_4_connection *c, struct data_area_4_thread *tc, const char *suffix );

    // Finish connection
    //errno_t     (*disconnect)( struct data_area_4_connection *c, struct data_area_4_thread *tc );
    errno_t     (*disconnect)( struct data_area_4_connection *c );
};




struct data_area_4_connection
{
    struct data_area_4_thread *         owner;          // Just this one can use
    struct pvm_connection_ops *         kernel;         // Stuff in kernel that serves us

    pvm_object_t                        callback;
    int                                 callback_method;
    int                                 n_active_callbacks;

    // Persistent kernel state, p_kernel_state_object is binary
    size_t                              p_kernel_state_size;
    pvm_object_t                        p_kernel_state_object;
    void *                              p_kernel_state;

    pvm_object_t                        (*blocking_syscall_worker)
                                            ( 
                                              pvm_object_t this, struct data_area_4_thread *tc, 
                                              int nmethod, pvm_object_t arg 
                                            );

    // Volatile kernel state
    size_t                              v_kernel_state_size;
    void *                              v_kernel_state;

    char                                name[1024];     // Used to reconnect on restart
};

errno_t phantom_connect_object( struct data_area_4_connection *da, struct data_area_4_thread *tc);
//errno_t phantom_disconnect_object( struct data_area_4_connection *da, struct data_area_4_thread *tc);
errno_t phantom_disconnect_object( struct data_area_4_connection *da );


// Internal connect - when connection is used by other object (host_object)
//int pvm_connect_object_internal(struct data_area_4_connection *da, int connect_type, pvm_object_t host_object, void *arg);
errno_t phantom_connect_object_internal(struct data_area_4_connection *da, int connect_type, pvm_object_t host_object, void *arg);



#endif // PVM_INTERNAL_DA_H

