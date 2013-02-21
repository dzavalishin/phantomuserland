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

#include <vm/stacks.h>
#include <vm/syscall.h>

#include "ids/opcode_ids.h"

#include <kernel/snap_sync.h>

//#define printf printf


/*
 * NB! Refcount contract:
 *        all push/pop/get/set operations with objects just copy pointers and do not change refcount
 *        - someone who consumes the arg must!
 */


#define DEB_CALLRET 0

//static int debug_print_instr = 1;
int debug_print_instr = 0;


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
    if( debug_print_instr ) printf("os load %d; ", slot);
    struct pvm_object o = pvm_get_ofield( this_object(), slot);
    os_push( ref_inc_o (o) );
}

static void pvm_exec_save( struct data_area_4_thread *da, unsigned slot )
{
    if( debug_print_instr ) printf("os save %d; ", slot);
    pvm_set_ofield( this_object(), slot, os_pop() );
}


static void pvm_exec_iload( struct data_area_4_thread *da, unsigned slot )
{
    if( debug_print_instr ) printf("is load %d; ", slot);
    int v = pvm_get_int( pvm_get_ofield( this_object(), slot) );
    is_push( v );
}

static void pvm_exec_isave( struct data_area_4_thread *da, unsigned slot )
{
    if( debug_print_instr ) printf("is save %d; ", slot);
    pvm_set_ofield( this_object(), slot, pvm_create_int_object(is_pop()) );
}


static void pvm_exec_get( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) printf("os stack get %d; ", abs_stack_pos);
    pvm_object_t o = pvm_ostack_abs_get(da->_ostack, abs_stack_pos);
    os_push( ref_inc_o(o) );
}

static void pvm_exec_set( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) printf("os stack set %d; ", abs_stack_pos);
    pvm_ostack_abs_set( da->_ostack, abs_stack_pos, os_pop() );
}


static void pvm_exec_iget( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) printf("is stack get %d; ", abs_stack_pos);
    is_push( pvm_istack_abs_get(da->_istack, abs_stack_pos) );
}

static void pvm_exec_iset( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) printf("is stack set %d; ", abs_stack_pos);
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
        if( debug_print_instr ) printf("except does unwind, ");
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
    if( debug_print_instr ) printf("except jump to %d; ", jump_to);
    da->code.IP = jump_to;
    os_push(thrown_obj);
}


static syscall_func_t pvm_exec_find_syscall( struct pvm_object _class, unsigned int syscall_index );


// syscalss numbers are specific to object class
static void pvm_exec_sys( struct data_area_4_thread *da, unsigned int syscall_index )
{
    if( debug_print_instr ) printf("sys %d start; ", syscall_index );

    //n_args = is_top();

    // which object's syscall we'll call
    struct pvm_object o = this_object();

    syscall_func_t func = pvm_exec_find_syscall( o.data->_class, syscall_index );

    if( func == 0 )
    {
        if( debug_print_instr ) printf("sys %d invalid! ", syscall_index );
    }
    else
    {
        // TODO dec refcount for all the args after call?
        // OR! in syscall arg pop?
        // OR! in syscall code?
        if( func( o, da ) == 0 )	// exec syscall
            pvm_exec_do_throw(da); // exception reported
    }

    if( debug_print_instr ) printf("sys %d end; ", syscall_index );
}



static void init_cfda(struct data_area_4_thread *da, struct data_area_4_call_frame *cfda, unsigned int method_index, unsigned int n_param )
{
	cfda->ordinal = method_index;
    // which object's method we'll call - pop after args!

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

    struct pvm_object o = os_pop();

    struct pvm_object_storage *code = pvm_exec_find_method( o, method_index );
    assert(code != 0);
    pvm_exec_set_cs( cfda, code );
    cfda->this_object = o;
}


static void pvm_exec_call( struct data_area_4_thread *da, unsigned int method_index, unsigned int n_param, int do_optimize_ret )
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

    init_cfda(da, cfda, method_index, n_param);

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

void pvm_exec(pvm_object_t current_thread)
{
    int prefix_long = 0;
    int prefix_float = 0;
    int prefix_double = 0;

#define DO_TWICE  (prefix_long || prefix_double)
#define DO_FPOINT (prefix_float || prefix_double)

    if( !pvm_object_class_is( current_thread, pvm_get_thread_class() ))
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
        touch_snap_catch();
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
            switch(instruction)
            {
            case opcode_isum:
                if( debug_print_instr ) printf("l-isum; ");
                {
                    int64_t add = ls_pop();
                    ls_push( ls_pop() + add );
                }
                break;

            case opcode_imul:
                if( debug_print_instr ) printf("l-imul; ");
                {
                    int64_t mul = ls_pop();
                    ls_push( ls_pop() * mul );
                }
                break;

            case opcode_isubul:
                if( debug_print_instr ) printf("l-isubul; ");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(u-l);
                }
                break;

            case opcode_isublu:
                if( debug_print_instr ) printf("l-isublu; ");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(l-u);
                }
                break;

            case opcode_idivul:
                if( debug_print_instr ) printf("l-idivul; ");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(u/l);
                }
                break;

            case opcode_idivlu:
                if( debug_print_instr ) printf("l-idivlu; ");
                {
                    int64_t u = ls_pop();
                    int64_t l = ls_pop();
                    ls_push(l/u);
                }
                break;

            case opcode_ior:
                if( debug_print_instr ) printf("l-ior; ");
                { int64_t operand = ls_pop();	ls_push( ls_pop() | operand ); }
                break;

            case opcode_iand:
                if( debug_print_instr ) printf("l-iand; ");
                { int64_t operand = ls_pop();	ls_push( ls_pop() & operand ); }
                break;

            case opcode_ixor:
                if( debug_print_instr ) printf("l-ixor; ");
                { int64_t operand = ls_pop();	ls_push( ls_pop() ^ operand ); }
                break;

            case opcode_inot:
                if( debug_print_instr ) printf("l-inot; ");
                { int64_t operand = ls_pop();	ls_push( ~operand ); }
                break;



            case opcode_log_or:
                if( debug_print_instr ) printf("l-lor; ");
                {
                    int64_t o1 = ls_pop();
                    int64_t o2 = ls_pop();
                    ls_push( o1 || o2 );
                }
                break;

            case opcode_log_and:
                if( debug_print_instr ) printf("l-land; ");
                {
                    int64_t o1 = ls_pop();
                    int64_t o2 = ls_pop();
                    ls_push( o1 && o2 );
                }
                break;

            case opcode_log_xor:
                if( debug_print_instr ) printf("l-lxor; ");
                {
                    int64_t o1 = ls_pop() ? 1 : 0;
                    int64_t o2 = ls_pop() ? 1 : 0;
                    ls_push( o1 ^ o2 );
                }
                break;

            case opcode_log_not:
                if( debug_print_instr ) printf("l-lnot; ");
                {
                    int64_t operand = ls_pop();
                    ls_push( !operand );
                }
                break;

                // NB! Returns int!
            case opcode_ige:	// >=
                if( debug_print_instr ) printf("l-ige; ");
                { int64_t operand = ls_pop();	is_push( ls_pop() >= operand ); }
                break;
            case opcode_ile:	// <=
                if( debug_print_instr ) printf("l-ile; ");
                { int64_t operand = ls_pop();	is_push( ls_pop() <= operand ); }
                break;
            case opcode_igt:	// >
                if( debug_print_instr ) printf("l-igt; ");
                { int64_t operand = ls_pop();	is_push( ls_pop() > operand ); }
                break;
            case opcode_ilt:	// <
                if( debug_print_instr ) printf("l-ilt; ");
                { int64_t operand = ls_pop();	is_push( ls_pop() < operand ); }
                break;



            case opcode_i2o:
                if( debug_print_instr ) printf("l-i2o; ");
                os_push(pvm_create_long_object(ls_pop()));
                break;

            case opcode_o2i:
                if( debug_print_instr ) printf("l-o2i; ");
                {
                    struct pvm_object o = os_pop();
                    if( o.data == 0 ) pvm_exec_panic("l-o2i(null)");
                    ls_push( pvm_get_long( o ) );
                    ref_dec_o(o);
                }
                break;
            }
        // End of long ops

        if( prefix_float )
            switch(instruction)
            {
            }
        // End of float ops

        if( prefix_double )
            switch(instruction)
            {
            }
        // End of double ops




        switch(instruction)
        {
        case opcode_nop:
            if( debug_print_instr ) printf("nop; ");
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

            // type switch prefixes --------------------------------

        case opcode_prefix_long:   prefix_long   = 1; break;
        case opcode_prefix_float:  prefix_float  = 1; break;
        case opcode_prefix_double: prefix_double = 1; break;

            // int stack ops ---------------------------------------

        case opcode_is_dup:
            if( debug_print_instr ) printf("is dup; ");
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
            if( debug_print_instr ) printf("is drop; ");
            is_pop(); if(DO_TWICE) is_pop();
            break;

        case opcode_iconst_0:
            if( debug_print_instr ) printf("iconst 0; ");
            is_push(0); if(DO_TWICE) is_push(0);
            break;

        case opcode_iconst_1:
            if( debug_print_instr ) printf("iconst 1; ");
            is_push(1); if(DO_TWICE) is_push(1);
            break;

        case opcode_iconst_8bit:
            {
                int v = pvm_code_get_byte(&(da->code));
                if(DO_TWICE) ls_push(v);
                else is_push(v);
                if( debug_print_instr ) printf("iconst8 = %d; ", v);
                break;
            }

        case opcode_iconst_32bit:
            {
                int v = pvm_code_get_int32(&(da->code));
                if(DO_TWICE) ls_push(v);
                else is_push(v);
                if( debug_print_instr ) printf("iconst32 = %d; ", v);
                break;
            }


        case opcode_isum:
            if( debug_print_instr ) printf("isum; ");
            {
                int add = is_pop();
                //is_top() += add;
                is_push( is_pop() + add );
            }
            break;

        case opcode_imul:
            if( debug_print_instr ) printf("imul; ");
            {
                int mul = is_pop();
                //is_top() *= mul;
                is_push( is_pop() * mul );
            }
            break;

        case opcode_isubul:
            if( debug_print_instr ) printf("isubul; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(u-l);
            }
            break;

        case opcode_isublu:
            if( debug_print_instr ) printf("isublu; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(l-u);
            }
            break;

        case opcode_idivul:
            if( debug_print_instr ) printf("idivul; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(u/l);
            }
            break;

        case opcode_idivlu:
            if( debug_print_instr ) printf("idivlu; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(l/u);
            }
            break;

        case opcode_ior:
            if( debug_print_instr ) printf("ior; ");
            { int operand = is_pop();	is_push( is_pop() | operand ); }
            break;

        case opcode_iand:
            if( debug_print_instr ) printf("iand; ");
            { int operand = is_pop();	is_push( is_pop() & operand ); }
            break;

        case opcode_ixor:
            if( debug_print_instr ) printf("ixor; ");
            { int operand = is_pop();	is_push( is_pop() ^ operand ); }
            break;

        case opcode_inot:
            if( debug_print_instr ) printf("inot; ");
            { int operand = is_pop();	is_push( ~operand ); }
            break;



        case opcode_log_or:
            if( debug_print_instr ) printf("lor; ");
            {
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 || o2 );
            }
            break;

        case opcode_log_and:
            if( debug_print_instr ) printf("land; ");
            {
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 && o2 );
            }
            break;

        case opcode_log_xor:
            if( debug_print_instr ) printf("lxor; ");
            {
                int o1 = is_pop() ? 1 : 0;
                int o2 = is_pop() ? 1 : 0;
                is_push( o1 ^ o2 );
            }
            break;

        case opcode_log_not:
            if( debug_print_instr ) printf("lnot; ");
            {
                int operand = is_pop();
                is_push( !operand );
            }
            break;


        case opcode_ige:	// >=
            if( debug_print_instr ) printf("ige; ");
            { int operand = is_pop();	is_push( is_pop() >= operand ); }
            break;
        case opcode_ile:	// <=
            if( debug_print_instr ) printf("ile; ");
            { int operand = is_pop();	is_push( is_pop() <= operand ); }
            break;
        case opcode_igt:	// >
            if( debug_print_instr ) printf("igt; ");
            { int operand = is_pop();	is_push( is_pop() > operand ); }
            break;
        case opcode_ilt:	// <
            if( debug_print_instr ) printf("ilt; ");
            { int operand = is_pop();	is_push( is_pop() < operand ); }
            break;



        case opcode_i2o:
            if( debug_print_instr ) printf("i2o; ");
            os_push(pvm_create_int_object(is_pop()));
            break;

        case opcode_o2i:
            if( debug_print_instr ) printf("o2i; ");
            {
                struct pvm_object o = os_pop();
                if( o.data == 0 ) pvm_exec_panic("o2i(null)");
                is_push( pvm_get_int( o ) );
                ref_dec_o(o);
            }
            break;


        case opcode_os_eq:
            if( debug_print_instr ) printf("os eq; ");
            {
                struct pvm_object o1 = os_pop();
                struct pvm_object o2 = os_pop();
                is_push( o1.data == o2.data );
                ref_dec_o(o1);
                ref_dec_o(o2);
                break;
            }

        case opcode_os_neq:
            if( debug_print_instr ) printf("os neq; ");
            {
                struct pvm_object o1 = os_pop();
                struct pvm_object o2 = os_pop();
                is_push( o1.data != o2.data );
                ref_dec_o(o1);
                ref_dec_o(o2);
                break;
            }

        case opcode_os_isnull:
            if( debug_print_instr ) printf("isnull; ");
            {
                struct pvm_object o1 = os_pop();
                is_push( pvm_is_null( o1 ) );
                ref_dec_o(o1);
                break;
            }

        case opcode_os_push_null:
            if( debug_print_instr ) printf("push null; ");
            {
                os_push( pvm_get_null_object() );
                break;
            }


            // summoning, special ----------------------------------------------------

        case opcode_summon_null:
            if( debug_print_instr ) printf("push null; ");
            os_push( pvm_get_null_object() ); // so what opcode_os_push_null is for then?
            break;

        case opcode_summon_thread:
            if( debug_print_instr ) printf("summon thread; ");
            os_push( ref_inc_o( current_thread ) );
            //printf("ERROR: summon thread; ");
            break;

        case opcode_summon_this:
            if( debug_print_instr ) printf("summon this; ");
            os_push( ref_inc_o( this_object() ) );
            break;

        case opcode_summon_class_class:
            if( debug_print_instr ) printf("summon class class; ");
            // it has locked refcount
            os_push( pvm_get_class_class() );
            break;

        case opcode_summon_interface_class:
            if( debug_print_instr ) printf("summon interface class; ");
            // locked refcnt
            os_push( pvm_get_interface_class() );
            break;

        case opcode_summon_code_class:
            if( debug_print_instr ) printf("summon code class; ");
        	// locked refcnt
            os_push( pvm_get_code_class() );
            break;

        case opcode_summon_int_class:
            if( debug_print_instr ) printf("summon int class; ");
        	// locked refcnt
            os_push( pvm_get_int_class() );
            break;

        case opcode_summon_string_class:
            if( debug_print_instr ) printf("summon string class; ");
        	// locked refcnt
            os_push( pvm_get_string_class() );
            break;

        case opcode_summon_array_class:
            if( debug_print_instr ) printf("summon array class; ");
        	// locked refcnt
            os_push( pvm_get_array_class() );
            break;

        case opcode_summon_by_name:
            {
                if( debug_print_instr ) printf("summon by name; ");
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
            if( debug_print_instr ) printf("new; ");
            {
                pvm_object_t cl = os_pop();
                os_push( pvm_create_object( cl ) );
                //ref_dec_o( cl );  // object keep class ref
            }
            break;

        case opcode_copy:
            if( debug_print_instr ) printf("copy; ");
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
            if( debug_print_instr ) printf(" compose32; ");
            {
                int num = pvm_code_get_int32(&(da->code));
                struct pvm_object in_class = os_pop();
                os_push( pvm_exec_compose_object( in_class, da->_ostack, num ) );
            }
            break;

        case opcode_os_decompose:
            if( debug_print_instr ) printf(" decompose; ");
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
            if( debug_print_instr ) printf("sconst bin; ");
            os_push(pvm_code_get_string(&(da->code)));
            break;


            // flow ------------------------------------------------------------------

        case opcode_jmp:
            if( debug_print_instr ) printf("jmp %d; ", da->code.IP);
            da->code.IP = pvm_code_get_rel_IP_as_abs(&(da->code));
            break;


        case opcode_djnz:
            {
                if( debug_print_instr ) printf("djnz " );
                int new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                //is_top()--;
                is_push( is_pop() - 1 );
                if( is_top() ) da->code.IP = new_IP;

                if( debug_print_instr ) printf("(%d) -> %d; ", is_top() , new_IP );
            }
            break;

        case opcode_jz:
            {
                if( debug_print_instr ) printf("jz " );
                int new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                int test = is_pop();
                if( !test ) da->code.IP = new_IP;

                if( debug_print_instr ) printf("(%d) -> %d; ", test , new_IP );
            }
            break;


        case opcode_switch:
            {
                if( debug_print_instr ) printf("switch ");
                unsigned int tabsize    = pvm_code_get_int32(&(da->code));
                int shift               = pvm_code_get_int32(&(da->code));
                unsigned int divisor    = pvm_code_get_int32(&(da->code));
                int stack_top = is_pop();

                if( debug_print_instr ) printf("(%d+%d)/%d, ", stack_top, shift, divisor );


                unsigned int start_table_IP = da->code.IP;
                unsigned int displ = (stack_top+shift)/divisor;
                unsigned int new_IP = start_table_IP+(tabsize*4); // default

                if( debug_print_instr ) printf("displ %d, etab addr %d ", displ, new_IP );


                if( displ < tabsize )
                {
                    da->code.IP = start_table_IP+(displ*4); // TODO BUG! 4!
                    if( debug_print_instr ) printf("load from %d, ", da->code.IP );
                    new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                }
                da->code.IP = new_IP;

                if( debug_print_instr ) printf("switch(%d) ->%d; ", displ, new_IP );
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
                if( DEB_CALLRET || debug_print_instr ) printf( "%d); ", da->stack_depth );
            }
            break;

            // exceptions are like ret ---------------------------------------------------

        case opcode_throw:
            if( DEB_CALLRET || debug_print_instr ) printf( "\nthrow     (stack_depth %d -> ", da->stack_depth );
            pvm_exec_do_throw(da);
            if( DEB_CALLRET || debug_print_instr ) printf( "%d); ", da->stack_depth );
            break;

        case opcode_push_catcher:
            {
                unsigned addr = pvm_code_get_rel_IP_as_abs(&(da->code));
                if( debug_print_instr ) printf("push catcher %u; ", addr );
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
            if( debug_print_instr ) printf("pop catcher; ");
            //cf->pop_catcher();
            //call_frame.estack().pop();
            ref_dec_o( es_pop().object );
            break;

            // ok, now method calls ------------------------------------------------------

            // these 4 are parameter-less calls!
        case opcode_short_call_0:           pvm_exec_call(da,0,0,1);   break;
        case opcode_short_call_1:           pvm_exec_call(da,1,0,1);   break;
        case opcode_short_call_2:           pvm_exec_call(da,2,0,1);   break;
        case opcode_short_call_3:           pvm_exec_call(da,3,0,1);   break;

        case opcode_call_8bit:
            {
                unsigned int method_index = pvm_code_get_byte(&(da->code));
                unsigned int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param,1);
            }
            break;
        case opcode_call_32bit:
            {
                unsigned int method_index = pvm_code_get_int32(&(da->code));
                unsigned int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param,1);
            }
            break;


            // object stack --------------------------------------------------------------

        case opcode_os_dup:
            if( debug_print_instr ) printf("os dup; ");
            {
                pvm_object_t o = os_top();
                os_push( ref_inc_o( o ) );
            }
            break;

        case opcode_os_drop:
            if( debug_print_instr ) printf("os drop; ");
            ref_dec_o( os_pop() );
            break;

        case opcode_os_pull32:
            if( debug_print_instr ) printf("os pull; ");
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
                pvm_exec_call(da,instruction & 0x1F,n_param,0); //no optimization for soon return
                break;
            }

            printf("Unknown op code 0x%X\n", instruction );
            pvm_exec_panic( "thread exec: unknown opcode" ); //, instruction );
            //exit(33);
        }

        if( prefix_long || prefix_float || prefix_double )
            printf("Unused type prefix on op code 0x%X\n", instruction );

        prefix_long   = 0;
        prefix_float  = 0;
        prefix_double = 0;

    }
}




static syscall_func_t pvm_exec_find_syscall( struct pvm_object _class, unsigned int syscall_index )
{
    if(!(_class.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS))
        pvm_exec_panic( "pvm_exec_find_syscall: not a class object" );

    struct data_area_4_class *da = (struct data_area_4_class *)&(_class.data->da);

    // TODO fix this back
    //if( syscall_index >= pvm_internal_classes[da->sys_table_id].syscalls_table_size ) pvm_exec_panic("find_syscall: syscall_index no out of table size" );

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
    if( pvm_object_class_is( thrown->object, test->object) )
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

