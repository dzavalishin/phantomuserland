/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Bytecode interpreter.
 *
 *
**/


#include <phantom_assert.h>

#include <vm/root.h>
#include <vm/internal_da.h>
#include <vm/internal.h>
#include <vm/object_flags.h>
#include <vm/exception.h>
#include <vm/alloc.h>

#include <vm/exec.h>
#include <vm/code.h>
#include <vm/reflect.h>

#include <vm/stacks.h>
#include <vm/syscall.h>

#include "ids/opcode_ids.h"

#include <kernel/snap_sync.h>
#include <kernel/debug.h>

#include <exceptions.h>


static errno_t find_dynamic_method( dynamic_method_info_t *mi );


/*
 * NB! Refcount contract:
 *        all push/pop/get/set operations with objects just copy pointers and do not change refcount
 *        - someone who consumes the arg must!
 */


#define DEB_CALLRET 0
#define DEB_DYNCALL 0

//static int debug_print_instr = 1;
int debug_print_instr = 0;

#define LISTI(iName) do { if( debug_print_instr ) lprintf("%s @ %d; ",(iName), da->code.IP); } while(0)
#define LISTIA(fmt,a) do { if( debug_print_instr ) { lprintf((fmt), a); lprintf(" @ %d; ",da->code.IP); } } while(0)



/**
 *
 * Stacks access macros
 *
**/




#define os_push( o ) 	pvm_ostack_push( da->_ostack, o )
#define os_pop() 	pvm_ostack_pop( da->_ostack )
#define os_top() 	pvm_ostack_top( da->_ostack )
#define os_empty() 	pvm_ostack_empty( da->_ostack )

#define os_pull( pos ) 	pvm_ostack_pull( da->_ostack, pos )


#define is_push( i ) 	pvm_istack_push( da->_istack, i )
#define is_pop() 	pvm_istack_pop( da->_istack )
#define is_top() 	pvm_istack_top( da->_istack )
#define is_empty() 	pvm_istack_empty( da->_istack )

#define ls_push( i ) 	pvm_lstack_push( da->_istack, i )
#define ls_pop() 	pvm_lstack_pop( da->_istack )
#define ls_top() 	pvm_lstack_top( da->_istack )
#define ls_empty() 	pvm_lstack_empty( da->_istack )


#define es_push( i ) 	pvm_estack_push( da->_estack, i )
#define es_pop() 	pvm_estack_pop( da->_estack )
#define es_empty()      pvm_estack_empty( da->_estack )


#define this_object()   (da->_this_object)


/**
 *
 * bit representation saving type conv
 *
**/

// v must be lvalue
#define TO_DOUBLE( __v ) (*((double *)&(__v)))
#define TO_LONG( __v ) (*((u_int64_t *)&(__v)))
#define TO_FLOAT( __v ) (*((float *)&(__v)))
#define TO_INT( __v ) (*((u_int32_t *)&(__v)))

#define AS_DOUBLE( __a1, __a2, __op ) (TO_DOUBLE(__a1) __op TO_DOUBLE(__a2))
#define AS_FLOAT( __a1, __a2, __op )  (TO_FLOAT(__a1)  __op TO_FLOAT(__a2))




#define DOUBLE_STACK_OP( ___op ) \
do { \
	int64_t a1 = ls_pop(); \
        int64_t a2 = ls_pop(); \
        double r = AS_DOUBLE( a1, a2, ___op ); \
        ls_push( TO_LONG(r) ); \
} while(0)


#define FLOAT_STACK_OP( ___op ) \
do { \
	int32_t a1 = is_pop(); \
        int32_t a2 = is_pop(); \
        float r = AS_FLOAT( a1, a2, ___op ); \
        is_push( TO_INT(r) ); \
} while(0)



/**
 *
 * Helpers
 *
**/



void pvm_exec_load_fast_acc(struct data_area_4_thread *da)
{
    struct data_area_4_call_frame *cf = (struct data_area_4_call_frame *)&(da->call_frame.data->da);

    da->code.IP_max  = cf->IP_max;
    da->code.code    = cf->code;

    da->code.IP      = cf->IP;  /* Instruction Pointer */

    da->_this_object = cf->this_object;
    da->_istack = (struct data_area_4_integer_stack*)(& cf->istack.data->da);
    da->_ostack = (struct data_area_4_object_stack*)(& cf->ostack.data->da);
    da->_estack = (struct data_area_4_exception_stack*)(& cf->estack.data->da);
}

void pvm_exec_save_fast_acc(struct data_area_4_thread *da)
{
    struct data_area_4_call_frame *cf = (struct data_area_4_call_frame *)&(da->call_frame.data->da);
    cf->IP = da->code.IP;
}



/**
 *
 * Workers.
 *
**/

static void pvm_exec_load( struct data_area_4_thread *da, unsigned slot )
{
    LISTIA("os load %d", slot);
    struct pvm_object o = pvm_get_ofield( this_object(), slot);
    os_push( ref_inc_o (o) );
}

static void pvm_exec_save( struct data_area_4_thread *da, unsigned slot )
{
    LISTIA("os save %d", slot);
    pvm_set_ofield( this_object(), slot, os_pop() );
}


static void pvm_exec_iload( struct data_area_4_thread *da, unsigned slot )
{
    LISTIA("is load %d", slot);
    int v = pvm_get_int( pvm_get_ofield( this_object(), slot) );
    is_push( v );
}

static void pvm_exec_isave( struct data_area_4_thread *da, unsigned slot )
{
    LISTIA("is save %d", slot);
    pvm_set_ofield( this_object(), slot, pvm_create_int_object(is_pop()) );
}


static void pvm_exec_get( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    LISTIA("os stack get %d", abs_stack_pos);
    pvm_object_t o = pvm_ostack_abs_get(da->_ostack, abs_stack_pos);
    os_push( ref_inc_o(o) );
}

static void pvm_exec_set( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    LISTIA("os stack set %d", abs_stack_pos);
    pvm_ostack_abs_set( da->_ostack, abs_stack_pos, os_pop() );
}


static void pvm_exec_iget( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    LISTIA("is stack get %d", abs_stack_pos);
    is_push( pvm_istack_abs_get(da->_istack, abs_stack_pos) );
}

static void pvm_exec_iset( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    LISTIA("is stack set %d", abs_stack_pos);
    pvm_istack_abs_set( da->_istack, abs_stack_pos, is_pop() );
}


static void free_call_frame(struct pvm_object cf, struct data_area_4_thread *da)
{
    pvm_object_da( cf, call_frame )->prev.data = 0; // Or else refcounter will follow this link
    ref_dec_o( cf ); // we are erasing reference to old call frame - release it!

    da->stack_depth--;
}


static void pvm_exec_do_return(struct data_area_4_thread *da)
{
    struct pvm_object ret = pvm_object_da( da->call_frame, call_frame )->prev; // prev exists - we guarantee it

    pvm_object_t return_val = os_empty() ?  pvm_get_null_object() : os_pop(); // return value not in stack? Why?

    free_call_frame( da->call_frame, da );

    da->call_frame = ret;
    pvm_exec_load_fast_acc(da);

    os_push( return_val );
}


static int pvm_exec_find_catch(
                               struct data_area_4_exception_stack* stack,
                               unsigned int *jump_to,
                               struct pvm_object thrown_obj );


// object to throw is on stack
static void pvm_exec_do_throw(struct data_area_4_thread *da)
{
    unsigned int jump_to = (unsigned int)-1; // to cause fault
    struct pvm_object thrown_obj = os_pop();
    // call_frame.catch_found( &jump_to, thrown_obj )
    while( !(pvm_exec_find_catch( da->_estack, &jump_to, thrown_obj )) )
    {
        // like ret here
        LISTI("except does unwind, ");
        struct pvm_object ret = pvm_object_da( da->call_frame, call_frame )->prev;

        if( pvm_is_null( ret ) )
        {
            printf("Unhandled exception: ");
            pvm_object_print( thrown_obj );
            printf("\n");
            //getchar();

            pvm_exec_panic( "unwind: nowhere to return" );
        }
        free_call_frame( da->call_frame, da );

        da->call_frame = ret;
        pvm_exec_load_fast_acc(da);
    }
    LISTIA("except jump to %d", jump_to);
    da->code.IP = jump_to;
    os_push(thrown_obj);
}


static syscall_func_t pvm_exec_find_syscall( struct pvm_object _class, unsigned int syscall_index );


// syscalss numbers are specific to object class
static void pvm_exec_sys( struct data_area_4_thread *da, unsigned int syscall_index )
{
    LISTIA("sys %d start", syscall_index );

    //n_args = is_top();

    // which object's syscall we'll call
    struct pvm_object o = this_object();

    syscall_func_t func = pvm_exec_find_syscall( o.data->_class, syscall_index );

    if( func == 0 )
    {
        LISTIA("sys %d invalid! ", syscall_index );
    }
    else
    {
        // TODO dec refcount for all the args after call?
        // OR! in syscall arg pop?
        // OR! in syscall code?
        if( func( o, da ) == 0 )	// exec syscall
            pvm_exec_do_throw(da); // exception reported
    }

    LISTIA("sys %d end", syscall_index );
}



static void init_cfda(struct data_area_4_thread *da, struct data_area_4_call_frame *cfda, unsigned int method_index, unsigned int n_param, pvm_object_t new_this )
{
    cfda->ordinal = method_index;
    // which object's method we'll call - pop after args!

#if 1
    // TODO warn? print call info?
    if( n_param > (1024*16) )
    {
        lprintf("n_param too big: %d\n", n_param );
        //n_param = 1024*16; // no - stack underflow will follow
    }
#endif
    // allocate places on stack
    {
        unsigned int i;
        for( i = n_param; i; i-- )
        {
            pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), pvm_get_null_object() );
        }
    }

    // fill 'em in correct order
    {
        unsigned int i;
        for( i = n_param; i; i-- )
        {
            pvm_ostack_abs_set( pvm_object_da(cfda->ostack, object_stack), i-1, os_pop() );
        }
    }

    // pass him real number of parameters
    pvm_istack_push( pvm_object_da(cfda->istack, integer_stack), n_param);

    if( pvm_is_null(new_this) )
        new_this = os_pop();

    struct pvm_object_storage *code = pvm_exec_find_method( new_this, method_index );
    assert(code != 0);
    pvm_exec_set_cs( cfda, code );
    cfda->this_object = new_this;
}


static void pvm_exec_call( struct data_area_4_thread *da, unsigned int method_index, unsigned int n_param, int do_optimize_ret, pvm_object_t new_this )
{
    if( DEB_CALLRET || debug_print_instr ) printf( "\ncall %d (stack_depth %d -> ", method_index, da->stack_depth );

    /*
     * Stack growth optimization for bytecode [opcode_call; opcode_ret]
     * and [opcode_call; opcode_os_drop; opcode_ret]
     *
     * While executing "return f(x)"  we do not need callee callframe any more
     * so we can free it before executing f(x), to avoid unneeded stack growth
     * and memory footprint. Effective for tail recursion and mutual recursion.
     * Implemented in gcc -O2 long ago.
     */
    int optimize_stack = 0;

    if ((opcode_os_drop == pvm_code_get_byte_speculative(&(da->code)) &&
         opcode_ret == pvm_code_get_byte_speculative2(&(da->code))
        ) ||
        (opcode_ret == pvm_code_get_byte_speculative(&(da->code)) &&
         do_optimize_ret
        )
       )
    {
        optimize_stack = es_empty();
    }

    pvm_exec_save_fast_acc(da);  // not needed for optimized stack in fact

    struct pvm_object new_cf = pvm_create_call_frame_object();
    struct data_area_4_call_frame* cfda = pvm_object_da( new_cf, call_frame );

    init_cfda(da, cfda, method_index, n_param, new_this);

    if (!optimize_stack)
        cfda->prev = da->call_frame;  // link
    else
    {
        cfda->prev = pvm_object_da( da->call_frame, call_frame )->prev; // set

        free_call_frame( da->call_frame, da );

        if( DEB_CALLRET || debug_print_instr ) printf( "*" );
    }

    da->stack_depth++;
    da->call_frame = new_cf;
    pvm_exec_load_fast_acc(da);

    if( DEB_CALLRET || debug_print_instr ) printf( "%d); ", da->stack_depth );
}



/**
 *
 * Well... here is the root of all evil. The bytecode interpreter itself.
 *
 * This function returns true if top bytecode method returns or uncatched exception happens.
 *
 * TODO: exception must be recorded somehow, no?
 *
 * TODO: have special checkpoint to check for snapshot is going to start. Pause and save_fast_acc,
 * report being ready for snap. Will be unpaused by snapshot code after finishing 1st phase (setting
 * all pages to readonly).
 *
 * TODO: Long (blocking) operations called from here can't be recorded in snapshot (C stack is
 * not in snap and can't be there), so we have to restart them each snap - set a longjmp return
 * point for that, return there and restart last opcode on restart.
 *
 * TODO: If thread was in a driver during snap, do a different longjmp, and make a mark in a thread's
 * state that if thread is restored, it has to get special REBOOT exception delivered. Reset that flag
 * after returning from syscall and before being ready for a next snap..
 *
 *
 *
**/

static void do_pvm_exec(pvm_object_t current_thread)
{
    int prefix_long = 0;
    int prefix_float = 0;
    int prefix_double = 0;

#define DO_TWICE  (prefix_long || prefix_double)
#define DO_FPOINT (prefix_float || prefix_double)

    if( !pvm_object_class_exactly_is( current_thread, pvm_get_thread_class() ))
        panic("attempt to run not a thread");

    struct data_area_4_thread *da = (struct data_area_4_thread *)&(current_thread.data->da);

    pvm_exec_load_fast_acc(da); // For any case

#if OLD_VM_SLEEP
    // Thread was snapped sleeping - resleep it
    if(da->sleep_flag)
        phantom_thread_sleep_worker( da );
#else
#warning resleep?
#endif

    while(1)
    {

#if NEW_SNAP_SYNC
        touch_snap_catch(); // touch special memory page, if snap is scheduled, page will be write-protected and we'll gen page fault
#else
        if(phantom_virtual_machine_snap_request)
        {
            pvm_exec_save_fast_acc(da); // Before snap
            phantom_thread_wait_4_snap();
            //pvm_exec_load_fast_acc(da); // We don't need this, if we die, we will enter again from above :)
        }
#endif

#if 0 // GC_ENABLED  // GC can be enabled here for test purposes only.
        static int gcc = 0;
        gcc++;
        if( gcc > 20000 )
        {
            gcc = 0;
            run_gc();
        }
#endif // GC_ENABLED

        unsigned char instruction = pvm_code_get_byte(&(da->code));
        //printf("instr 0x%02X ", instruction);

        if( prefix_long )
        {
            prefix_long = 0;
            switch(instruction)
            {
            default:
                prefix_long = 1;  // attempt nonprefix impl of inctruction, maybe it checks for a modifier
                goto noprefix;

            case opcode_ishl:
                LISTI("l-ishl");
                {
                    int64_t val = ls_pop();
                    ls_push( val << is_pop() );
                }
                break;

            case opcode_ishr:
                LISTI("l-ishr");
                {
                    int64_t val = ls_pop();
                    ls_push( val >> is_pop() );
                }
                break;

            case opcode_ushr:
                LISTI("l-ushr");
                {
                    u_int64_t val = ls_pop();
                    ls_push( val >> is_pop() );
                }
                break;


            case opcode_isum:
                LISTI("l-isum");
                {
                    int64_t add = ls_pop();
                    ls_push( ls_pop() + add );
                }
                break;

            case opcode_imul:
                LISTI("l-imul");
                {
                    int64_t mul = ls_pop();
                    ls_push( ls_pop() * mul );
                }
                break;

            case opcode_isubul:
                LISTI("l-isubul");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(u-l);
                }
                break;

            case opcode_isublu:
                LISTI("l-isublu");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(l-u);
                }
                break;

            case opcode_idivul:
                LISTI("l-idivul");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(u/l);
                }
                break;

            case opcode_idivlu:
                LISTI("l-idivlu");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(l/u);
                }
                break;

            case opcode_ior:
                LISTI("l-ior");
                { int64_t operand = ls_pop();	ls_push( ls_pop() | operand ); }
                break;

            case opcode_iand:
                LISTI("l-iand");
                { int64_t operand = ls_pop();	ls_push( ls_pop() & operand ); }
                break;

            case opcode_ixor:
                LISTI("l-ixor");
                { int64_t operand = ls_pop();	ls_push( ls_pop() ^ operand ); }
                break;

            case opcode_inot:
                LISTI("l-inot");
                { int64_t operand = ls_pop();	ls_push( ~operand ); }
                break;



            case opcode_log_or:
                LISTI("l-lor");
                {
                    int64_t o1 = ls_pop();
                    int64_t o2 = ls_pop();
                    ls_push( o1 || o2 );
                }
                break;

            case opcode_log_and:
                LISTI("l-land");
                {
                    int64_t o1 = ls_pop();
                    int64_t o2 = ls_pop();
                    ls_push( o1 && o2 );
                }
                break;

            case opcode_log_xor:
                LISTI("l-lxor");
                {
                    int64_t o1 = ls_pop() ? 1 : 0;
                    int64_t o2 = ls_pop() ? 1 : 0;
                    ls_push( o1 ^ o2 );
                }
                break;

            case opcode_log_not:
                LISTI("l-lnot");
                {
                    int64_t operand = ls_pop();
                    ls_push( !operand );
                }
                break;

                // NB! Returns int!
            case opcode_ige:	// >=
                LISTI("l-ige");
                { int64_t operand = ls_pop();	is_push( ls_pop() >= operand ); }
                break;
            case opcode_ile:	// <=
                LISTI("l-ile");
                { int64_t operand = ls_pop();	is_push( ls_pop() <= operand ); }
                break;
            case opcode_igt:	// >
                LISTI("l-igt");
                { int64_t operand = ls_pop();	is_push( ls_pop() > operand ); }
                break;
            case opcode_ilt:	// <
                LISTI("l-ilt");
                { int64_t operand = ls_pop();	is_push( ls_pop() < operand ); }
                break;

            case opcode_froml:
                LISTI("l-froml (nop)");
                break;

            case opcode_fromi:
                LISTI("l-fromi");
                {
                    ls_push( is_pop() );
                }
                break;

            case opcode_fromd:
                LISTI("l-fromd");
                {
                    long l = ls_pop();
                    double d = TO_DOUBLE( l );
                    ls_push( (long) d );
                }
                break;

            case opcode_fromf:
                LISTI("l-fromf");
                {
                    int i = is_pop();
                    float f = TO_FLOAT( i );
                    ls_push( (long) f );
                }
                break;


            case opcode_i2o:
                LISTI("l-i2o");
                os_push(pvm_create_long_object(ls_pop()));
                break;

            case opcode_o2i:
                LISTI("l-o2i");
                {
                    struct pvm_object o = os_pop();
                    if( o.data == 0 ) pvm_exec_panic("l-o2i(null)");
                    ls_push( pvm_get_long( o ) );
                    ref_dec_o(o);
                }
                break;
            }
            // End of long ops
            goto noops;
    	} // if(prefix_long)

        if( prefix_float )
        {
            prefix_float = 0;
            switch(instruction)
            {
            default: // Try classic implementation of that op
                prefix_float = 1; // attempt nonprefix impl of inctruction, maybe it checks for a modifier
                goto noprefix;
                //pvm_exec_panic("invalid double op");
                //break;

            case opcode_ishl: // Not defined for float, throw exception
            case opcode_ishr:
            case opcode_ushr:
            case opcode_ior:
            case opcode_iand:
            case opcode_ixor:
            case opcode_inot:
            case opcode_log_or:
            case opcode_log_and:
            case opcode_log_xor:
            case opcode_log_not:
                pvm_exec_panic("invalid float op");
                break;

            case opcode_isum:
                LISTI("f-isum");
                FLOAT_STACK_OP( + );
                break;

            case opcode_imul:
                LISTI("f-imul");
                FLOAT_STACK_OP( * );
                break;

            case opcode_isubul:
                LISTI("f-isubul");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    float r = AS_FLOAT( u, l, - );
                    is_push( TO_INT(r) );
                }
                break;

            case opcode_isublu:
                LISTI("f-isublu");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    float r = AS_FLOAT( l, u, - );
                    is_push( TO_INT(r) );
                }
                break;

            case opcode_idivul:
                LISTI("f-idivul");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    float r = AS_FLOAT( u, l, / );
                    is_push( TO_INT(r) );
                }
                break;

            case opcode_idivlu:
                LISTI("f-idivlu");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    float r = AS_FLOAT( l, u, / );
                    is_push( TO_INT(r) );
                }
                break;





                // NB! Returns int!
            case opcode_ige:
                LISTI("f-ige");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    int r = AS_FLOAT( l, u, >= );
                    is_push( r );
                }
                break;
            case opcode_ile:
                LISTI("f-ile");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    int r = AS_FLOAT( l, u, <= );
                    is_push( r );
                }
                break;
            case opcode_igt:
                LISTI("f-igt");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    int r = AS_FLOAT( l, u, > );
                    is_push( r );
                }
                break;
            case opcode_ilt:
                LISTI("f-ilt");
                {
                    int32_t u = is_pop();
                    int32_t l = is_pop();
                    int r = AS_FLOAT( l, u, < );
                    is_push( r );
                }
                break;


            case opcode_fromf:
                LISTI("f-fromf (nop)");
                break;

            case opcode_fromi:
                LISTI("f-fromi");
                {
                    float i = is_pop();
                    is_push( TO_INT( i ) );
                }
                break;

            case opcode_froml:
                LISTI("f-froml");
                {
                    float l = ls_pop();
                    is_push( TO_INT( l ) );
                }
                break;

            case opcode_fromd:
                LISTI("f-fromd");
                {
                    int64_t l = ls_pop();
                    float f = (float) TO_DOUBLE( l );
                    is_push( TO_INT( f ) );
                }
                break;

            case opcode_i2o:
                LISTI("f-i2o");
                {
                    //pvm_exec_panic("unimpl float i2o");
                    int32_t d = is_pop();
                    os_push( pvm_create_float_object( TO_FLOAT(d) ) );
                }
                break;

            case opcode_o2i:
                LISTI("f-o2i");
                //pvm_exec_panic("unimpl float o2i");
                {
                    struct pvm_object o = os_pop();
                    if( o.data == 0 ) pvm_exec_panic("f-o2i(null)");
                    float d = pvm_get_float( o );
                    is_push( TO_INT( d ) );
                    ref_dec_o(o);
                }
                break;

            }
            // End of float ops
            goto noops;
    	} // if(prefix_float)

        if( prefix_double )
        {
            prefix_double = 0;

            switch(instruction)
            {
            default:
                prefix_double = 1;  // attempt nonprefix impl of inctruction, maybe it checks for a modifier
                goto noprefix;

            case opcode_ishl: // Not defined for double, throw exception
            case opcode_ishr:
            case opcode_ushr:
            case opcode_ior:
            case opcode_iand:
            case opcode_ixor:
            case opcode_inot:
            case opcode_log_or:
            case opcode_log_and:
            case opcode_log_xor:
            case opcode_log_not:
                pvm_exec_panic("invalid double op");
                break;


            case opcode_isum:
                LISTI("d-isum");
                DOUBLE_STACK_OP( + );
                break;

            case opcode_imul:
                LISTI("d-imul");
                DOUBLE_STACK_OP( * );

//                {                    int64_t mul = ls_pop();                    ls_push( ls_pop() * mul );                }
                break;

            case opcode_isubul:
                LISTI("d-isubul");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    double r = AS_DOUBLE( u, l, - );
                    ls_push( TO_LONG(r) );
                }
                break;

            case opcode_isublu:
                LISTI("d-isublu");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    double r = AS_DOUBLE( l, u, - );
                    ls_push( TO_LONG(r) );
                }
                break;

            case opcode_idivul:
                LISTI("d-idivul");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    double r = AS_DOUBLE( u, l, / );
                    ls_push( TO_LONG(r) );
                }
                break;

            case opcode_idivlu:
                LISTI("d-idivlu");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    double r = AS_DOUBLE( l, u, / );
                    ls_push( TO_LONG(r) );
                }
                break;





                // NB! Returns int!
            case opcode_ige:
                LISTI("d-ige");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    int r = AS_DOUBLE( l, u, >= );
                    is_push( r );
                }
                break;
            case opcode_ile:
                LISTI("d-ile");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    int r = AS_DOUBLE( l, u, <= );
                    is_push( r );
                }
                break;
            case opcode_igt:
                LISTI("d-igt");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    int r = AS_DOUBLE( l, u, > );
                    is_push( r );
                }
                break;
            case opcode_ilt:
                LISTI("d-ilt");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    int r = AS_DOUBLE( l, u, < );
                    is_push( r );
                }
                break;


            case opcode_fromd:
                LISTI("d-fromd (nop)");
                break;

            case opcode_fromi:
                LISTI("d-fromi");
                {
                    double i = is_pop();
                    ls_push( TO_LONG( i ) );
                }
                break;

            case opcode_froml:
                LISTI("d-froml");
                {
                    double l = ls_pop();
                    ls_push( TO_LONG( l ) );
                }
                break;

            case opcode_fromf:
                LISTI("d-fromf");
                {
                    int i = is_pop();
                    double f = TO_FLOAT( i );
                    ls_push( TO_LONG( f ) );
                }
                break;

            case opcode_i2o: // ERROR IMPLEMENT ME
                LISTI("d-i2o");
                {
                    //pvm_exec_panic("unimpl double i2o");
                    int64_t d = ls_pop();
                    os_push( pvm_create_double_object( TO_DOUBLE(d) ) );
                }
                break;

            case opcode_o2i: // ERROR IMPLEMENT ME
                LISTI("d-o2i");
                //pvm_exec_panic("unimpl double o2i");
                {
                    struct pvm_object o = os_pop();
                    if( o.data == 0 ) pvm_exec_panic("d-o2i(null)");
                    double d = pvm_get_double( o );
                    ls_push( TO_LONG( d ) );
                    ref_dec_o(o);
                }
                break;
            }
            // End of double ops
            goto noops;
        } // if(prefix_double)


    noprefix:
        switch(instruction)
        {

        // type switch prefixes --------------------------------

        // NB! Not break, continue, or else code after the main switch will reset prefix and print warning

        case opcode_prefix_long:   prefix_long   = 1; continue;
        case opcode_prefix_float:  prefix_float  = 1; continue;
        case opcode_prefix_double: prefix_double = 1; continue;

        // special opcodes -------------------------------------

        case opcode_nop:
            LISTI("nop");
            break;

        case opcode_debug:
            {
                int type = pvm_code_get_byte(&(da->code)); //cf->cs.get_instr( cf->IP );
                printf("\n\nDebug 0x%02X", type );
                if( type & 0x80 )
                {
                    printf(" (" );
                    //cf->cs.get_string( cf->IP ).my_data()->print();
                    //get_string().my_data()->print();
                    pvm_object_t o = pvm_code_get_string(&(da->code));
                    pvm_object_print(o);
                    ref_dec_o(o);
                    printf(")" );
                }

                if( type & 0x01 ) debug_print_instr = 1;
                if( type & 0x02 ) debug_print_instr = 0;

                //if( cf->istack.empty() )printf(", istack empty");
                if( is_empty() ) printf(", istack empty");
                else                    printf(",\n\tistack top = %d", is_top() );
                if( os_empty() ) printf(", ostack empty");
                else
                {
                    printf(",\n\tostack top = {" );
                    pvm_object_print( os_top() );
                    printf("}" );
                }
                printf(";\n\n");
            }
            break;


            // sync ops ---------------------------------------

        case opcode_general_lock:
            LISTI("lock");
            {
                // This is java monitor, arbitrary object
                struct pvm_object lock_obj = os_pop();
                // TODO impl me
                ref_dec_o(lock_obj);
            }
            break;

        case opcode_general_unlock:
            LISTI("unlock");
            {
                // This is java monitor, arbitrary object
                struct pvm_object lock_obj = os_pop();
                // TODO impl me
                ref_dec_o(lock_obj);
            }
            break;

            // int stack ops ---------------------------------------

        case opcode_is_dup:
            LISTI("is dup");
            {
                if(DO_TWICE)
                {
                    int i1 = is_pop();
                    int i2 = is_pop();
                    is_push(i1); is_push(i2);
                    is_push(i1); is_push(i2);
                }
                else
                    is_push(is_top());
            }
            break;

        case opcode_is_drop:
            LISTI("is drop");
            is_pop(); if(DO_TWICE) is_pop();
            break;

        case opcode_iconst_0:
            LISTI("iconst 0");
            is_push(0); if(DO_TWICE) is_push(0);
            break;

        case opcode_iconst_1:
            LISTI("iconst 1");
            is_push(1); if(DO_TWICE) is_push(1);
            break;

        case opcode_iconst_8bit:
            {
                int v = pvm_code_get_byte(&(da->code));
                if(DO_TWICE) ls_push(v);
                else is_push(v);
                LISTIA("iconst8 = %d", v);
                break;
            }

        case opcode_iconst_32bit:
            {
                int v = pvm_code_get_int32(&(da->code));
                if(DO_TWICE) ls_push(v);
                else         is_push(v);
                LISTIA("iconst32 = %d", v);
                break;
            }

        case opcode_iconst_64bit:
            {
                int64_t v = pvm_code_get_int64(&(da->code));
                ls_push(v);
                LISTIA("iconst64 = %Ld", v);
                break;
            }

        case opcode_const_pool:
            {
                pvm_object_t oc = pvm_get_class( this_object() );
                struct data_area_4_class *cda = pvm_object_da( oc, class );

                int32_t id = pvm_code_get_int32(&(da->code));
                pvm_object_t co = pvm_get_ofield( cda->const_pool, id );
                os_push(co);
                LISTIA("const_pool id %d", id);
            }
            break;

        case opcode_cast:
            {
                pvm_object_t target_class = os_pop();
                pvm_object_t o = os_pop();

                // TODO cast here!

                os_push( o );
                LISTIA("cast %s", "unimplemented!");
            }
            break;


        case opcode_ishl:
            LISTI("ishl");
            {
                int val = is_pop();
                is_push( val << is_pop() );
            }
            break;

        case opcode_ishr:
            LISTI("ishr");
            {
                int val = is_pop();
                is_push( val >> is_pop() );
            }
            break;

        case opcode_ushr:
            LISTI("ushr");
            {
                unsigned val = is_pop();
                is_push( val >> is_pop() );
            }
            break;


        case opcode_isum:
            LISTI("isum");
            {
                int add = is_pop();
                //is_top() += add;
                is_push( is_pop() + add );
            }
            break;

        case opcode_imul:
            LISTI("imul");
            {
                int mul = is_pop();
                //is_top() *= mul;
                is_push( is_pop() * mul );
            }
            break;

        case opcode_isubul:
            LISTI("isubul");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(u-l);
            }
            break;

        case opcode_isublu:
            LISTI("isublu");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(l-u);
            }
            break;

        case opcode_idivul:
            LISTI("idivul");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(u/l);
            }
            break;

        case opcode_idivlu:
            LISTI("idivlu");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(l/u);
            }
            break;

        case opcode_ior:
            LISTI("ior");
            { int operand = is_pop();	is_push( is_pop() | operand ); }
            break;

        case opcode_iand:
            LISTI("iand");
            { int operand = is_pop();	is_push( is_pop() & operand ); }
            break;

        case opcode_ixor:
            LISTI("ixor");
            { int operand = is_pop();	is_push( is_pop() ^ operand ); }
            break;

        case opcode_inot:
            LISTI("inot");
            { int operand = is_pop();	is_push( ~operand ); }
            break;



        case opcode_log_or:
            LISTI("lor");
            {
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 || o2 );
            }
            break;

        case opcode_log_and:
            LISTI("land");
            {
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 && o2 );
            }
            break;

        case opcode_log_xor:
            LISTI("lxor");
            {
                int o1 = is_pop() ? 1 : 0;
                int o2 = is_pop() ? 1 : 0;
                is_push( o1 ^ o2 );
            }
            break;

        case opcode_log_not:
            LISTI("lnot");
            {
                int operand = is_pop();
                is_push( !operand );
            }
            break;


        case opcode_ige:	// >=
            LISTI("ige");
            { int operand = is_pop();	is_push( is_pop() >= operand ); }
            break;
        case opcode_ile:	// <=
            LISTI("ile");
            { int operand = is_pop();	is_push( is_pop() <= operand ); }
            break;
        case opcode_igt:	// >
            LISTI("igt");
            { int operand = is_pop();	is_push( is_pop() > operand ); }
            break;
        case opcode_ilt:	// <
            LISTI("ilt");
            { int operand = is_pop();	is_push( is_pop() < operand ); }
            break;


            case opcode_fromi:
                LISTI("i-fromi (nop)");
                break;

            case opcode_froml:
                LISTI("i-froml");
                {
                    is_push( (int) ls_pop() );
                }
                break;

            case opcode_fromd:
                LISTI("i-fromd");
                {
                    long l = ls_pop();
                    double d = TO_DOUBLE( l );
                    is_push( (int) d );
                }
                break;

            case opcode_fromf:
                LISTI("i-fromf");
                {
                    int i = is_pop();
                    float f = TO_FLOAT( i );
                    is_push( (int) f );
                }
                break;



        case opcode_i2o:
            LISTI("i2o");
            os_push(pvm_create_int_object(is_pop()));
            break;

        case opcode_o2i:
            LISTI("o2i");
            {
                struct pvm_object o = os_pop();
                if( o.data == 0 ) pvm_exec_panic("o2i(null)");
                is_push( pvm_get_int( o ) );
                ref_dec_o(o);
            }
            break;


        case opcode_os_eq:
            LISTI("os eq");
            {
                struct pvm_object o1 = os_pop();
                struct pvm_object o2 = os_pop();
                is_push( o1.data == o2.data );
                ref_dec_o(o1);
                ref_dec_o(o2);
                break;
            }

        case opcode_os_neq:
            LISTI("os neq");
            {
                struct pvm_object o1 = os_pop();
                struct pvm_object o2 = os_pop();
                is_push( o1.data != o2.data );
                ref_dec_o(o1);
                ref_dec_o(o2);
                break;
            }

        case opcode_os_isnull:
            LISTI("isnull");
            {
                struct pvm_object o1 = os_pop();
                is_push( pvm_is_null( o1 ) );
                ref_dec_o(o1);
                break;
            }
/*
        case opcode_os_push_null:
            LISTI("push null");
            {
                os_push( pvm_get_null_object() );
                break;
            }
*/

            // summoning, special ----------------------------------------------------

        case opcode_summon_null:
            LISTI("push null");
            os_push( pvm_get_null_object() ); // so what opcode_os_push_null is for then?
            break;

        case opcode_summon_thread:
            LISTI("summon thread");
            os_push( ref_inc_o( current_thread ) );
            //printf("ERROR: summon thread");
            break;

        case opcode_summon_this:
            LISTI("summon this");
            os_push( ref_inc_o( this_object() ) );
            break;

        case opcode_summon_class_class:
            LISTI("summon class class");
            // it has locked refcount
            os_push( pvm_get_class_class() );
            break;

        case opcode_summon_interface_class:
            LISTI("summon interface class");
            // locked refcnt
            os_push( pvm_get_interface_class() );
            break;

        case opcode_summon_code_class:
            LISTI("summon code class");
        	// locked refcnt
            os_push( pvm_get_code_class() );
            break;

        case opcode_summon_int_class:
            LISTI("summon int class");
        	// locked refcnt
            os_push( pvm_get_int_class() );
            break;

        case opcode_summon_string_class:
            LISTI("summon string class");
        	// locked refcnt
            os_push( pvm_get_string_class() );
            break;

        case opcode_summon_array_class:
            LISTI("summon array class");
        	// locked refcnt
            os_push( pvm_get_array_class() );
            break;

        case opcode_summon_by_name:
            {
                LISTI("summon by name");
                struct pvm_object name = pvm_code_get_string(&(da->code));
                struct pvm_object cl = pvm_exec_lookup_class_by_name( name );
                ref_dec_o(name);
                // TODO: Need throw here?
                if( pvm_is_null( cl ) ) {
                    pvm_exec_panic("summon by name: null class");
                    //printf("summon by name: null class");
                    //pvm_exec_do_throw(da);
                    break;
                }
                os_push( cl );  // cl popped from stack - don't increment
            }
            break;

            /**
             * TODO A BIG NOTE for object creation
             *
             * We must be SURE that it is NOT ever possible to pass
             * non-internal object as init data to internal and vice versa!
             * It would be a securily hole!
             *
             **/


        case opcode_new:
            LISTI("new");
            {
                pvm_object_t cl = os_pop();
                os_push( pvm_create_object( cl ) );
                //ref_dec_o( cl );  // object keep class ref
            }
            break;

        case opcode_copy:
            LISTI("copy");
            {
                pvm_object_t o = os_pop();
                os_push( pvm_copy_object( o ) );
                ref_dec_o(o);
            }
            break;

            // if you want to enable these, work out refcount
            // and security issues first!
            // compose/decompose
#if 0
        case opcode_os_compose32:
            LISTI(" compose32");
            {
                int num = pvm_code_get_int32(&(da->code));
                struct pvm_object in_class = os_pop();
                os_push( pvm_exec_compose_object( in_class, da->_ostack, num ) );
            }
            break;

        case opcode_os_decompose:
            LISTI(" decompose");
            {
                struct pvm_object to_decomp = os_pop();
                int num = da_po_limit(to_decomp.data);
                is_push( num );
                while( num )
                {
                    num--;
                    struct pvm_object o = pvm_get_ofield( to_decomp, num);
                    os_push( ref_inc_o( o ) );
                }
                os_push(to_decomp.data->_class);
            }
            break;
#endif
            // string ----------------------------------------------------------------

        case opcode_sconst_bin:
            LISTI("sconst bin");
            os_push(pvm_code_get_string(&(da->code)));
            break;


            // flow ------------------------------------------------------------------

        case opcode_jmp:
            LISTIA("jmp %d", da->code.IP);
            da->code.IP = pvm_code_get_rel_IP_as_abs(&(da->code));
            break;


        case opcode_djnz:
            {
                int new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                //is_top()--;
                is_push( is_pop() - 1 );
                if( is_top() ) da->code.IP = new_IP;

                LISTIA("djnz (%d)", is_top() );
                LISTIA("djnz -> %d", new_IP );
            }
            break;

        case opcode_jz:
            {
                int new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                int test = is_pop();
                if( !test ) da->code.IP = new_IP;

                LISTIA("jz (%d)", test );
                LISTIA("jz -> %d",  new_IP );
            }
            break;


        case opcode_switch:
            {
                unsigned int tabsize    = pvm_code_get_int32(&(da->code));
                int shift               = pvm_code_get_int32(&(da->code));
                unsigned int divisor    = pvm_code_get_int32(&(da->code));
                int stack_top = is_pop();

                //LISTIA("switch (%d+%d)/%d, ", stack_top, shift, divisor );
                LISTIA("switch (%d)", stack_top );


                unsigned int start_table_IP = da->code.IP;
                unsigned int displ = (stack_top+shift)/divisor;
                unsigned int new_IP = start_table_IP+(tabsize*4); // default

                //LISTIA("displ %d, etab addr %d ", displ, new_IP );


                if( displ < tabsize )
                {
                    da->code.IP = start_table_IP+(displ*4); // TODO BUG! 4!
                    LISTIA("load from %d, ", da->code.IP );
                    new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                }
                da->code.IP = new_IP;

                //LISTIA("switch(%d) ->%d", displ, new_IP );
                LISTIA("switch ->%d", new_IP );
            }
            break;


        case opcode_ret:
            {
                if( DEB_CALLRET || debug_print_instr ) printf( "\nret     (stack_depth %d -> ", da->stack_depth );
                struct pvm_object ret = pvm_object_da( da->call_frame, call_frame )->prev;

                if( pvm_is_null( ret ) )
                {
                    if( DEB_CALLRET || debug_print_instr ) printf( "exit thread)\n");
                    return;  // exit thread
                }
                pvm_exec_do_return(da);
                if( DEB_CALLRET || debug_print_instr ) printf( "%d)", da->stack_depth );
            }
            break;

            // exceptions are like ret ---------------------------------------------------

        case opcode_throw:
            if( DEB_CALLRET || debug_print_instr ) printf( "\nthrow     (stack_depth %d -> ", da->stack_depth );
            pvm_exec_do_throw(da);
            if( DEB_CALLRET || debug_print_instr ) printf( "%d)", da->stack_depth );
            break;

        case opcode_push_catcher:
            {
                unsigned addr = pvm_code_get_rel_IP_as_abs(&(da->code));
                LISTIA("push catcher %u", addr );
                //cf->push_catcher( addr, os_pop() );
                //call_frame.estack().push(exception_handler(os_pop(),addr));

                struct pvm_exception_handler eh;
                eh.object = os_pop();
                eh.jump = addr;

                //ref_inc_o( eh.object );


                es_push( eh );
            }
            break;

        case opcode_pop_catcher:
            LISTI("pop catcher");
            //cf->pop_catcher();
            //call_frame.estack().pop();
            ref_dec_o( es_pop().object );
            break;

            // ok, now method calls ------------------------------------------------------

            // these 4 are parameter-less calls!
        case opcode_short_call_0:           pvm_exec_call(da,0,0,1,pvm_get_null_object());   break;
        case opcode_short_call_1:           pvm_exec_call(da,1,0,1,pvm_get_null_object());   break;
        case opcode_short_call_2:           pvm_exec_call(da,2,0,1,pvm_get_null_object());   break;
        case opcode_short_call_3:           pvm_exec_call(da,3,0,1,pvm_get_null_object());   break;

        case opcode_call_8bit:
            {
                unsigned int method_index = pvm_code_get_byte(&(da->code));
                unsigned int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param,1,pvm_get_null_object());
            }
            break;
        case opcode_call_32bit:
            {
                unsigned int method_index = pvm_code_get_int32(&(da->code));
                unsigned int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param,1,pvm_get_null_object());
            }
            break;


        case opcode_dynamic_invoke:
            {
                dynamic_method_info_t mi;

                mi.method_name = os_pop();
                mi.new_this = os_pop();
                /*
                pvm_object_t np = os_pop();
                if( !IS_PHANTOM_INT(np) )
                {
                    printf("dyn call n_param not int: " );
                    pvm_object_dump( np );
                    mi.n_param = 0;
                }
                else
                    mi.n_param = pvm_get_int( np );
                ref_dec_o( np );
                */
                mi.n_param = is_pop();
#if DEB_DYNCALL
                printf("dyn call %d param, name = '", mi.n_param );
                //pvm_object_dump( mi.method_name );
                pvm_puts( mi.method_name );
                printf("', this class = " );
                //pvm_object_dump( mi.new_this );
                pvm_object_t cn = pvm_get_class_name( mi.new_this );
                pvm_puts( cn );
                ref_dec_o( cn );
                printf("\n" );
#endif // DEB_DYNCALL

                if( find_dynamic_method( &mi ) )
                    pvm_exec_panic("dynamic invoke failed");

                pvm_exec_call(da,mi.method_ordinal,mi.n_param,1,mi.new_this);
            }
            break;

            // object stack --------------------------------------------------------------

        case opcode_os_dup:
            LISTI("os dup");
            {
                pvm_object_t o = os_top();
                os_push( ref_inc_o( o ) );
            }
            break;

        case opcode_os_drop:
            LISTI("os drop");
            ref_dec_o( os_pop() );
            break;

        case opcode_os_pull32:
            LISTI("os pull");
            {
                pvm_object_t o = os_pull(pvm_code_get_int32(&(da->code)));
                os_push( ref_inc_o( o ) );
            }
            break;

        case opcode_os_load8:       pvm_exec_load(da, pvm_code_get_byte(&(da->code)));	break;
        case opcode_os_load32:      pvm_exec_load(da, pvm_code_get_int32(&(da->code)));	break;

        case opcode_os_save8:       pvm_exec_save(da, pvm_code_get_byte(&(da->code)));	break;
        case opcode_os_save32:      pvm_exec_save(da, pvm_code_get_int32(&(da->code)));	break;

        case opcode_is_load8:       pvm_exec_iload(da, pvm_code_get_byte(&(da->code)));	break;
        case opcode_is_save8:       pvm_exec_isave(da, pvm_code_get_byte(&(da->code)));	break;

        case opcode_os_get32:        pvm_exec_get(da, pvm_code_get_int32(&(da->code)));	break;
        case opcode_os_set32:        pvm_exec_set(da, pvm_code_get_int32(&(da->code)));	break;

        case opcode_is_get32:        pvm_exec_iget(da, pvm_code_get_int32(&(da->code)));	break;
        case opcode_is_set32:        pvm_exec_iset(da, pvm_code_get_int32(&(da->code)));	break;

        default:
            if( (instruction & 0xF0 ) == opcode_sys_0 )
            {
                pvm_exec_sys(da,instruction & 0x0F);
    sys_sleep:
#if OLD_VM_SLEEP
                // Only sys can put thread asleep
                // If we are snapped here we, possibly, will continue from
                // the entry to this func. So save fast acc and recheck
                // sleep condition on the func entry.
                if(da->sleep_flag)
                {
                    pvm_exec_save_fast_acc(da); // Before snap
                    phantom_thread_sleep_worker( da );
                }
#endif
                break;
            }

            if( instruction  == opcode_sys_8bit )
            {
                pvm_exec_sys(da,pvm_code_get_byte(&(da->code))); //cf->cs.get_byte( cf->IP ));
                goto sys_sleep;
                //break;
            }

            if( (instruction & 0xE0 ) == opcode_call_00 )
            {
                unsigned n_param = pvm_code_get_byte(&(da->code));
                pvm_exec_call(da,instruction & 0x1F,n_param,0,pvm_get_null_object()); //no optimization for soon return
                break;
            }

            printf("Unknown op code 0x%X\n", instruction );
            pvm_exec_panic( "thread exec: unknown opcode" ); //, instruction );

        } // outer switch(instruction)

        // NB! We have continue after instructions that set prefix (long/float/double) so that
        // control does not reach here and we do not reset prefix and do not print warning

    noops:
        // TODO can't happen
        if( prefix_long || prefix_float || prefix_double )
            printf("Unused type prefix on op code 0x%X\n", instruction );

        prefix_long = prefix_float = prefix_double = 0;

    } // while(1)
}

void pvm_exec(pvm_object_t current_thread)
{
#if CONF_USE_E4C
    volatile int status = 0;
    //e4c_context_begin( 0 );
    e4c_reusing_context(status, -1)
    {

    E4C_TRY {
#endif // CONF_USE_E4C

    do_pvm_exec(current_thread);

#if CONF_USE_E4C
    } E4C_CATCH(PvmException) {
        printf("got exception: ");
        const e4c_exception *e = e4c_get_exception();
        e4c_print_exception(e);
    } E4C_FINALLY {
    }

    } // reusing context
    ///e4c_context_end();

    (void) status;
#endif // CONF_USE_E4C
}






static syscall_func_t pvm_exec_find_syscall( struct pvm_object _class, unsigned int syscall_index )
{
    if(!(_class.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS))
        pvm_exec_panic( "pvm_exec_find_syscall: not a class object" );

    struct data_area_4_class *da = 0;

    // TODO make sure compiler does not generate such calls and return panic
#if 0
        if( da->object_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL )
            pvm_exec_panic("find_syscall: not internal class in SYS" );
#else
    // Find parent which is internal. This can happen if class got a method from
    // internal parent, such as .internal.object. Happens often with constructor.
    int i = 1024; // max parent levels
    while( i-- > 0 )
    {
        da = (struct data_area_4_class *)&(_class.data->da);

        if( da->object_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERNAL )
            break;

        // we do that only for 0 = construct, that's a hack, must be gone too )
        if( syscall_index != 0 )
            pvm_exec_panic("find_syscall: not internal class in SYS > 0" );

        if( pvm_is_null( da->class_parent ) )
            pvm_exec_panic("find_syscall: not internal class and no internal parent" );

            _class = da->class_parent;
    }

    if( pvm_is_null( da->class_parent ) )
            pvm_exec_panic("find_syscall: not internal class and no internal parent" );
#endif

    if( da->sys_table_id >= pvm_n_internal_classes )
        pvm_exec_panic("find_syscall: internal class index out of table" );
    //pvm_exec_panic("find_syscall: internal class index (%d) out of table (%d)", da->sys_table_id, pvm_n_internal_classes );

    // TODO fix this back
    if( syscall_index >= *pvm_internal_classes[da->sys_table_id].syscalls_table_size_ptr ) pvm_exec_panic("find_syscall: syscall_index no out of table size" );

    syscall_func_t *tab = pvm_internal_classes[da->sys_table_id].syscalls_table;
    return tab[syscall_index];
}

/*
 *
 * Returns code object
 *
 */

struct pvm_object_storage * pvm_exec_find_method( struct pvm_object o, unsigned int method_index )
{
    if( o.data == 0 )
    {
        pvm_exec_panic( "pvm_exec_find_method: null object!" );
    }

    struct pvm_object_storage *iface = o.interface;
    if( iface == 0 )
    {
    	if( o.data->_class.data == 0 )
    	{
            //dumpo(o.data);
            pvm_exec_panic( "pvm_exec_find_method: no interface and no class!" );
    	}
        iface = pvm_object_da( o.data->_class, class )->object_default_interface.data;
    }

    if( iface == 0 )
        pvm_exec_panic( "pvm_exec_find_method: no interface found" );

    if(!(iface->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE))
        pvm_exec_panic( "pvm_exec_find_method: not an interface object" );

    if(method_index > da_po_limit(iface))
        pvm_exec_panic( "pvm_exec_find_method: method index is out of bounds" );

    return da_po_ptr(iface->da)[method_index].data;
}



static int catch_comparator( void *backptr, struct pvm_exception_handler *test )
{
    struct pvm_exception_handler *thrown = (struct pvm_exception_handler *)backptr;

//printf(" (exc cls cmp) ");
    if( pvm_object_class_is_or_child( thrown->object, test->object) )
    {
        thrown->jump = test->jump;
        return 1;
    }
    return 0;
}

static int pvm_exec_find_catch(
                               struct data_area_4_exception_stack* stack,
                               unsigned int *jump_to,
                               struct pvm_object thrown_obj )
{
    struct pvm_exception_handler topass;
    topass.object = thrown_obj;
    topass.jump = 0;

//printf(" (exc lookup catch) ");
    if( pvm_estack_foreach( stack, &topass, &catch_comparator ) )
    {
        *jump_to = topass.jump;
        return 1;
    }

    return 0;
}



/**
 *
 * Compose object
 *
**/

//pvm_object
//pvm_exec_compose_object( pvm_object in_class, pow_ostack in_stack, int to_pop )
#if 0
static struct pvm_object pvm_exec_compose_object(
                                                 struct pvm_object in_class,
                                                 struct data_area_4_object_stack *in_stack,
                                                 int to_pop
                                                )
{
    //struct pvm_object out = pvm_object_create_fixed( in_class ); // This one does not initialize internal ones
    struct pvm_object out = pvm_create_object( in_class ); // This one does initialize internal ones

    //struct pvm_object_storage	* _data = out.data;

    //struct data_area_4_class *cda = (struct data_area_4_class *)in_class.data->da;

    int das = out.data->_da_size;

    struct pvm_object * data_area = (struct pvm_object *)out.data->da;

    int max = das/sizeof(struct pvm_object);

    if( to_pop > max ) pvm_exec_panic("compose: cant compose so many fields");

    int i;
    for( i = 0; i < to_pop; i++ )
    {
        data_area[i] = pvm_ostack_pop( in_stack );
    }

    for( ; i < max; i++ )
        data_area[i] = pvm_get_null_object(); // null

    /*
    struct pvm_object out;
    out.data = _data;


    out.interface = cda->object_default_interface;
    */
    return out;
}
#endif

// Todo it's a call_frame method!
void pvm_exec_set_cs( struct data_area_4_call_frame* cfda, struct pvm_object_storage * code )
{
    struct data_area_4_code *cda = (struct data_area_4_code *)code->da;

    cfda->IP = 0;
    cfda->IP_max = cda->code_size;
    cfda->code = cda->code;
    // TODO set ref from cfda to code for gc to make sure code wont be collected while running
    //pvm_object_t co;
    //co.data = code;
    //co.interface = 0;
}


// TODO: implement!
struct pvm_object pvm_exec_lookup_class_by_name(struct pvm_object name)
{
    // Try internal
    struct pvm_object ret = pvm_lookup_internal_class(name);
    if( !pvm_is_null(ret) )
        return ret;

    /*
     *
     * TODO BUG! This executes code in a tight loop. It will prevent
     * snaps from being done. Redo with a separate thread start.
     *
     * Run class loader in the main pvm_exec() loop? Hmmm.
     *
     */
//printf("lookup_class_by_name\n");


    if( pvm_is_null(pvm_root.class_loader) )
        return pvm_create_null_object();

    // Try userland loader
    struct pvm_object args[1] = { name };
    return pvm_exec_run_method( pvm_root.class_loader, 8, 1, args );
}


/*
 *
 * TODO BUG! This executes code in a tight loop. It will prevent
 * snaps from being done. Redo with a separate thread start.
 *
 */

struct pvm_object
pvm_exec_run_method(
                    struct pvm_object this_object,
                    int method,
                    int n_args,
                    struct pvm_object args[]
                   )
{
    struct pvm_object new_cf = pvm_create_call_frame_object();
    struct data_area_4_call_frame* cfda = pvm_object_da( new_cf, call_frame );

    int i;
    for( i = n_args; i > 0; i-- )
    {
        pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), ref_inc_o( args[i-1] ) );
    }

    pvm_istack_push( pvm_object_da(cfda->istack, integer_stack), n_args); // pass him real number of parameters

    struct pvm_object_storage *code = pvm_exec_find_method( this_object, method );
    pvm_exec_set_cs( cfda, code );
    cfda->this_object = ref_inc_o( this_object );

    pvm_object_t thread = pvm_create_thread_object( new_cf );

    pvm_exec( thread );

    pvm_object_t ret = pvm_ostack_pop( pvm_object_da(cfda->ostack, object_stack) );

    pvm_release_thread_object( thread );

    return ret;
}

// Find a method for a dynamic invoke
static errno_t find_dynamic_method( dynamic_method_info_t *mi )
{
    //int is_global = 0;
    if( pvm_is_null( mi->new_this ) )
    {
        //is_global = 0;
        mi->target_class = pvm_null;
    }
    else
        mi->target_class = pvm_get_class( mi->new_this );

    // no public classless funcs
    if( pvm_is_null( mi->target_class ) )
        return ENOENT;


    int ord = pvm_get_method_ordinal( mi->target_class, mi->method_name );
    if( ord < 0 )
    {
        printf("dyn method not found '");
        pvm_object_print(mi->method_name);
        printf("'\n");
        return ENOENT;
    }

    mi->method_ordinal = ord;
    return 0;
}



