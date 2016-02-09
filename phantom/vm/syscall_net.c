/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel ready: yes
 * Preliminary: no
 *
 *
**/


#include <phantom_libc.h>
#include <vm/syscall_net.h>
#include <vm/alloc.h>



static int debug_print = 0;





static int si_tcp_tostring_5(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    DEBUG_INFO;
    SYSCALL_RETURN( pvm_create_string_object( "tcp socket" ));
}


static int si_tcp_connect_16(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 2);

    //SYSCALL_PUT_THIS_THREAD_ASLEEP()

    SYSCALL_THROW_STRING( "not implemented" );
}

static int si_tcp_disconnect_17(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 2);


    SYSCALL_THROW_STRING( "not implemented" );
}




static int si_tcp_waitsend_18(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 0);

//    if(!si_tcp_ready_to_send(da))
//        SYSCALL_PUT_THIS_THREAD_ASLEEP();

    SYSCALL_THROW_STRING( "not implemented" );
    SYSCALL_RETURN_NOTHING;
}







static int si_tcp_send_19(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 2);


    SYSCALL_THROW_STRING( "not implemented" );
}






static int si_tcp_waitrecv_20(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 2);

//    if(!si_tcp_ready_to_recv(da))
//        SYSCALL_PUT_THIS_THREAD_ASLEEP();

    SYSCALL_THROW_STRING( "not implemented" );
}






static int si_tcp_recv_21(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 2);


    SYSCALL_THROW_STRING( "not implemented" );
}







static int si_tcp_waitaccept_22(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 1);
    int backlog = POP_INT();
    (void) backlog;

    //SYSCALL_PUT_THIS_THREAD_ASLEEP()

    SYSCALL_THROW_STRING( "not implemented" );
}


static int si_tcp_accept_23(struct pvm_object me , struct data_area_4_thread *tc )
{
    (void) me;
    //struct data_area_4_tcp      *da = pvm_data_area( me, tcp );

    DEBUG_INFO;
    int n_param = POP_ISTACK;

    CHECK_PARAM_COUNT(n_param, 2);


    SYSCALL_THROW_STRING( "not implemented" );
}










syscall_func_t	syscall_table_4_tcp[24] =
{
    &si_void_0_construct,           	&si_void_1_destruct,
    &si_void_2_class,               	&si_void_3_clone,
    &si_void_4_equals,              	&si_tcp_tostring_5,
    &si_void_6_toXML,               	&si_void_7_fromXML,
    // 8
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&si_void_15_hashcode,
    // 16
    &si_tcp_connect_16,    		&si_tcp_disconnect_17,
    &si_tcp_waitsend_18,               	&si_tcp_send_19,
    &si_tcp_waitrecv_20,    		&si_tcp_recv_21,
    &si_tcp_waitaccept_22,     		&si_tcp_accept_23,

};
DECLARE_SIZE(tcp);











#if 0

syscall_func_t	syscall_table_4_udp[26] =
{
    &si_void_0_construct,           	&si_void_1_destruct,
    &si_void_2_class,               	&si_void_3_clone,
    &si_void_4_equals,              	&si_udp_tostring_5,
    &si_void_6_toXML,               	&si_void_7_fromXML,
    // 8
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&invalid_syscall,
    &invalid_syscall,               	&si_void_15_hashcode,
    // 16
    &si_udp_bind_16,    		&si_udp_unbind_17,
    &si_udp_waitsend_18,               	&si_udp_send_19,
    &si_udp_waitrecv_20,    		&si_udp_recv_21,

    &si_udp_waitrecvfrom_22,     	&si_udp_recvfrom_23,
    &si_udp_waitsendto_24,     		&si_udp_sendto_25,

};
DECLARE_SIZE(udp);



#endif



