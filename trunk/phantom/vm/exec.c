/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/


#include <phantom_assert.h>

#include "vm/root.h"
#include "vm/internal_da.h"
#include "vm/internal.h"
#include "vm/object_flags.h"
#include "vm/exception.h"
#include "vm/alloc.h"

#include "vm/exec.h"
#include "vm/code.h"

#include "vm/stacks.h"
#include "vm/syscall.h"

#include "ids/opcode_ids.h"

#include <kernel/snap_sync.h>

#define hal_printf printf


#define DEB_CALLRET 0

static int debug_print_instr = 0;


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

#define es_push( i ) 	pvm_estack_push( da->_estack, i )
#define es_pop() 	pvm_estack_pop( da->_estack )


#define this_object()   (da->_this_object)


/**
 *
 * Helpers
 *
**/


/* Poor man's exceptions */
void pvm_exec_throw( const char *reason )
{
    pvm_memcheck();
    // TODO: longjmp?
    panic("pvm_exec_throw: %s", reason );
}


void pvm_exec_load_fast_acc(struct data_area_4_thread *da)
{
    struct data_area_4_call_frame *cf = (struct data_area_4_call_frame *)&(da->call_frame.data->da);

    da->code.IP_max  = cf->IP_max;
    da->code.code    = cf->code;

    da->code.IP      = cf->IP; //da->call_frame.get_IP();

    da->_this_object = cf->this_object; //da->call_frame.get_this();

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
    if( debug_print_instr ) hal_printf("os load %d; ", slot);
    os_push( pvm_get_ofield( this_object(), slot) );
}

static void pvm_exec_save( struct data_area_4_thread *da, unsigned slot )
{
    if( debug_print_instr ) hal_printf("os save %d; ", slot);
    pvm_set_ofield( this_object(), slot, os_pop() );
}


static void pvm_exec_iload( struct data_area_4_thread *da, unsigned slot )
{
    if( debug_print_instr ) hal_printf("is load %d; ", slot);
    // TODO pvm_get_asint must return int and do not change refcount
    pvm_object_t o = pvm_get_ofield( this_object(), slot);
    int v = pvm_get_int( o );
    ref_dec_o(o);
    is_push( v );
}

static void pvm_exec_isave( struct data_area_4_thread *da, unsigned slot )
{
    if( debug_print_instr ) hal_printf("is save %d; ", slot);
    pvm_set_ofield( this_object(), slot, pvm_create_int_object(is_pop()) );
}


static void pvm_exec_get( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) hal_printf("os stack get %d; ", abs_stack_pos);
    pvm_object_t o = pvm_ostack_abs_get(da->_ostack, abs_stack_pos);
    ref_inc_o( o );
    os_push( o );
}

static void pvm_exec_set( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) hal_printf("os stack set %d; ", abs_stack_pos);
    // TODO killing var in stack - dec refcount
    pvm_ostack_abs_set( da->_ostack, abs_stack_pos, os_pop() );
}


static void pvm_exec_iget( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) hal_printf("is stack get %d; ", abs_stack_pos);
    is_push( pvm_istack_abs_get(da->_istack, abs_stack_pos) );
}

static void pvm_exec_iset( struct data_area_4_thread *da, unsigned abs_stack_pos )
{
    if( debug_print_instr ) hal_printf("is stack set %d; ", abs_stack_pos);
    pvm_istack_abs_set( da->_istack, abs_stack_pos, is_pop() );
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
        if( debug_print_instr ) hal_printf("except does unwind, ");
        struct pvm_object ret = pvm_object_da( da->call_frame, call_frame )->prev;
        if( pvm_is_null( ret ) )
        {
            printf("Unhandled exception: ");
            pvm_object_print( thrown_obj );
            printf("\n");
            //getchar();

            pvm_exec_throw( "unwind: nowhere to return" );
        }
        // TODO dec refcounts for stack frame we leave!
        pvm_exec_save_fast_acc(da); // need?
        da->call_frame = ret;
        pvm_exec_load_fast_acc(da);
    }
    if( debug_print_instr ) hal_printf("except jump to %d; ", jump_to);
    da->code.IP = jump_to;
    os_push(thrown_obj);
}


static syscall_func_t pvm_exec_find_syscall( struct pvm_object _class, int syscall_index );


// syscalss numbers are specific to object class
static void pvm_exec_sys( struct data_area_4_thread *da, unsigned syscall_index )
{
    if( debug_print_instr ) hal_printf("sys %d start; ", syscall_index );

    //n_args = is_top();

    // which object's syscall we'll call
    struct pvm_object o = this_object();

    syscall_func_t func = pvm_exec_find_syscall( o.data->_class, syscall_index );

    if( func == 0 )
    {
        if( debug_print_instr ) hal_printf("sys %d invalid! ", syscall_index );
    }
    else
    {
        // TODO dec refcount for all the args after call?
        // OR! in syscall arg pop?
        // OR! in syscall code?
        if( func( o, da ) == 0 )	// exec syscall
            pvm_exec_do_throw(da); // exception reported
    }

    if( debug_print_instr ) hal_printf("sys %d end; ", syscall_index );
}





static void pvm_exec_call( struct data_area_4_thread *da, unsigned method_index, unsigned n_param )
{
    if( DEB_CALLRET || debug_print_instr )
    {
        hal_printf("call %d; ", method_index );
    }
    // which object's method we'll call - pop after args!
    struct pvm_object new_cf = pvm_create_call_frame_object();
    struct data_area_4_call_frame* cfda = pvm_object_da( new_cf, call_frame );

    // allocae places on stack
    {
        int i;
        for( i = n_param; i; i-- )
        {
            pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), pvm_create_null_object() );
        }
    }

    // fill 'em in correct order
    {
        int i;
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


    cfda->prev = da->call_frame;

    pvm_exec_save_fast_acc(da);
    da->call_frame = new_cf;
    pvm_exec_load_fast_acc(da);

    //if( debug_print_instr ) hal_printf("call %d end; ", method_index );
}





//static struct pvm_object pvm_exec_lookup_class( struct data_area_4_thread *thread, struct pvm_object name);


static struct pvm_object pvm_exec_compose_object(
                                                 struct pvm_object class,
                                                 struct data_area_4_object_stack *stack,
                                                 int number
                                                );


/*
 *
 * This must be, possibly, killed, no?
 * At least it can't be used widely. The only known 'fair' use
 * is boot code execution, which must happen only once in system's life.
 *
 * Second known use is call to userland class lookup/load code, which must
 * be redone with a separate thread. Calling thread must then be stopped out
 * of the interpreter to make sure snaps are safe.
 *
void pvm_exec_loop_kludge(struct pvm_object current_thread)
{
    struct data_area_4_thread *da = (struct data_area_4_thread *)&(current_thread.data->da);
    // TODO: check for current_thread to be thread for real

    pvm_exec_load_fast_acc(da); // For any case

    while(1)
    {
    }
}
 */



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

void pvm_exec(struct pvm_object current_thread)
{
    struct data_area_4_thread *da = (struct data_area_4_thread *)&(current_thread.data->da);
    // TODO: check for current_thread to be thread for real

    pvm_exec_load_fast_acc(da); // For any case

    // Thread was snapped sleeping - resleep it
    if(da->sleep_flag)
        phantom_thread_sleep_worker( da );

    while(1)
    {

        if(phantom_virtual_machine_snap_request)
        {
            pvm_exec_save_fast_acc(da); // Before snap
            phantom_thread_wait_4_snap();
            //pvm_exec_load_fast_acc(da); // We don't need this, if we die, we will enter again from above :)
        }

#if GC_ENABLED
        static int gcc = 0;
        gcc++;
        if( gcc > 20000 )
        {
            gcc = 0;
            run_gc();
        }
#endif // GC_ENABLED

        unsigned char instruction = pvm_code_get_byte(&(da->code));
        //hal_printf("instr 0x%02X ", instruction);


        switch(instruction)
        {
        case opcode_nop:
            if( debug_print_instr ) hal_printf("nop; ");
            break;

        case opcode_debug:
            {
                int type = pvm_code_get_byte(&(da->code)); //cf->cs.get_instr( cf->IP );
                hal_printf("\n\nDebug 0x%02X", type );
                if( type & 0x80 )
                {
                    hal_printf(" (" );
                    //cf->cs.get_string( cf->IP ).my_data()->print();
                    //get_string().my_data()->print();
                    pvm_object_print( pvm_code_get_string(&(da->code)) );
                    hal_printf(")" );
                }

                if( type & 0x01 ) debug_print_instr = 1;
                if( type & 0x02 ) debug_print_instr = 0;

                //if( cf->istack.empty() )hal_printf(", istack empty");
                if( is_empty() ) hal_printf(", istack empty");
                else                    hal_printf(",\n\tistack top = %d", is_top() );
                if( os_empty() ) hal_printf(", ostack empty");
                else
                {
                    hal_printf(",\n\tostack top = {" );
                    pvm_object_print( os_top() );
                    hal_printf("}" );
                }
                hal_printf(";\n\n");
            }
            break;


            // int stack ops ---------------------------------------

        case opcode_is_dup:
            if( debug_print_instr ) hal_printf("is dup; ");
            {
                is_push(is_top());
            }
            break;

        case opcode_is_drop:
            if( debug_print_instr ) hal_printf("is drop; ");
            is_pop();
            break;

        case opcode_iconst_0:
            if( debug_print_instr ) hal_printf("iconst 0; ");
            is_push(0);
            break;

        case opcode_iconst_1:
            if( debug_print_instr ) hal_printf("iconst 1; ");
            is_push(1);
            break;

        case opcode_iconst_8bit:
            {
                int v = pvm_code_get_byte(&(da->code));
                is_push(v);
                if( debug_print_instr ) hal_printf("iconst8 = %d; ", v);
                break;
            }

        case opcode_iconst_32bit:
            {
                int v = pvm_code_get_int32(&(da->code));
                is_push(v);
                if( debug_print_instr ) hal_printf("iconst32 = %d; ", v);
                break;
            }


        case opcode_isum:
            if( debug_print_instr ) hal_printf("isum; ");
            {
                int add = is_pop();
                //is_top() += add;
                is_push( is_pop() + add );
            }
            break;

        case opcode_imul:
            if( debug_print_instr ) hal_printf("imul; ");
            {
                int mul = is_pop();
                //is_top() *= mul;
                is_push( is_pop() * mul );
            }
            break;

        case opcode_isubul:
            if( debug_print_instr ) hal_printf("isubul; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(u-l);
            }
            break;

        case opcode_isublu:
            if( debug_print_instr ) hal_printf("isublu; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(l-u);
            }
            break;

        case opcode_idivul:
            if( debug_print_instr ) hal_printf("idivul; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(u/l);
            }
            break;

        case opcode_idivlu:
            if( debug_print_instr ) hal_printf("idivlu; ");
            {
                int u = is_pop();
                int l = is_pop();
                is_push(l/u);
            }
            break;

        case opcode_ior:
            if( debug_print_instr ) hal_printf("ior; ");
            { int operand = is_pop();	is_push( is_pop() | operand ); }
            break;

        case opcode_iand:
            if( debug_print_instr ) hal_printf("iand; ");
            { int operand = is_pop();	is_push( is_pop() & operand ); }
            break;

        case opcode_ixor:
            if( debug_print_instr ) hal_printf("ixor; ");
            { int operand = is_pop();	is_push( is_pop() ^ operand ); }
            break;

        case opcode_inot:
            if( debug_print_instr ) hal_printf("inot; ");
            { int operand = is_pop();	is_push( ~operand ); }
            break;



        case opcode_log_or:
            if( debug_print_instr ) hal_printf("lor; ");
            {
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 || o2 );
            }
            break;

        case opcode_log_and:
            if( debug_print_instr ) hal_printf("land; ");
            {
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 && o2 );
            }
            break;

        case opcode_log_xor:
            if( debug_print_instr ) hal_printf("lxor; ");
            {
                int o1 = is_pop() ? 1 : 0;
                int o2 = is_pop() ? 1 : 0;
                is_push( o1 ^ o2 );
            }
            break;

        case opcode_log_not:
            if( debug_print_instr ) hal_printf("lnot; ");
            {
                int operand = is_pop();
                is_push( !operand );
            }
            break;


        case opcode_ige:	// >=
            if( debug_print_instr ) hal_printf("ige; ");
            { int operand = is_pop();	is_push( is_pop() >= operand ); }
            break;
        case opcode_ile:	// <=
            if( debug_print_instr ) hal_printf("ile; ");
            { int operand = is_pop();	is_push( is_pop() <= operand ); }
            break;
        case opcode_igt:	// >
            if( debug_print_instr ) hal_printf("igt; ");
            { int operand = is_pop();	is_push( is_pop() > operand ); }
            break;
        case opcode_ilt:	// <
            if( debug_print_instr ) hal_printf("ilt; ");
            { int operand = is_pop();	is_push( is_pop() < operand ); }
            break;



        case opcode_i2o:
            if( debug_print_instr ) hal_printf("i2o; ");
            os_push(pvm_create_int_object(is_pop()));
            break;

        case opcode_o2i:
            if( debug_print_instr ) hal_printf("o2i; ");
            {
                struct pvm_object o = os_pop();
                if( o.data == 0 ) pvm_exec_throw("o2i(null)");
                ref_dec_o(o);
                is_push( pvm_get_int( o ) );
            }
            break;


        case opcode_os_eq:
            if( debug_print_instr ) hal_printf("os eq; ");
            {
                struct pvm_object o1 = os_pop();
                struct pvm_object o2 = os_pop();
                is_push( o1.data == o2.data );
                ref_dec_o(o1);
                ref_dec_o(o2);
                break;
            }

        case opcode_os_neq:
            if( debug_print_instr ) hal_printf("os neq; ");
            {
                struct pvm_object o1 = os_pop();
                struct pvm_object o2 = os_pop();
                is_push( o1.data != o2.data );
                ref_dec_o(o1);
                ref_dec_o(o2);
                break;
            }

        case opcode_os_isnull:
            if( debug_print_instr ) hal_printf("isnull; ");
            {
                struct pvm_object o1 = os_pop();
                is_push( pvm_is_null( o1 ) );
                ref_dec_o(o1);
                break;
            }

        case opcode_os_push_null:
            if( debug_print_instr ) hal_printf("push null; ");
            {
                os_push( pvm_create_null_object() );
                break;
            }


            // summoning, special ----------------------------------------------------

        case opcode_summon_null:
            if( debug_print_instr ) hal_printf("push null; ");
            os_push( pvm_create_null_object() ); // TODO BUG: so what opcode_os_push_null is for then?
            break;

        case opcode_summon_thread:
            if( debug_print_instr ) hal_printf("summon thread; ");
            ref_inc_o( current_thread );
            os_push( current_thread );
            //hal_printf("ERROR: summon thread; ");
            break;

        case opcode_summon_this:
            if( debug_print_instr ) hal_printf("summon this; ");
            ref_inc_o( this_object() );
            os_push( this_object() );
            break;

        case opcode_summon_class_class:
            if( debug_print_instr ) hal_printf("summon class class; ");
            // it has locked refcount
            os_push( pvm_get_class_class() );
            break;

        case opcode_summon_interface_class:
            if( debug_print_instr ) hal_printf("summon interface class; ");
            // locked refcnt
            os_push( pvm_get_interface_class() );
            break;

        case opcode_summon_code_class:
            if( debug_print_instr ) hal_printf("summon code class; ");
        	// locked refcnt
            os_push( pvm_get_code_class() );
            break;

        case opcode_summon_int_class:
            if( debug_print_instr ) hal_printf("summon int class; ");
        	// locked refcnt
            os_push( pvm_get_int_class() );
            break;

        case opcode_summon_string_class:
            if( debug_print_instr ) hal_printf("summon string class; ");
        	// locked refcnt
            os_push( pvm_get_string_class() );
            break;

        case opcode_summon_array_class:
            if( debug_print_instr ) hal_printf("summon array class; ");
        	// locked refcnt
            os_push( pvm_get_array_class() );
            break;

        case opcode_summon_by_name:
            {
                if( debug_print_instr ) hal_printf("summon by name; ");
                struct pvm_object name = pvm_code_get_string(&(da->code));
                //os_push(pvm_object_storage::get_string_class() );
                //hal_printf("ERROR: summon by name; "); name.my_data()->print();
                struct pvm_object cl = pvm_exec_lookup_class(da,name);
                // TODO: Need throw here? Just return null
                if( pvm_is_null( cl ) )
                    pvm_exec_throw("summon by name: null class");
                ref_inc_o( cl );
                os_push( cl );
            }
            break;

            /**
             * TODO
             * A BIG NOTE for object creation
             *
             * We must be SURE that it is NOT ever possible to pass
             * non-internal object as init data to internal and vice versa!
             * It would be a securily hole!
             *
             **/


        case opcode_new:
            if( debug_print_instr ) hal_printf("new; ");
            {
                //os_push( pvm_object_storage::create( os_pop(), 0 ) );
                pvm_object_t cl = os_pop();
                os_push( pvm_create_object( cl ) );
                ref_dec_o( cl );
            }
            break;

        case opcode_copy:
            if( debug_print_instr ) hal_printf("copy; ");
            // incs refcounts there
            os_push( pvm_copy_object( os_pop() ) );
            break;

            // TODO if you want to enable these, work out refcount
            // and security issues first!
            // compose/decompose
#if 0
        case opcode_os_compose32:
            if( debug_print_instr ) hal_printf(" compose32; ");
            {
                int num = pvm_code_get_int32(&(da->code));
                struct pvm_object in_class = os_pop();
                os_push( pvm_exec_compose_object( in_class, da->_ostack, num ) );
            }
            break;

        case opcode_os_decompose:
            if( debug_print_instr ) hal_printf(" decompose; ");
            {
                struct pvm_object to_decomp = os_pop();
                int num = da_po_limit(to_decomp.data);
                is_push( num );
                while( num )
                {
                    num--;
                    os_push( pvm_get_ofield( to_decomp, num) );
                }
                os_push(to_decomp.data->_class);
            }
            break;
#endif
            // string ----------------------------------------------------------------

        case opcode_sconst_bin:
            if( debug_print_instr ) hal_printf("sconst bin; ");
            os_push(pvm_code_get_string(&(da->code)));
            break;


            // flow ------------------------------------------------------------------

        case opcode_jmp:
            if( debug_print_instr ) hal_printf("jmp %d; ", da->code.IP);
            da->code.IP = pvm_code_get_rel_IP_as_abs(&(da->code));
            break;


        case opcode_djnz:
            {
                if( debug_print_instr ) hal_printf("djnz " );
                int new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                //is_top()--;
                is_push( is_pop() - 1 );
                if( is_top() ) da->code.IP = new_IP;

                if( debug_print_instr ) hal_printf("(%d) -> %d; ", is_top() , new_IP );
            }
            break;

        case opcode_jz:
            {
                if( debug_print_instr ) hal_printf("jz " );
                int new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                int test = is_pop();
                if( !test ) da->code.IP = new_IP;

                if( debug_print_instr ) hal_printf("(%d) -> %d; ", test , new_IP );
            }
            break;


        case opcode_switch:
            {
                if( debug_print_instr ) hal_printf("switch ");
                unsigned int tabsize    = pvm_code_get_int32(&(da->code));
                int shift               = pvm_code_get_int32(&(da->code));
                unsigned int divisor    = pvm_code_get_int32(&(da->code));
                int stack_top = is_pop();

                if( debug_print_instr ) hal_printf("(%d+%d)/%d, ", stack_top, shift, divisor );


                unsigned int start_table_IP = da->code.IP;
                unsigned int displ = (stack_top+shift)/divisor;
                unsigned int new_IP = start_table_IP+(tabsize*4); // default

                if( debug_print_instr ) hal_printf("displ %d, etab addr %d ", displ, new_IP );


                if( displ < tabsize )
                {
                    da->code.IP = start_table_IP+(displ*4); // BUG! 4!
                    if( debug_print_instr ) hal_printf("load from %d, ", da->code.IP );
                    new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                }
                da->code.IP = new_IP;

                if( debug_print_instr ) hal_printf("switch(%d) ->%d; ", displ, new_IP );
            }
            break;


        case opcode_ret:
            {
                if( DEB_CALLRET || debug_print_instr ) hal_printf("ret; ");
                //if( cf->estack.empty() ) throw except( "exec", "nowhere to return" );
                struct pvm_object ret = pvm_object_da( da->call_frame, call_frame )->prev;//da->call_frame.get_prev();
                if( pvm_is_null( ret ) )
                {
                    return;
                    //throw except( "exec", "nowhere to return" );
                }


                /*
                if(!os_empty())
                {
                    //ret.ostack().push(os_pop());

                    pvm_object os = pvm_object_da( ret, call_frame )->ostack;

                    pvm_ostack_push( pvm_object_da( os, object_stack ), os_pop() );
                }
                */

                //int do_return_val = !os_empty();
                pvm_object_t ret_val = os_empty() ?
                    pvm_create_null_object() : os_pop();

                //if(do_return_val) ret_val = ;

                // TODO dec refcnt for all old stack?
                // TODO dec refcount for old this!
                pvm_exec_save_fast_acc(da); // need?
                da->call_frame = ret;
                pvm_exec_load_fast_acc(da);

                //if(do_return_val)
                os_push( ret_val );
            }
            break;

            // exceptions are like ret ---------------------------------------------------

        case opcode_throw:
            if( DEB_CALLRET || debug_print_instr ) hal_printf("throw; ");
            pvm_exec_do_throw(da);
            break;

        case opcode_push_catcher:
            {
                unsigned addr = pvm_code_get_rel_IP_as_abs(&(da->code));
                if( debug_print_instr ) hal_printf("push catcher %u; ", addr );
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
            if( debug_print_instr ) hal_printf("pop catcher; ");
            //cf->pop_catcher();
            //call_frame.estack().pop();
            // TODO dec refcnt
            es_pop();
            break;

            // ok, now method calls ------------------------------------------------------

            // these 4 are parameter-less calls!
        case opcode_short_call_0:			pvm_exec_call(da,0,0);	break;
        case opcode_short_call_1:			pvm_exec_call(da,1,0);	break;
        case opcode_short_call_2:			pvm_exec_call(da,2,0);	break;
        case opcode_short_call_3:			pvm_exec_call(da,3,0);	break;

        case opcode_call_8bit:
            {
                int method_index = pvm_code_get_byte(&(da->code));
                int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param);
            }
            break;
        case opcode_call_32bit:
            {
                int method_index = pvm_code_get_int32(&(da->code));
                int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param);
            }
            break;


            // object stack --------------------------------------------------------------

        case opcode_os_dup:
            if( debug_print_instr ) hal_printf("os dup; ");
            {
                pvm_object_t o = os_top();
                ref_inc_o( o );
                os_push( o );//_ostack->top());
            }
            break;

        case opcode_os_drop:
            if( debug_print_instr ) hal_printf("os drop; ");
            ref_dec_o( os_pop() );
            break;

        case opcode_os_pull32:
            if( debug_print_instr ) hal_printf("os pull; ");
            {
                pvm_object_t o = os_pull(pvm_code_get_int32(&(da->code)));
                ref_inc_o( o );
                os_push( o );
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
                // Only sys can put thread asleep
                // If we are snapped here we, possibly, will continue from
                // the entry to this func. So save fasc acc and recheck
                // sleep condition on the func entry.
                if(da->sleep_flag)
                {
                    pvm_exec_save_fast_acc(da); // Before snap
                    phantom_thread_sleep_worker( da );
                }
                continue;
            }

            if( instruction  == opcode_sys_8bit )
            {
                pvm_exec_sys(da,pvm_code_get_byte(&(da->code))); //cf->cs.get_byte( cf->IP ));
                goto sys_sleep;
                //continue;
            }

            if( (instruction & 0xE0 ) == opcode_call_00 )
            {
                int n_param = pvm_code_get_byte(&(da->code));
                pvm_exec_call(da,instruction & 0x1F,n_param);
                continue;
            }

            hal_printf("Unknown op code 0x%X\n", instruction );
            pvm_exec_throw( "thread exec: unknown opcode" ); //, instruction );
            //exit(33);
        }
    }
}




static syscall_func_t pvm_exec_find_syscall( struct pvm_object _class, int syscall_index )
{
    if(!(_class.data->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_CLASS))
        pvm_exec_throw( "pvm_exec_find_syscall: not a class object" );

    struct data_area_4_class *da = (struct data_area_4_class *)&(_class.data->da);

    // TODO fix this back
    //if( syscall_index >= da->class_sys_table_size )                        throw except("find_syscall", "sys no out of table size" );

    syscall_func_t *tab = pvm_internal_classes[da->sys_table_id].syscalls_table;
    return tab[syscall_index];
}

/*
 *
 * Returns code object
 *
 */

struct pvm_object_storage * pvm_exec_find_method( struct pvm_object o, int method_index )
{
	if( o.data == 0 )
	{
		pvm_exec_throw( "pvm_exec_find_method: null object!" );
	}

	struct pvm_object_storage *iface = o.interface;
    if( iface == 0 ) //pvm_is_null( iface ) )
    {
    	if( o.data->_class.data == 0 )
    	{
            //dumpo(o.data);
            pvm_exec_throw( "pvm_exec_find_method: no interface and no class!" );
    	}
        iface = pvm_object_da( o.data->_class, class )->object_default_interface.data;
    }

    if( iface == 0 ) //pvm_is_null( iface ) )
        pvm_exec_throw( "pvm_exec_find_method: no interface found" );

    if(!(iface->_flags & PHANTOM_OBJECT_STORAGE_FLAG_IS_INTERFACE))
        pvm_exec_throw( "pvm_exec_find_method: not an interface object" );

    if(method_index > da_po_limit(iface))
        pvm_exec_throw( "pvm_exec_find_method: method index is out of bounds" );

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

    if( to_pop > max ) pvm_exec_throw("compose: cant compose so many fields");

    int i;
    for( i = 0; i < to_pop; i++ )
    {
        data_area[i] = pvm_ostack_pop( in_stack );
        ref_inc_o(data_area[i]);
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
    gc_root_add(code);
}


// TODO: implement!
struct pvm_object pvm_exec_lookup_class( struct data_area_4_thread *thread, struct pvm_object name)
{
    //pvm_exec_throw("pvm_exec_lookup_class: not implemented");

    // Try internal
    struct pvm_object ret = pvm_lookup_internal_class(name);
    if( !pvm_is_null(ret) )
        return ret;

    /*
     *
     * BUG! This executes code in a tight loop. It will prevent
     * snaps from being done. Redo with a separate thread start.
     *
     */

    // Try userland loader
    struct pvm_object args[1] = { name };
    return pvm_exec_run_method( pvm_root.class_loader, 8, 1, args );
}


/*
 *
 * BUG! This executes code in a tight loop. It will prevent
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
    for( i = n_args-1; i > -1; i-- )
    {
        ref_inc_o( args[i] );
        pvm_ostack_push( pvm_object_da(cfda->ostack, object_stack), args[i] );
    }

    pvm_istack_push( pvm_object_da(cfda->istack, integer_stack), n_args); // pass him real number of parameters

    struct pvm_object_storage *code = pvm_exec_find_method( this_object, method );
    pvm_exec_set_cs( cfda, code );
    ref_inc_o( cfda->this_object = this_object );

    pvm_object_t thread = pvm_create_thread_object( new_cf );

    gc_root_add( thread.data );
    pvm_exec( thread );

    pvm_object_t ret = pvm_ostack_pop( pvm_object_da(cfda->ostack, object_stack) );

    gc_root_rm( thread.data );
    ref_dec_o( thread );

    return ret;
}

