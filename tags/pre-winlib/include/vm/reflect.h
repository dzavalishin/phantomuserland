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

int pvm_ip_to_linenum(pvm_object_t tclass, int method_ordinal, int ip);


pvm_object_t pvm_get_method_name( pvm_object_t tclass, int method_ordinal );
pvm_object_t pvm_get_field_name( pvm_object_t tclass, int ordinal );



// -----------------------------------------------------------------------
// Debug


void pvm_backtrace_current_thread(void);
void pvm_backtrace(struct data_area_4_thread *tda);




#endif // REFLECT_H


