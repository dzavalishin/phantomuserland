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

#include "vm/exec.h"
#include "vm/code.h"
#include "vm/alloc.h"

#include <threads.h>
#include <thread_private.h>

static void pvm_backtrace(void);

/* Poor man's exceptions */
void pvm_exec_panic( const char *reason )
{
	// TO DO: longjmp?
	//panic("pvm_exec_throw: %s", reason );
	//syslog()
	printf("pvm_exec_panic: %s", reason );
	pvm_backtrace();
	pvm_memcheck();
	hal_exit_kernel_thread();
}

static void pvm_backtrace(void)
{
	phantom_thread_t *t = get_current_thread();
	if( t == 0 ) goto nope;

	if(!(t->thread_flags & THREAD_FLAG_VM))
	{
		printf("pvm_backtrace called for non-VM thread!\n");
		return;
	}

	pvm_object_storage_t *_ow = get_thread_owner( t );
	if( _ow == 0 ) goto nope;

	struct data_area_4_thread *tda = (struct data_area_4_thread *)&_ow->da;

	// todo check class

	if(tda->tid != t->tid)
	{
		printf("pvm_backtrace VM thread TID doesn't match!\n");
		return;
	}

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
		struct data_area_4_call_frame *fda = pvm_object_da(sframe,call_frame);

		printf("pvm_backtrace frame:\n");
		pvm_object_dump(sframe);
		printf("pvm_backtrace frame this:\n");
		pvm_object_dump(fda->this_object);
		printf("pvm_backtrace frame IP: %d\n", fda->IP);

		sframe = fda->prev;
	}

	return;

	nope:
	printf("Unable to print backtrace\n");
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

	struct vm_code_linenum *sp = bin->data;



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
}


