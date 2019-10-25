/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2019 Dmitry Zavalishin, dz@dz.ru
 *
 * .internal.stringbuilder class implementation
 * 
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalClasses>
 * See <https://github.com/dzavalishin/phantomuserland/wiki/InternalMethodWritingGuide>
 *
**/


#define DEBUG_MSG_PREFIX "vm.sysc.sbuild"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>

#include "vm/object.h"
#include "vm/internal.h"
#include "vm/internal_da.h"
#include "vm/syscall.h"
#include "vm/root.h"
#include "vm/p2c.h"
#include "vm/alloc.h"

#include "i_stringbuilder.h"

static int debug_print = 0;



// --------- same as in string ---------------------------------------------------------

static int si_stringbuilder_3_clone( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
    SYSCALL_RETURN(pvm_create_stringbuilder_object_binary( (char *)sbda->str, sbda->len ));
}

static int si_stringbuilder_4_equals( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];

    int iret = 0;
    if( !pvm_is_null(him) )
    {
        ASSERT_STRING(him);

        struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
        struct data_area_4_stringbuilder *himda = pvm_object_da( him, string );

        iret =
            me->_class == him->_class &&
            sbda->len == himda->len &&
            0 == strncmp( (const char*)sbda->str, (const char*)himda->str, sbda->len )
            ;
    }
    SYS_FREE_O(him);

    // BUG - can compare just same classes? Call his .compare if he is not string?
    SYSCALL_RETURN(pvm_create_int_object( iret ) );
}

static int si_stringbuilder_5_tostring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
    SYSCALL_RETURN(pvm_create_string_object_binary( (char *)sbda->str, sbda->len ));

}

static int si_stringbuilder_8_substring( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(2);

    int parmlen = AS_INT(args[1]);
    int index = AS_INT(args[0]);


    if( index < 0 || index >= sbda->len )
        SYSCALL_THROW_STRING( "string.substring index is out of bounds" );

    int len = sbda->len - index;
    if( parmlen < len ) len = parmlen;

    if( len < 0 )
        SYSCALL_THROW_STRING( "string.substring length is negative" );

    SYSCALL_RETURN(pvm_create_string_object_binary( (char *)sbda->str + index, len ));
}

static int si_stringbuilder_9_byteat( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(1);
    int index = AS_INT(args[0]);

    int len = sbda->len;

    if(index > len-1 )
        SYSCALL_THROW_STRING( "stringbuilder.charAt index is out of bounds" );

    SYSCALL_RETURN(pvm_create_int_object( sbda->str[index]  ));
}

/// Concat string to me
static int si_stringbuilder_10_concat( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];
    ASSERT_STRING(him);

    struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
    struct data_area_4_string *himda = pvm_object_da( him, string );
/*
    pvm_object_t oret = pvm_create_string_object_binary_cat(
                (char *)sbda->data, sbda->length,
                (char *)himda->data, himda->length );
*/

    size_t newlen = sbda->len + himda->length;

    // Use < to have place for final '\0'
    if( newlen >= sbda->bufsize )
    {
        // Will resize
        pvm_object_t news = pvm_create_string_object_binary( sbda->str, newlen * 2 );
        assert( !pvm_isnull(news) );
        SYS_FREE_O( sbda->buffer );
        sbda->buffer = news;

        struct data_area_4_string *new_s_da = pvm_object_da( news, string );

        //sbda->len is the same
        sbda->str = new_s_da->data;
        sbda->bufsize = new_s_da->length;
        sbda->update_count++;
    }
    
    // Have place, just copy in
    strlcpy( sbda->str + sbda->len, himda->data, himda->length + 1 );

    SYS_FREE_O(him);

    SYSCALL_RETURN_INT( newlen );
}


static int si_stringbuilder_11_length( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    ASSERT_STRING(me);
    struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
    
    CHECK_PARAM_COUNT(0);

    SYSCALL_RETURN(pvm_create_int_object( sbda->len ));
}

static int si_stringbuilder_12_find( pvm_object_t me, pvm_object_t *ret, struct data_area_4_thread *tc, int n_args, pvm_object_t *args )
{
    DEBUG_INFO;
    
    CHECK_PARAM_COUNT(1);
    pvm_object_t him = args[0];
    ASSERT_STRING(him);

    struct data_area_4_stringbuilder *sbda = pvm_object_da( me, string );
    struct data_area_4_string *himda = pvm_object_da( him, string );

    unsigned char * cret = (unsigned char *)strnstrn(
                (char *)sbda->str, sbda->len,
                (char *)himda->data, himda->length );

    SYS_FREE_O(him);

    int pos = -1;

    if( cret != 0 )
        pos = cret - (sbda->str);

    SYSCALL_RETURN(pvm_create_int_object( pos ));
}








syscall_func_t  syscall_table_4_window[16] =
{
    &si_void_0_construct,           &si_void_1_destruct,
    &si_void_2_class,               &si_stringbuilder_3_clone,
    &si_stringbuilder_4_equals,     &si_stringbuilder_5_tostring,
    &si_void_6_toXML,               &si_void_7_fromXML,
    // 8
    &si_stringbuilder_8_substring,  &si_stringbuilder_9_byteat,
    &si_stringbuilder_10_concat,    &si_stringbuilder_11_length,
    &si_stringbuilder_12_find,      &invalid_syscall,
    &invalid_syscall,               &si_void_15_hashcode,
    // 16

};
DECLARE_SIZE(window);




void pvm_internal_init_window(pvm_object_t os)
{
    struct data_area_4_window      *da = (struct data_area_4_window *)os->da;

    //pvm_object_t bin = pvm_create_binary_object( PVM_MAX_TTY_PIXELS * 4 + sizeof(drv_video_window_t), 0 );
    pvm_object_t bin = pvm_create_binary_object( drv_video_window_bytes( PVM_DEF_TTY_XSIZE, PVM_DEF_TTY_YSIZE ) + sizeof(drv_video_window_t), 0 );
    da->o_pixels = bin;

    struct data_area_4_binary *bda = (struct data_area_4_binary *)bin->da;

    void *pixels = &bda->data;

    strlcpy( da->title, "Window", sizeof(da->title) );

    da->fg = COLOR_BLACK;
    da->bg = COLOR_WHITE;
    da->x = 0;
    da->y = 0;
    da->autoupdate = 1;

    //lprintf("pvm_internal_init_window w %p pix %p\n", &(da->w), pixels );

    drv_video_window_init( &(da->w), pixels, PVM_DEF_TTY_XSIZE, PVM_DEF_TTY_YSIZE, 100, 100, da->bg, WFLAG_WIN_DECORATED, da->title );

    {
    pvm_object_t o;
    o = os;


    da->connector = pvm_create_connection_object();
    struct data_area_4_connection *cda = (struct data_area_4_connection *)(da->connector->da);

    phantom_connect_object_internal(cda, 0, o, 0);

    // This object needs OS attention at restart
    // TODO do it by class flag in create fixed or earlier?
    pvm_add_object_to_restart_list( o );
    }

}


void pvm_gc_iter_window(gc_iterator_call_t func, pvm_object_t  os, void *arg)
{
    struct data_area_4_window *da = (struct data_area_4_window *)os->da;

    func( arg, da->connector );
    func( arg, da->o_pixels );
}


pvm_object_t     pvm_create_window_object(pvm_object_t owned )
{
    //pvm_object_t ret = pvm_object_create_fixed( pvm_get_window_class() );
    pvm_object_t ret = pvm_create_object( pvm_get_window_class() );
    struct data_area_4_window *da = (struct data_area_4_window *)ret->da;

    //lprintf("pvm_create_window_object %p n", ret );

    (void)da;

    return ret;
}

void pvm_gc_finalizer_window( pvm_object_t  os )
{
    // is it called?
    struct data_area_4_window      *da = (struct data_area_4_window *)os->da;

    //struct data_area_4_binary *bda = (struct data_area_4_binary *)da->o_pixels->da;
    //void *pixels = &bda->data;

    drv_video_window_destroy(&(da->w));
}


void pvm_restart_window( pvm_object_t o )
{
    pvm_add_object_to_restart_list( o ); // Again!

    struct data_area_4_window *da = pvm_object_da( o, window );

    struct data_area_4_binary *bda = (struct data_area_4_binary *)da->o_pixels->da;
    window_handle_t pixels = (window_handle_t)&bda->data;

    printf("restart WIN\n");

    w_restart_init( &(da->w), pixels );

    //&(da->w)->title = da->title; // must be correct in snap? don't reset?
    w_set_title( &(da->w), da->title );

    /*
    queue_init(&(da->w.events));
    da->w.events_count = 0;

    iw_enter_allwq( &da->w );

    //event_q_put_win( 0, 0, UI_EVENT_WIN_REPAINT, &da->w );
    ev_q_put_win( 0, 0, UI_EVENT_WIN_REDECORATE, &da->w );
    */
    w_restart_attach( &(da->w) );
}








