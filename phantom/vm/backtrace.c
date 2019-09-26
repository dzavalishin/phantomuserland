/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Bytecode backtrace
 *
 *
**/

#define DEBUG_MSG_PREFIX "vm.bt"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <vm/exec.h>
#include <vm/code.h>
#include <vm/alloc.h>
#include <vm/reflect.h>

#include <threads.h>
#include <exceptions.h>
#include <thread_private.h>

//static void pvm_backtrace(void);

//pvm_object_t pvm_get_method_name( pvm_object_t tclass, int method_ordinal );
//int pvm_ip_to_linenum(pvm_object_t tclass, int method_ordinal, int ip);

#if CONF_USE_E4C



E4C_DEFINE_EXCEPTION(PvmException, "Virtual machine error.", RuntimeException);
E4C_DEFINE_EXCEPTION(PvmCodeException, "Virtual machine bytecode error.", PvmException);
E4C_DEFINE_EXCEPTION(PvmDataException, "Virtual machine data error.", PvmException);



E4C_DEFINE_EXCEPTION(UnixException, "Unix subsystem error.", RuntimeException);
E4C_DEFINE_EXCEPTION(UnixSendSignalException, "Unix subsystem error.", UnixException);

#endif // e4c


/* Poor man's exceptions */
/* coverity[+kill] */
void pvm_exec_panic0( const char *reason )
{
    // TO DO: longjmp?
    //panic("pvm_exec_throw: %s", reason );
    //syslog()
    printf("pvm_exec_panic: %s\n", reason );
    pvm_backtrace_current_thread();

    pvm_memcheck();
#if CONF_USE_E4C
    printf("pvm_exec_panic: throwing\n", reason );
    E4C_THROW( PvmException, reason );
#else // CONF_USE_E4C
    hal_exit_kernel_thread();
#endif // CONF_USE_E4C
}

void pvm_exec_panic( const char *reason, struct data_area_4_thread *tda )
{
    // TO DO: longjmp?
    //panic("pvm_exec_throw: %s", reason );
    //syslog()
    printf("pvm_exec_panic: %s\n", reason );
    pvm_backtrace(tda);

    pvm_memcheck();
#if CONF_USE_E4C
    printf("pvm_exec_panic: throwing\n", reason );
    E4C_THROW( PvmException, reason );
#else // CONF_USE_E4C
    hal_exit_kernel_thread();
#endif // CONF_USE_E4C
}


void pvm_backtrace_current_thread(void)
{
    errno_t e = ENOENT;
    int tid = get_current_tid();
    if( tid < 0 )
    {
        printf("pvm_backtrace - get_current_tid failed!\n");
        goto nope;
    }

    void *owner;
    if( 0 != (e=t_get_owner( tid, &owner )) )
    {
        printf("pvm_backtrace - t_get_owner failed!\n");
        goto nope;
    }

    if( 0 == owner )
    {
        printf("pvm_backtrace - owner == 0!\n");
        goto nope;
    }

    pvm_object_storage_t *_ow = owner;

    struct data_area_4_thread *tda = (struct data_area_4_thread *)&_ow->da;

    if( _ow->_class != pvm_get_thread_class() )
    {
        printf("pvm_backtrace - not thread in owner!\n");
        return;
    }

    if(tda->tid != tid)
    {
        printf("pvm_backtrace VM thread TID doesn't match!\n");
        return;
    }

    pvm_backtrace(tda);
    return;

    nope:
        printf("Unable to print backtrace, e=%d\n", e);
}


void pvm_backtrace(struct data_area_4_thread *tda)
{
    struct pvm_code_handler *code = &tda->code;

    if(code->IP > code->IP_max)
    {
        printf("pvm_backtrace IP > IP_Max!\n");
        return;
    }

    printf("pvm_backtrace thread IP %d\n", code->IP);

    printf("pvm_backtrace thread this:\n");
    pvm_object_dump(tda->_this_object);
    printf("\n\n");

    pvm_object_t sframe = tda->call_frame;

    while( !pvm_is_null(sframe) )
    {
        //if( !pvm_object_class_is(sframe, pvm_get_stack_frame_class()) ) ??!

        struct data_area_4_call_frame *fda = pvm_object_da(sframe,call_frame);

        printf("pvm_backtrace frame:\n");
        pvm_object_dump(sframe);
        printf("\n");

        printf("pvm_backtrace frame this:\n");
        pvm_object_t thiso = fda->this_object;
        pvm_object_dump(thiso);
        printf("\n");

        pvm_object_t tclass = thiso->_class;
        int ord = fda->ordinal;

        printf("pvm_backtrace frame IP: %d Method ordinal %d\n", fda->IP, ord );

        int lineno = pvm_ip_to_linenum(tclass, ord, fda->IP);
        if( lineno >= 0 )
        {
            pvm_object_t mname = pvm_get_method_name( tclass, ord );

            pvm_object_print(mname);
            printf(":%d\n", lineno);
        }

        printf("\n\n");
        sframe = fda->prev;
    }

}



void pvm_trace_here(struct data_area_4_thread *tda)
{
    struct pvm_code_handler *code = &tda->code;

    if(code->IP > code->IP_max)
    {
        printf("pvm_trace_here IP > IP_Max!\n");
        return;
    }


    //printf("pvm_trace_here thread this:\n");
    printf("%% ");
#if 1
    pvm_object_t cn = pvm_get_class_name( tda->_this_object );
    pvm_object_print( cn );
#else
    pvm_object_dump(tda->_this_object);
    //printf("\n\n");
#endif
    //printf("pvm_trace_here thread IP %d\n", code->IP);

    pvm_object_t sframe = tda->call_frame;

    if( pvm_is_null(sframe) )
    {
        printf("pvm_trace_here call frame == 0\n");
        return;
    }

    //if( !pvm_object_class_is(sframe, pvm_get_stack_frame_class()) ) ??!

    struct data_area_4_call_frame *fda = pvm_object_da(sframe,call_frame);

    //printf("pvm_backtrace frame:\n");    pvm_object_dump(sframe);    printf("\n");

    pvm_object_t thiso = fda->this_object;

    //printf("pvm_backtrace frame this:\n");    pvm_object_dump(thiso);    printf("\n");

    pvm_object_t tclass = thiso->_class;
    int ord = fda->ordinal;
/*
    printf("pvm_backtrace frame IP: %d Method ordinal %d\n", fda->IP, ord );

    int lineno = pvm_ip_to_linenum(tclass, ord, fda->IP);
    if( lineno >= 0 )
    {
        pvm_object_t mname = pvm_get_method_name( tclass, ord );

        pvm_object_print(mname);
        printf(":%d\n", lineno);
    }
*/
    printf("\tordinal %d", ord );
    printf("\tIP %d ", code->IP);

    printf("\n");

}




int pvm_ip_to_linenum(pvm_object_t tclass, int method_ordinal, int ip)
{
    if(!pvm_object_class_exactly_is( tclass, pvm_get_class_class() ))
    {
        printf("pvm_ip_to_linenum: not a class\n");
        return -1;
    }

    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t ip2line_maps = cda->ip2line_maps;
    //cda->method_names = method_names;

    pvm_object_t map = pvm_get_ofield( ip2line_maps, method_ordinal );

    struct data_area_4_binary * bin = pvm_object_da( map, binary );

    int nrecords = (map->_da_size)/sizeof(struct vm_code_linenum);

    struct vm_code_linenum *sp = (void *)bin->data;



    int i;
    for( i = 0; i < nrecords; i++)
    {
        // TODO bin search, array must be sorted
        if( sp[i].ip >= ip )
        {
            if( i == 0 )
                return -1;

            return sp[i-1].line;
        }
    }

    return -1;
}







pvm_object_t pvm_get_method_name( pvm_object_t tclass, int method_ordinal )
{
    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t mnames = cda->method_names;

    if( pvm_is_null(mnames))
        return pvm_get_null_object();

    pvm_object_t name = pvm_get_ofield( mnames, method_ordinal );
    return name;
}

int pvm_get_method_name_count( pvm_object_t tclass )
{
    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t mnames = cda->method_names;

    if( pvm_is_null(mnames))
        return 0;

    return get_array_size( mnames );
}

// returns ord or -1
int pvm_get_method_ordinal( pvm_object_t tclass, pvm_object_t mname )
{
    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t mnames = cda->method_names;

    if( pvm_is_null(mnames) )
        return -1;

    if( pvm_is_null(mname) )
        return -1;

    if( !pvm_object_class_exactly_is( mname, pvm_get_string_class() ) )
        return -1;


    int nitems = get_array_size( mnames );
    int i;

    for( i = 0; i < nitems; i++ )
    {
        pvm_object_t curr_mname = pvm_get_ofield( mnames, i );

        int diff = pvm_strcmp( curr_mname, mname);
        if( diff == 0 )
            return i;
    }

    return -1;
}







pvm_object_t pvm_get_field_name( pvm_object_t tclass, int ordinal )
{
    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t fnames = cda->field_names;

    if( pvm_is_null(fnames))
        return pvm_get_null_object();

    pvm_object_t name = pvm_get_ofield( fnames, ordinal );
    return name;
}


int pvm_get_field_name_count( pvm_object_t tclass )
{
    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t fnames = cda->field_names;

    if( pvm_is_null(fnames))
        return 0;

    return get_array_size( fnames );
}









pvm_object_t pvm_get_class( pvm_object_t o )
{
    if( pvm_is_null(o))
        return o;

    return o->_class;
}
