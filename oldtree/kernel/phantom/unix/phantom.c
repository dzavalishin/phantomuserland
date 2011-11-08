/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Unix syscalls related to native Phantom stuff.
 *
 *
**/

#if HAVE_UNIX

#define DEBUG_MSG_PREFIX "uphantom"
#include <debug_ext.h>
#define debug_level_flow 2
#define debug_level_error 10
#define debug_level_info 10

#include <vm/object.h>
#include <vm/alloc.h>
#include <vm/exec.h>
#include <unix/uuprocess.h>
#include <threads.h>

// -----------------------------------------------------------------------
// Create object and run thread on it
// -----------------------------------------------------------------------

struct cb_parm
{
    char *                              class_name;
    int                                 method_ordinal;
};


static void run_class_thread(void *arg)
{
    int nmethod;
    struct pvm_object _this;

    {
        struct cb_parm *p = arg;

        char tn[32];
        snprintf( tn, sizeof(tn), "=%s", p->class_name ? p->class_name : "??" );
        hal_set_thread_name(tn);

        nmethod = p->method_ordinal;

        SHOW_FLOW( 1, "run '%s'.%d", p->class_name, nmethod );

        pvm_object_t name = pvm_create_string_object(p->class_name);

        if( pvm_is_null( name ) )
        {
            SHOW_ERROR0( 0, "name is null?" );
            return;
        }

        pvm_object_t cls = pvm_exec_lookup_class_by_name( name );

        if( pvm_is_null( cls ) )
        {
            SHOW_ERROR( 0, "no class '%s'", p->class_name );
            return;
        }
        ref_dec_o(name);

        pvm_object_t _this = pvm_create_object(cls);

        if( pvm_is_null( _this ) )
        {
            SHOW_ERROR( 0, "can't create instance of '%s'", p->class_name );
            return;
        }
        ref_dec_o(cls);

        if(p->class_name) free(p->class_name);
        free(p);
    }

    struct pvm_object args[1];

    pvm_exec_run_method( _this, nmethod, 0, args );
    ref_dec_o(_this);

    // Just die, no more meaning of life
}

static errno_t run_cb( const char *cname, int nmethod )
{
    struct cb_parm *p = calloc(1, sizeof(struct cb_parm));
    if(!p)
    {
        return ENOMEM;
    }

    // TODO must add o to kernel referenced objects list?

    p->method_ordinal = nmethod;
    p->class_name = strdup( cname );

    int tid = hal_start_thread( run_class_thread, p, 0);

    if( tid < 0 )
    {
        free(p);
        return EAGAIN;
    }

    return 0;
}


void usys_phantom_runclass( errno_t *err, uuprocess_t *u, const char *cname, int nmethod )
{
    (void) u;

    errno_t rc = run_cb( cname, nmethod );
    if(err) *err = rc;
}




#endif // HAVE_UNIX
