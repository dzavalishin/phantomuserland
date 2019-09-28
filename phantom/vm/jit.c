#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Minimalistic JIT - supposed to be based on libjit
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

static int debug_print_instr = 0;


void jit_binary_op(
                   struct jit_out *j,
                   jit_value_t (*func)( struct jit_out *j, jit_value_t r_src, jit_value_t r_dst ) )
{
    jit_value_t a1 = jit_is_pop( j );
    jit_value_t a2 = jit_is_pop( j );
    jit_value_t r = func( j, a1, a2 );
    jit_is_push( j, r );
}

void jit_unary_op(
                   struct jit_out *j,
                   jit_value_t  (*func)( struct jit_out *j, jit_value_t  r_src ) )
{
    jit_is_push( j, func( j, jit_is_pop( j ) ) );
}


errno_t jit_compile_method(struct pvm_code_handler *code)
{

    struct jit_out      jo;
    struct jit_out      *j = &jo;

    //struct data_area_4_thread *da = (struct data_area_4_thread *)&(current_thread->da);
    // TODO: check for current_thread to be thread for real

    // JITted code supposed to be called with da in BX

    //jit_gen_push( j, JIT_R_BX );
    //jit_gen_call( j, JIT_F_LOAD_F_ACC );
    //pvm_exec_load_fast_acc(da); // For any case

    {
#warning resleep?
    // Thread was snapped sleeping - resleep it
        //if(da->sleep_flag)        phantom_thread_sleep_worker( da );
        /*
        jit_load_thread_field( j, offsetof(da->sleep_flag) ); // to AX, gen flags
        jit_label jl = jit_get_label( j );
        jit_jz( jl );

        jit_gen_push( j, JIT_R_BX );
        jit_gen_call( j, JIT_F_THREAD_SLEEP_WORKER );
        jit_mark_label( j, jl );
        */
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
            /*
            jit_label jl = jit_get_label( j );
            jit_jz( jl );

            //jit_gen_push( j, JIT_R_BX );
            jit_gen_call( j, JIT_F_WAIT_SNAP );
            jit_mark_label( j, jl );
            */
        }


        jit_mark_possible_label(j);

        unsigned char instruction = pvm_code_get_byte(code);
        //printf("instr 0x%02X ", instruction);


        switch(instruction)
        {
        case opcode_nop:
            //if( debug_print_instr ) printf("nop; ");
            break;

        case opcode_debug:

            {
                int type = pvm_code_get_byte(code); //cf->cs.get_instr( cf->IP );

                //printf("\n\nDebug 0x%02X", type );
                if( type & 0x80 )
                {
                    //printf(" (" );
                    pvm_object_t o = pvm_code_get_string(code);
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
            jit_is_push( j, jit_is_top( j ) );
            break;

        case opcode_is_drop:
            if( debug_print_instr ) printf("is drop; ");
            jit_is_pop( j );
            break;

        case opcode_iconst_0:
            if( debug_print_instr ) printf("iconst 0; ");
            jit_is_push( j, jit_iconst( j, 0 ) );
            break;

        case opcode_iconst_1:
            if( debug_print_instr ) printf("iconst 1; ");
            jit_is_push( j, jit_iconst( j, 1 ) );
            break;

        case opcode_iconst_8bit:
            {
                int v = pvm_code_get_byte(code);
                jit_is_push( j, jit_iconst( j, v ) );
                if( debug_print_instr ) printf("iconst8 = %d; ", v);
                break;
            }

        case opcode_iconst_32bit:
            {
                int v = pvm_code_get_int32(code);
                jit_is_push( j, jit_iconst( j, v ) );
                if( debug_print_instr ) printf("iconst32 = %d; ", v);
                break;
            }


        case opcode_isum:
            if( debug_print_instr ) printf("isum; ");
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
                jit_value_t u = jit_is_pop( j );
                jit_value_t l = jit_is_pop( j );
                jit_value_t r = jit_gen_sub( j, u, l );
                jit_is_push( j, r );
            }
            break;

        case opcode_isublu:
            if( debug_print_instr ) printf("isublu; ");
            {
                jit_value_t u = jit_is_pop( j );
                jit_value_t l = jit_is_pop( j );
                jit_value_t r = jit_gen_sub( j, l, u );
                jit_is_push( j, r );
            }
            break;

        case opcode_idivul:
            if( debug_print_instr ) printf("idivul; ");
            {
                jit_value_t u = jit_is_pop( j );
                jit_value_t l = jit_is_pop( j );
                jit_value_t r = jit_gen_div( j, u, l );
                jit_is_push( j, r );
            }
            break;

        case opcode_idivlu:
            if( debug_print_instr ) printf("idivlu; ");
            {
                jit_value_t u = jit_is_pop( j );
                jit_value_t l = jit_is_pop( j );
                jit_value_t r = jit_gen_div( j, l, u );
                jit_is_push( j, r );
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
            jit_unary_op( j, jit_gen_binnot );
            break;



        case opcode_log_or:
            if( debug_print_instr ) printf("lor; ");
            jit_binary_op( j, jit_gen_logor );
            break;

        case opcode_log_and:
            if( debug_print_instr ) printf("land; ");
            jit_binary_op( j, jit_gen_logand );
            break;

        case opcode_log_xor:
            if( debug_print_instr ) printf("lxor; ");
            jit_binary_op( j, jit_gen_logxor );
            break;

        case opcode_log_not:
            if( debug_print_instr ) printf("lnot; ");
            jit_unary_op( j, jit_gen_lognot );
            break;


        case opcode_ige:	// >=
            if( debug_print_instr ) printf("ige; ");
            jit_binary_op( j, jit_gen_ige );
            break;
        case opcode_ile:	// <=
            if( debug_print_instr ) printf("ile; ");
            jit_binary_op( j, jit_gen_ile );
            break;
        case opcode_igt:	// >
            if( debug_print_instr ) printf("igt; ");
            jit_binary_op( j, jit_gen_igt );
            break;
        case opcode_ilt:	// <
            if( debug_print_instr ) printf("ilt; ");
            jit_binary_op( j, jit_gen_ilt );
            break;



        case opcode_i2o:
            if( debug_print_instr ) printf("i2o; ");
            jit_push_arg( j, jit_is_pop( j ) );
            jit_os_push( j, jit_gen_call( j, JIT_F_CREATE_INT_OBJ ) );
            break;

        case opcode_o2i:
            if( debug_print_instr ) printf("o2i; ");
            {
                jit_is_push( j, jit_o2int( j, jit_os_pop( j ) ) );// push AX
                //jit_refdec( j ); // TODO jit_o2int must refdec?
            }
            break;


        case opcode_os_eq:
            if( debug_print_instr ) printf("os eq; ");
            {
                jit_value_t v1 = jit_os_pop( j );
                jit_value_t v2 = jit_os_pop( j );

                jit_is_push( j, jit_gen_cmp( j, v1, v2 ) );

                jit_refdec( j, v1 );
                jit_refdec( j, v2 );

                break;
            }

        case opcode_os_neq:
            if( debug_print_instr ) printf("os neq; ");
            {
                jit_value_t v1 = jit_os_pop( j );
                jit_value_t v2 = jit_os_pop( j );

                jit_is_push( j, jit_gen_lognot( j, jit_gen_cmp( j, v1, v2 )) );

                jit_refdec( j, v1 );
                jit_refdec( j, v2 );

                break;
            }

        case opcode_os_isnull:
            if( debug_print_instr ) printf("isnull; ");
            {
                jit_value_t o1 = jit_os_pop( j );
                jit_is_push( j, jit_is_null( j, o1 ) );
                jit_refdec( j, o1 );
                break;
            }

        case opcode_os_push_null:
        case opcode_summon_null:
            if( debug_print_instr ) printf("push null; ");
            {
                jit_os_push( j, jit_get_null( j ) );
                break;
            }

            // summoning, special ----------------------------------------------------

        case opcode_summon_thread:
            if( debug_print_instr ) printf("summon thread; ");
            jit_os_push( j, jit_get_thread( j ) );
            break;

        case opcode_summon_this:
            if( debug_print_instr ) printf("summon this; ");
            jit_os_push( j, jit_refinc( j, jit_get_this( j ) ) );
            break;

        case opcode_summon_class_class:
            if( debug_print_instr ) printf("summon class class; ");
            jit_os_push( j, jit_get_class_class( j ) );
            break;

        case opcode_summon_interface_class:
            if( debug_print_instr ) printf("summon interface class; ");
            // locked refcnt
            jit_os_push( j, jit_get_iface_class( j ) );
            break;

        case opcode_summon_code_class:
            if( debug_print_instr ) printf("summon code class; ");
            // locked refcnt
            jit_os_push( j, jit_get_code_class( j ) );
            break;

        case opcode_summon_int_class:
            if( debug_print_instr ) printf("summon int class; ");
            // locked refcnt
            jit_os_push( j, jit_get_int_class( j ) );
            break;

        case opcode_summon_string_class:
            if( debug_print_instr ) printf("summon string class; ");
            // locked refcnt
            jit_os_push( j, jit_get_string_class( j ) );
            break;

        case opcode_summon_array_class:
            if( debug_print_instr ) printf("summon array class; ");
            // locked refcnt
            jit_os_push( j, jit_get_array_class( j ) );
            break;

            // TODO we can turn class to const, but the we have to keep refcount for it in
            // const space and release when we release compiled code
            // TODO const list is just an array? easy to release all

        case opcode_summon_by_name:
            {
                if( debug_print_instr ) printf("summon by name; ");

                pvm_object_t name = pvm_code_get_string(code);
                pvm_object_t cl = pvm_exec_lookup_class_by_name( name ); // TODO XXX must inc ref
                ref_dec_o(name);

                // Need throw here?

                if( pvm_is_null( cl ) ) {
                    printf("JIT: summon by name: null class\n");
                    return ENOENT;

                    // TODO XXX generate summon in run time?

                }
                int cid = jit_alloc_const( j, cl );
                jit_os_push( j, jit_get_const( j, cid ) );

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
                jit_os_push( j, jit_gen_call( j, JIT_F_CREATE_OBJ ) );// push AX, DX
            }
            break;

        case opcode_copy:
            if( debug_print_instr ) printf("copy; ");
            {
                jit_push_arg( j, jit_os_pop( j ) );
                jit_os_push( j, jit_gen_call( j, JIT_F_COPY_OBJ ) );
                // jit_refdec( j, JIT_R_CX ); XXX TODO need?
            }
            break;

            // if you want to enable these, work out refcount
            // and security issues first!
            // compose/decompose
#if 0
        case opcode_os_compose32:
            if( debug_print_instr ) printf(" compose32; ");
            {
                int num = pvm_code_get_int32(code);
                pvm_object_t in_class = os_pop();
                os_push( pvm_exec_compose_object( in_class, da->_ostack, num ) );
            }
            break;

        case opcode_os_decompose:
            if( debug_print_instr ) printf(" decompose; ");
            {
                pvm_object_t to_decomp = os_pop();
                int num = da_po_limit(to_decomp);
                is_push( num );
                while( num )
                {
                    num--;
                    pvm_object_t o = pvm_get_ofield( to_decomp, num);
                    os_push( ref_inc_o( o ) );
                }
                os_push(to_decomp->_class);
            }
            break;
#endif
            // string ----------------------------------------------------------------

        case opcode_sconst_bin:
            if( debug_print_instr ) printf("sconst bin; ");
            //os_push(pvm_code_get_string(code));
            {
                int constid = jit_alloc_const( j, pvm_code_get_string( code ) );
                jit_os_push( j, jit_get_const( j, constid ) );
            }
            break;


            // flow ------------------------------------------------------------------

        case opcode_jmp:
            {
                int newip = pvm_code_get_rel_IP_as_abs(code);
                if( debug_print_instr ) printf("jmp %d; ", newip );
                //da->code.IP = pvm_code_get_rel_IP_as_abs(code);
                jit_jump( j, newip );
            }
            break;


        case opcode_djnz:
            {
                if( debug_print_instr ) printf("djnz " );
                int newip = pvm_code_get_rel_IP_as_abs(code);

                //is_push( is_pop() - 1 );
                jit_decrement( j, jit_is_top(j) );
                jit_jnz( j, newip, jit_is_top(j) );

                //if( debug_print_instr ) printf("(%d) -> %d; ", is_top() , new_IP );
            }
            break;

        case opcode_jz:
            {
                if( debug_print_instr ) printf("jz " );
                int newip = pvm_code_get_rel_IP_as_abs(code);
                jit_jz( j, newip, jit_is_pop(j) );
            }
            break;


        case opcode_switch:
            {
#warning impl
                /*
                if( debug_print_instr ) printf("switch ");
                unsigned int tabsize    = pvm_code_get_int32(code);
                int shift               = pvm_code_get_int32(code);
                unsigned int divisor    = pvm_code_get_int32(code);
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
                    new_IP = pvm_code_get_rel_IP_as_abs(code);
                }
                da->code.IP = new_IP;

                if( debug_print_instr ) printf("switch(%d) ->%d; ", displ, new_IP );
                */
                printf("no switch yet");
                return ENXIO;
            }
            break;


        case opcode_ret:
            {
                if( debug_print_instr ) printf( "\nret\n" );
                jit_ret(j);
            }
            break;

            // exceptions are like ret ---------------------------------------------------

        case opcode_throw:
            if( debug_print_instr ) printf( "\nthrow\n" );
            jit_throw(j, jit_os_pop(j) );
            break;

        case opcode_push_catcher:
            {
                unsigned addr = pvm_code_get_rel_IP_as_abs(code);
                if( debug_print_instr ) printf("push catcher %u; ", addr );
                jit_es_push( j, jit_os_pop(j), addr );
            }
            break;

        case opcode_pop_catcher:
            if( debug_print_instr ) printf("pop catcher; ");
            jit_refdec( j, jit_es_pop(j) );
            break;

            // ok, now method calls ------------------------------------------------------

            // these 4 are parameter-less calls!
        case opcode_short_call_0:
            jit_os_push( j, jit_call_method( j, 0, 0, 1) );
            break;
        case opcode_short_call_1:
            jit_os_push( j, jit_call_method( j, 1, 0, 1) );
            break;
        case opcode_short_call_2:
            jit_os_push( j, jit_call_method( j, 2, 0, 1) );
            break;
        case opcode_short_call_3:
            jit_os_push( j, jit_call_method( j, 3, 0, 1) );
            break;

        case opcode_call_8bit:
            {
                unsigned int method_index = pvm_code_get_byte(code);
                unsigned int n_param = pvm_code_get_int32(code);
                jit_os_push( j, jit_call_method( j, method_index, n_param, 1) ); // optimization for soon return
            }
            break;
        case opcode_call_32bit:
            {
                unsigned int method_index = pvm_code_get_int32(code);
                unsigned int n_param = pvm_code_get_int32(code);
                jit_os_push( j, jit_call_method( j, method_index, n_param, 1) ); // optimization for soon return
            }
            break;


            // object stack --------------------------------------------------------------

        case opcode_os_dup:
            if( debug_print_instr ) printf("os dup; ");
            jit_os_push( j, jit_refinc( j, jit_os_top( j ) ) );
            break;

        case opcode_os_drop:
            if( debug_print_instr ) printf("os drop; ");
            jit_refdec( j, jit_os_pop( j ) );
            break;

        case opcode_os_pull32:
            if( debug_print_instr ) printf("os pull; ");
            {
                int pos = pvm_code_get_int32(code);
                jit_os_push( j, jit_refinc( j, jit_os_pull( j, pos ) )  ); // TODO XXX load must refinc!
            }
            break;

        case opcode_os_load8:
            {
                int slot = pvm_code_get_byte(code);
                jit_os_push( j, jit_os_load( j, slot ) ); // TODO XXX load must refinc!
            }
            break;

        case opcode_os_load32:
            {
                int slot = pvm_code_get_int32(code);
                jit_os_push( j, jit_os_load( j, slot ) ); // TODO XXX load must refinc!
                break;
            }

        case opcode_os_save8:
            {
                int slot = pvm_code_get_byte(code);
                jit_os_save( j, slot, jit_os_pop( j ) );
            }
            break;

        case opcode_os_save32:
            {
                int slot = pvm_code_get_int32(code);
                jit_os_save( j, slot, jit_os_pop( j ) );
            }
            break;

        case opcode_is_load8:
            //pvm_exec_iload(da, pvm_code_get_byte(code));
            {
                int slot = pvm_code_get_byte(code);
                jit_value_t ov = jit_os_load( j, slot );
                jit_value_t iv = jit_o2int( j, ov ); // TODO XXX jit_o2int must gen refdec
                jit_is_push( j, iv );
            }
            break;

        case opcode_is_save8:
            {
                int slot = pvm_code_get_byte(code);
                jit_push_arg( j, jit_is_pop( j ) ); // TODO arg is int?
                // pvm_create_int_object
                jit_os_save( j, slot, jit_gen_call( j, JIT_F_CREATE_INT_OBJ ) );
            }
            break;

        case opcode_os_get32:
            {
                int pos = pvm_code_get_int32(code);
                jit_value_t v = jit_os_absget( j, pos ); // AX, DX = obj
                //jit_refinc( j, v ); // TODO XXX must not be needed - absget must inc!
                jit_os_push( j, v );
            }
            break;

        case opcode_os_set32:
            {
                int pos = pvm_code_get_int32(code);
                jit_os_absset( j, pos, jit_os_pop( j ) );
            }
            break;

        case opcode_is_get32:
            {
                int pos = pvm_code_get_int32(code);
                jit_is_push( j, jit_is_absget( j, pos ) );
            }
            break;
        case opcode_is_set32:
            {
                int pos = pvm_code_get_int32(code);
                jit_is_absset( j, pos, jit_is_pop( j ) );
            }
            break;

        default:
            if( (instruction & 0xF0 ) == opcode_sys_0 )
            {
                jit_sys( j, instruction & 0x0F);
                /*
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
                */
            }

            if( instruction  == opcode_sys_8bit )
            {
                jit_sys( j, pvm_code_get_byte(code)); 
                //goto sys_sleep;
                break;
            }

            if( (instruction & 0xE0 ) == opcode_call_00 )
            {
                unsigned int n_param = pvm_code_get_byte(code);
                jit_os_push( j, jit_call_method( j, instruction & 0x1F, n_param, 0) ); //no optimization for soon return
                break;
            }

            printf("JIT: Unknown op code 0x%X\n", instruction );

            return ENOENT;
        }
    }
}



#endif
