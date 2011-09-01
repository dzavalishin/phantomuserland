#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Minimalistic JIT
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/

#include <phantom_assert.h>
#include <errno.h>

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

#include "jit.h"



void jit_binary_op(
                   struct jit_out *j,
                   void (*func)( struct jit_out *j, int r_src, int r_dst ) )
{
    jit_is_pop( j );// pop AX
    jit_gen_mov( j, JIT_R_AX, JIT_R_DX ); // ax -> dx
    jit_is_pop( j );// pop AX
    func( j, JIT_R_DX, JIT_R_AX ); // ax += dx
    jit_is_push( j );// push AX

}

void jit_unary_op(
                   struct jit_out *j,
                   void (*func)( struct jit_out *j, int r_src, int r_dst ) )
{
    jit_is_pop( j );// pop AX
    func( j, JIT_R_DX, JIT_R_AX ); // ax += dx
    jit_is_push( j );// push AX
}


errno_t pvm_jit(struct pvm_object current_thread)
{

    struct jit_out      jo;
    struct jit_out      j = &jo;

    //struct data_area_4_thread *da = (struct data_area_4_thread *)&(current_thread.data->da);
    // TODO: check for current_thread to be thread for real

    // JITted code supposed to be called with da in BX

    jit_gen_push( j, JIT_R_BX );
    jit_gen_call( j, JIT_F_LOAD_F_ACC );
    //pvm_exec_load_fast_acc(da); // For any case

    {
    // Thread was snapped sleeping - resleep it
    //if(da->sleep_flag)        phantom_thread_sleep_worker( da );

        jit_load_thread_field( j, offsetof(da->sleep_flag) ); // to AX, gen flags
        jit_label jl = jit_get_label( j );
        jit_jz( jl );

        jit_gen_push( j, JIT_R_BX );
        jit_gen_call( j, JIT_F_THREAD_SLEEP_WORKER );
        jit_mark_label( j, jl );
    }

    while(1)
    {

        /*
        if(phantom_virtual_machine_snap_request)
        {
            pvm_exec_save_fast_acc(da); // Before snap
            phantom_thread_wait_4_snap();
            //pvm_exec_load_fast_acc(da); // We don't need this, if we die, we will enter again from above :)
        }
        */

        {
            jit_check_snap_request( j );

            jit_label jl = jit_get_label( j );
            jit_jz( jl );

            //jit_gen_push( j, JIT_R_BX );
            jit_gen_call( j, JIT_F_WAIT_SNAP );
            jit_mark_label( j, jl );
        }


        unsigned char instruction = pvm_code_get_byte(&(da->code));
        //printf("instr 0x%02X ", instruction);


        switch(instruction)
        {
        case opcode_nop:
            //if( debug_print_instr ) printf("nop; ");
            break;

        case opcode_debug:

            {
                int type = pvm_code_get_byte(&(da->code)); //cf->cs.get_instr( cf->IP );

                //printf("\n\nDebug 0x%02X", type );
                if( type & 0x80 )
                {
                    //printf(" (" );
                    pvm_object_t o = pvm_code_get_string(&(da->code));
                    //pvm_object_print(o);
                    ref_dec_o(o);
                    //printf(")" );
                }

                if( type & 0x01 ) debug_print_instr = 1;
                if( type & 0x02 ) debug_print_instr = 0;

                /*
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
                */
            }

            break;


            // int stack ops ---------------------------------------

        case opcode_is_dup:
            if( debug_print_instr ) printf("is dup; ");
            {
                //is_push(is_top());
                jit_is_top( j ); // AX = stack top
                jit_is_push( j );// push AX
            }
            break;

        case opcode_is_drop:
            if( debug_print_instr ) printf("is drop; ");
            //is_pop();
            jit_is_pop( j );
            break;

        case opcode_iconst_0:
            if( debug_print_instr ) printf("iconst 0; ");
            //is_push(0);
            jit_iconst( j, 0 ); // mov 0, %ax
            jit_is_push( j );// push AX
            break;

        case opcode_iconst_1:
            if( debug_print_instr ) printf("iconst 1; ");
            //is_push(1);
            jit_iconst( j, 1 ); // mov 1, %ax
            jit_is_push( j );// push AX
            break;

        case opcode_iconst_8bit:
            {
                int v = pvm_code_get_byte(&(da->code));
                //is_push(v);
                jit_iconst( j, v ); // mov v, %ax
                jit_is_push( j );// push AX
                if( debug_print_instr ) printf("iconst8 = %d; ", v);
                break;
            }

        case opcode_iconst_32bit:
            {
                int v = pvm_code_get_int32(&(da->code));
                //is_push(v);
                jit_iconst( j, v ); // mov v, %ax
                jit_is_push( j );// push AX
                if( debug_print_instr ) printf("iconst32 = %d; ", v);
                break;
            }


        case opcode_isum:
            if( debug_print_instr ) printf("isum; ");
            /*{
                //int add = is_pop();
                //is_push( is_pop() + add );
                jit_is_pop( j );// pop AX
                jit_gen_mov( j, JIT_R_AX, JIT_R_DX ); // ax -> dx
                jit_is_pop( j );// pop AX
                jit_gen_add( j, JIT_R_DX, JIT_R_AX ); // ax += dx
                jit_is_push( j );// push AX
            }*/
            jit_binary_op( j, jit_gen_add );
            break;

        case opcode_imul:
            if( debug_print_instr ) printf("imul; ");
            {
                //int mul = is_pop();
                //is_push( is_pop() * mul );
                jit_binary_op( j, jit_gen_mul );
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
            //{ int operand = is_pop();	is_push( is_pop() | operand ); }
            jit_binary_op( j, jit_gen_binor );
            break;

        case opcode_iand:
            if( debug_print_instr ) printf("iand; ");
            //{ int operand = is_pop();	is_push( is_pop() & operand ); }
            jit_binary_op( j, jit_gen_binand );
            break;

        case opcode_ixor:
            if( debug_print_instr ) printf("ixor; ");
            //{ int operand = is_pop();	is_push( is_pop() ^ operand ); }
            jit_binary_op( j, jit_gen_binxor );
            break;

        case opcode_inot:
            if( debug_print_instr ) printf("inot; ");
            //{ int operand = is_pop();	is_push( ~operand ); }
            jit_unary_op( j, jit_gen_binnot );
            break;



        case opcode_log_or:
            if( debug_print_instr ) printf("lor; ");
            /*{
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 || o2 );
            }*/
            jit_binary_op( j, jit_gen_logor );
            break;

        case opcode_log_and:
            if( debug_print_instr ) printf("land; ");
            /*{
                int o1 = is_pop();
                int o2 = is_pop();
                is_push( o1 && o2 );
            }*/
            jit_binary_op( j, jit_gen_logand );
            break;

        case opcode_log_xor:
            if( debug_print_instr ) printf("lxor; ");
            /*{
                int o1 = is_pop() ? 1 : 0;
                int o2 = is_pop() ? 1 : 0;
                is_push( o1 ^ o2 );
            }*/
            jit_binary_op( j, jit_gen_logxor );
            break;

        case opcode_log_not:
            if( debug_print_instr ) printf("lnot; ");
            /*{
                int operand = is_pop();
                is_push( !operand );
            }*/
            jit_unary_op( j, jit_gen_lognot );
            break;


        case opcode_ige:	// >=
            if( debug_print_instr ) printf("ige; ");
            //{ int operand = is_pop();	is_push( is_pop() >= operand ); }
            jit_binary_op( j, jit_gen_ige );
            break;
        case opcode_ile:	// <=
            if( debug_print_instr ) printf("ile; ");
            //{ int operand = is_pop();	is_push( is_pop() <= operand ); }
            jit_binary_op( j, jit_gen_ile );
            break;
        case opcode_igt:	// >
            if( debug_print_instr ) printf("igt; ");
            //{ int operand = is_pop();	is_push( is_pop() > operand ); }
            jit_binary_op( j, jit_gen_igt );
            break;
        case opcode_ilt:	// <
            if( debug_print_instr ) printf("ilt; ");
            //{ int operand = is_pop();	is_push( is_pop() < operand ); }
            jit_binary_op( j, jit_gen_ilt );
            break;



        case opcode_i2o:
            if( debug_print_instr ) printf("i2o; ");
            //os_push(pvm_create_int_object(is_pop()));
            jit_is_pop( j );// is pop -> AX
            jit_gen_call( j, JIT_F_CREATE_INT_OBJ ); // pvm_create_int_object
            jit_os_push( j );
            break;

        case opcode_o2i:
            if( debug_print_instr ) printf("o2i; ");
            {
                //struct pvm_object o = os_pop();
                //if( o.data == 0 ) pvm_exec_panic("o2i(null)");
                //is_push( pvm_get_int( o ) );
                //ref_dec_o(o);

                jit_os_pop( j );
                jit_o2int( j ); // AX = AX->int
                jit_is_push( j );// push AX
                jit_refdec( j ); // AX
            }
            break;


        case opcode_os_eq:
            if( debug_print_instr ) printf("os eq; ");
            {
                //struct pvm_object o1 = os_pop();
                //struct pvm_object o2 = os_pop();
                //is_push( o1.data == o2.data );
                //ref_dec_o(o1);
                //ref_dec_o(o2);

                jit_os_pop( j );// pop AX = data DX = iface
                jit_gen_mov( j, JIT_R_AX, JIT_R_CX ); // ax -> cx
                jit_os_pop( j );// pop AX = data DX = iface
                jit_gen_mov( j, JIT_R_AX, JIT_R_DX ); // ax -> dx 4 refdec

                jit_gen_cmp( j, JIT_R_CX, JIT_R_AX ); // ax = (ax == cx)

                jit_is_push( j );// push AX

                jit_refdec( j, JIT_R_CX );
                jit_refdec( j, JIT_R_DX );

                break;
            }

        case opcode_os_neq:
            if( debug_print_instr ) printf("os neq; ");
            {
                /*
                struct pvm_object o1 = os_pop();
                struct pvm_object o2 = os_pop();
                is_push( o1.data != o2.data );
                ref_dec_o(o1);
                ref_dec_o(o2);
                */

                jit_os_pop( j );// pop AX = data DX = iface
                jit_gen_mov( j, JIT_R_AX, JIT_R_CX ); // ax -> cx
                jit_os_pop( j );// pop AX = data DX = iface
                jit_gen_mov( j, JIT_R_AX, JIT_R_DX ); // ax -> dx 4 refdec

                jit_gen_cmp( j, JIT_R_CX, JIT_R_AX ); // ax = (ax == cx)
                jit_gen_neg( j, JIT_R_AX ); // ax = !ax

                jit_is_push( j );// push AX

                jit_refdec( j, JIT_R_CX );
                jit_refdec( j, JIT_R_DX );

                break;
            }

        case opcode_os_isnull:
            if( debug_print_instr ) printf("isnull; ");
            {
                struct pvm_object o1 = os_pop();
                is_push( pvm_is_null( o1 ) );
                ref_dec_o(o1);

                jit_os_pop( j );// pop AX = data DX = iface
                jit_gen_mov( j, JIT_R_AX, JIT_R_DX ); // ax -> dx 4 refdec

                jit_gen_isnull( j, JIT_R_AX ); // ax = (ax == 0)

                jit_is_push( j );// push AX

                jit_refdec( j, JIT_R_DX );

                break;
            }

        case opcode_os_push_null:
            if( debug_print_instr ) printf("push null; ");
            {
                //os_push( pvm_get_null_object() );
                jit_get_null( j );// AX = 0 DX = 0
                jit_os_push( j );// push AX
                break;
            }


            // summoning, special ----------------------------------------------------

        case opcode_summon_null:
            if( debug_print_instr ) printf("push null; ");
            jit_get_null( j );// AX = 0 DX = 0
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_thread:
            if( debug_print_instr ) printf("summon thread; ");
            //os_push( ref_inc_o( current_thread ) );
            jit_get_thread( j );// AX, DX = thread ptr
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_this:
            if( debug_print_instr ) printf("summon this; ");
            //os_push( ref_inc_o( this_object() ) );
            jit_get_this( j );// AX, DX = this ptr
            jit_refinc( j, JIT_R_AX );
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_class_class:
            if( debug_print_instr ) printf("summon class class; ");
            // it has locked refcount
            //os_push( pvm_get_class_class() );
            jit_get_class_class( j );// AX, DX = ptr
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_interface_class:
            if( debug_print_instr ) printf("summon interface class; ");
            // locked refcnt
            //os_push( pvm_get_interface_class() );
            jit_get_iface_class( j );// AX, DX = ptr
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_code_class:
            if( debug_print_instr ) printf("summon code class; ");
        	// locked refcnt
            //os_push( pvm_get_code_class() );
            jit_get_code_class( j );// AX, DX = thread ptr
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_int_class:
            if( debug_print_instr ) printf("summon int class; ");
        	// locked refcnt
            //os_push( pvm_get_int_class() );
            jit_get_int_class( j );// AX, DX = thread ptr
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_string_class:
            if( debug_print_instr ) printf("summon string class; ");
            // locked refcnt
            //os_push( pvm_get_string_class() );
            jit_get_string_class( j );// AX, DX = thread ptr
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_array_class:
            if( debug_print_instr ) printf("summon array class; ");
        	// locked refcnt
            //os_push( pvm_get_array_class() );
            jit_get_array_class( j );// AX, DX = thread ptr
            jit_os_push( j );// push AX, DX
            break;

        case opcode_summon_by_name:
            {
#error do it
                if( debug_print_instr ) printf("summon by name; ");
                struct pvm_object name = pvm_code_get_string(&(da->code));
                struct pvm_object cl = pvm_exec_lookup_class_by_name( name );
                ref_dec_o(name);
                // Need throw here?
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
             * A BIG NOTE for object creation
             *
             * We must be SURE that it is NOT ever possible to pass
             * non-internal object as init data to internal and vice versa!
             * It would be a securily hole!
             *
             **/


        case opcode_new:
            if( debug_print_instr ) printf("new; ");
            {
                //pvm_object_t cl = os_pop();
                //os_push( pvm_create_object( cl ) );
                //ref_dec_o( cl );  // object keep class ref

                jit_os_pop( j );// pop AX, DX
                jit_gen_call( j, JIT_F_CREATE_OBJ );
                jit_os_push( j );// push AX, DX
            }
            break;

        case opcode_copy:
            if( debug_print_instr ) printf("copy; ");
            {
                //pvm_object_t o = os_pop();
                //os_push( pvm_copy_object( o ) );
                //ref_dec_o(o);

                jit_os_pop( j );// pop AX, DX
                jit_gen_mov( j, JIT_R_AX, JIT_R_CX ); // ax -> cx 4 refdec
                jit_gen_call( j, JIT_F_COPY_OBJ );
                jit_os_push( j );// push AX, DX
                jit_refdec( j, JIT_R_CX );
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
#error
            if( debug_print_instr ) printf("sconst bin; ");
            //os_push(pvm_code_get_string(&(da->code)));
            {
                int constid = jit_alloc_string_const( j, pvm_code_get_string( &(da->code) ) );
                // const alloc code must saturate refcnt!
                jit_get_const_string( constid ); // AX, DX = str const obj
                //jit_refinc( JIT_R_AX );
                jit_os_push( j );// push AX, DX
            }
            break;


            // flow ------------------------------------------------------------------

        case opcode_jmp:
            {
                int newip = pvm_code_get_rel_IP_as_abs(&(da->code));
                if( debug_print_instr ) printf("jmp %d; ", newip );
                //da->code.IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                jit_jump( newip );
            }
            break;


        case opcode_djnz:
            {
#error impl
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
#error impl
                if( debug_print_instr ) printf("jz " );
                int new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                int test = is_pop();
                if( !test ) da->code.IP = new_IP;

                if( debug_print_instr ) printf("(%d) -> %d; ", test , new_IP );
            }
            break;


        case opcode_switch:
            {
#error impl
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
                    da->code.IP = start_table_IP+(displ*4); // BUG! 4!
                    if( debug_print_instr ) printf("load from %d, ", da->code.IP );
                    new_IP = pvm_code_get_rel_IP_as_abs(&(da->code));
                }
                da->code.IP = new_IP;

                if( debug_print_instr ) printf("switch(%d) ->%d; ", displ, new_IP );
            }
            break;


        case opcode_ret:
            {
#error impl
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
#error impl
            if( DEB_CALLRET || debug_print_instr ) printf( "\nthrow     (stack_depth %d -> ", da->stack_depth );
            pvm_exec_do_throw(da);
            if( DEB_CALLRET || debug_print_instr ) printf( "%d); ", da->stack_depth );
            break;

        case opcode_push_catcher:
            {
#error impl
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
#error impl
            if( debug_print_instr ) printf("pop catcher; ");
            //cf->pop_catcher();
            //call_frame.estack().pop();
            ref_dec_o( es_pop().object );
            break;

            // ok, now method calls ------------------------------------------------------

            // these 4 are parameter-less calls!
#error impl
        case opcode_short_call_0:           pvm_exec_call(da,0,0,1);   break;
        case opcode_short_call_1:           pvm_exec_call(da,1,0,1);   break;
        case opcode_short_call_2:           pvm_exec_call(da,2,0,1);   break;
        case opcode_short_call_3:           pvm_exec_call(da,3,0,1);   break;

        case opcode_call_8bit:
            {
#error impl
                unsigned int method_index = pvm_code_get_byte(&(da->code));
                unsigned int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param,1);
            }
            break;
        case opcode_call_32bit:
            {
#error impl
                unsigned int method_index = pvm_code_get_int32(&(da->code));
                unsigned int n_param = pvm_code_get_int32(&(da->code));
                pvm_exec_call(da,method_index,n_param,1);
            }
            break;


            // object stack --------------------------------------------------------------

        case opcode_os_dup:
            if( debug_print_instr ) printf("os dup; ");
            {
                //pvm_object_t o = os_top();
                //os_push( ref_inc_o( o ) );
                jit_os_top( j );// stack top -> AX, DX
                jit_refinc( j, JIT_R_AX );
                jit_os_push( j );// push AX, DX
            }
            break;

        case opcode_os_drop:
            if( debug_print_instr ) printf("os drop; ");
            //ref_dec_o( os_pop() );
            jit_os_pop( j );// pop AX = data, DX = iface
            jit_refdec( j, JIT_R_AX );
            break;

        case opcode_os_pull32:
            if( debug_print_instr ) printf("os pull; ");
            {
                //pvm_object_t o = os_pull(pvm_code_get_int32(&(da->code)));
                //os_push( ref_inc_o( o ) );
                int pos = pvm_code_get_int32(&(da->code));
                jit_os_pull( j, pos );// pull AX = data, DX = iface
                jit_refinc( j, JIT_R_AX );
                jit_os_push( j );// push AX, DX
            }
            break;

        case opcode_os_load8:
            //pvm_exec_load(da, pvm_code_get_byte(&(da->code)));
            {
                int slot = pvm_code_get_byte(&(da->code));
                jit_os_load( j, slot );// read obj slot AX = data, DX = iface
                jit_refinc( j, JIT_R_AX );
                jit_os_push( j );// push AX, DX
            }
            break;

        case opcode_os_load32:
            {
                //pvm_exec_load(da, pvm_code_get_int32(&(da->code)));
                int slot = pvm_code_get_int32(&(da->code));
                jit_os_load( j, slot );// read obj slot AX = data, DX = iface
                jit_refinc( j, JIT_R_AX );
                jit_os_push( j );// push AX, DX
                break;
            }

        case opcode_os_save8:
            //pvm_exec_save(da, pvm_code_get_byte(&(da->code)));
            {
                int slot = pvm_code_get_byte(&(da->code));
                jit_os_pop( j );// pop AX = data, DX = iface
                jit_os_save( j, slot );// wr obj slot AX = data, DX = iface
            }
            break;
        case opcode_os_save32:
            //pvm_exec_save(da, pvm_code_get_int32(&(da->code)));
            {
                int slot = pvm_code_get_int32(&(da->code));
                jit_os_pop( j );// pop AX = data, DX = iface
                jit_os_save( j, slot );// wr obj slot AX = data, DX = iface
            }
            break;

        case opcode_is_load8:
            //pvm_exec_iload(da, pvm_code_get_byte(&(da->code)));
            {
                int slot = pvm_code_get_byte(&(da->code));
                jit_os_load( j, slot );// read obj slot AX = data, DX = iface
                jit_o2int( j ); // AX = AX->int
                jit_is_push( j );// push AX
            }
            break;

        case opcode_is_save8:
            //pvm_exec_isave(da, pvm_code_get_byte(&(da->code)));
            {
                int slot = pvm_code_get_byte(&(da->code));
                jit_is_pop( j );// is pop -> AX
                jit_gen_call( j, JIT_F_CREATE_INT_OBJ ); // pvm_create_int_object
                //jit_int2o( j ); // AX = int_object(AX)
                jit_os_save( j, slot );
            }
            break;

        case opcode_os_get32:
            //pvm_exec_get(da, pvm_code_get_int32(&(da->code)));
            {
                int pos = pvm_code_get_int32(&(da->code));
                jit_os_absget( j, pos ); // AX, DX = obj
                jit_refinc( j, JIT_R_AX );
                jit_os_push( j );
            }
            break;

        case opcode_os_set32:
            //pvm_exec_set(da, pvm_code_get_int32(&(da->code)));
            {
                int pos = pvm_code_get_int32(&(da->code));
                jit_os_pop( j );
                jit_os_absset( j, pos ); 
            }
            break;

        case opcode_is_get32:
            //pvm_exec_iget(da, pvm_code_get_int32(&(da->code)));
            {
                int pos = pvm_code_get_int32(&(da->code));
                jit_is_absget( j, pos ); // AX, DX = obj
                jit_is_push( j );
            }
            break;
        case opcode_is_set32:
            //pvm_exec_iset(da, pvm_code_get_int32(&(da->code)));
            {
                int pos = pvm_code_get_int32(&(da->code));
                jit_is_pop( j );
                jit_is_absset( j, pos );
            }
            break;

        default:
            if( (instruction & 0xF0 ) == opcode_sys_0 )
            {
                jit_sys( j, instruction & 0x0F);
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
                break;
            }

            if( instruction  == opcode_sys_8bit )
            {
                jit_sys( j, pvm_code_get_byte(&(da->code))); 
                goto sys_sleep;
                //break;
            }

            if( (instruction & 0xE0 ) == opcode_call_00 )
            {
#error impl
                unsigned int n_param = pvm_code_get_byte(&(da->code));
                pvm_exec_call(da,instruction & 0x1F,n_param,0); //no optimization for soon return
                break;
            }

            printf("Unknown op code 0x%X\n", instruction );

            return ENOENT;
            //pvm_exec_panic( "thread exec: unknown opcode" ); //, instruction );
            //exit(33);
        }
    }
}



#endif
