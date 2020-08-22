/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2008 Dmitry Zavalishin, dz@dz.ru
 *
 * Reflections and debugging.
 *
 *
**/

#ifndef REFLECT_H
#define REFLECT_H

#include <vm/object.h>
#include <vm/internal_da.h>

int             pvm_ip_to_linenum(pvm_object_t tclass, int method_ordinal, int ip);


int             pvm_get_method_name_count( pvm_object_t tclass );
pvm_object_t    pvm_get_method_name( pvm_object_t tclass, int method_ordinal );
int             pvm_get_method_ordinal( pvm_object_t tclass, pvm_object_t mname );



int             pvm_get_field_name_count( pvm_object_t tclass );
pvm_object_t    pvm_get_field_name( pvm_object_t tclass, int ordinal );


// JSON

struct json_parse_context
{
	
};


// -----------------------------------------------------------------------
// Debug


void pvm_backtrace_current_thread(void);
void pvm_backtrace(struct data_area_4_thread *tda);
void pvm_trace_here(struct data_area_4_thread *tda); // Print info about current program location




#endif // REFLECT_H


