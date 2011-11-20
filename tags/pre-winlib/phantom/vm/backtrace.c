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

#include <vm/exec.h>
#include <vm/code.h>
#include <vm/alloc.h>
#include <vm/reflect.h>

#include <threads.h>
#include <thread_private.h>

//static void pvm_backtrace(void);

//pvm_object_t pvm_get_method_name( pvm_object_t tclass, int method_ordinal );
//int pvm_ip_to_linenum(pvm_object_t tclass, int method_ordinal, int ip);



/* Poor man's exceptions */
void pvm_exec_panic( const char *reason )
{
    // TO DO: longjmp?
    //panic("pvm_exec_throw: %s", reason );
    //syslog()
    printf("pvm_exec_panic: %s", reason );
    pvm_backtrace_current_thread();

    pvm_memcheck();
    hal_exit_kernel_thread();
}


void pvm_backtrace_current_thread(void)
{
    errno_t e = ENOENT;
    int tid = get_current_tid();
    if( tid < 0 ) goto nope;

    void *owner;
    if( 0 != (e=t_get_owner( tid, &owner )) )
        goto nope;

    if( 0 == owner )
        goto nope;

    pvm_object_storage_t *_ow = owner;

    struct data_area_4_thread *tda = (struct data_area_4_thread *)&_ow->da;

    if( _ow->_class.data != pvm_get_thread_class().data )
    {
        printf("pvm_backtrace - not thread in owner!\n");
        return;
    }

    if(tda->tid != tid)
    {
        printf("pvm_backtrace VM thread TID doesn't match!\n");
        return;
    }

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

    printf("pvm_backtrace thread this:\n");
    pvm_object_dump(tda->_this_object);

    pvm_object_t sframe = tda->call_frame;

    while( !pvm_is_null(sframe) )
    {
        //if( !pvm_object_class_is(sframe, pvm_get_stack_frame_class()) ) ??!

        struct data_area_4_call_frame *fda = pvm_object_da(sframe,call_frame);

        printf("pvm_backtrace frame:\n");
        pvm_object_dump(sframe);
        printf("pvm_backtrace frame this:\n");
        pvm_object_t thiso = fda->this_object;
        pvm_object_dump(thiso);
        printf("pvm_backtrace frame IP: %d\n", fda->IP);

        pvm_object_t tclass = thiso.data->_class;
        int ord = fda->ordinal;

        int lineno = pvm_ip_to_linenum(tclass, ord, fda->IP);
        pvm_object_t mname = pvm_get_method_name( tclass, ord );

        pvm_object_print(mname);
        printf(":%d\n", lineno);

        printf("\n\n");
        sframe = fda->prev;
    }

}


int pvm_ip_to_linenum(pvm_object_t tclass, int method_ordinal, int ip)
{
    if(!pvm_object_class_is( tclass, pvm_get_class_class() ))
    {
        printf("pvm_ip_to_linenum: not a class\n");
        return 0;
    }

    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t ip2line_maps = cda->ip2line_maps;
    //cda->method_names = method_names;

    pvm_object_t map = pvm_get_ofield( ip2line_maps, method_ordinal );

    struct data_area_4_binary * bin = pvm_object_da( map, binary );

    int nrecords = (map.data->_da_size)/sizeof(struct vm_code_linenum);

    struct vm_code_linenum *sp = (void *)bin->data;



    int i;
    for( i = 0; i < nrecords; i++)
    {
        // TODO bin search, array must be sorted
        if( sp[i].ip >= ip )
        {
            if( i == 0 )
                return 0;

            return sp[i-1].line;
        }
    }

    return 0;
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

pvm_object_t pvm_get_field_name( pvm_object_t tclass, int ordinal )
{
    struct data_area_4_class *cda= pvm_object_da( tclass, class );
    pvm_object_t fnames = cda->field_names;

    if( pvm_is_null(fnames))
        return pvm_get_null_object();

    pvm_object_t name = pvm_get_ofield( fnames, ordinal );
    return name;
}

