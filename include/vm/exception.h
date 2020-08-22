/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * VM exceptions
 *
 *
**/

#ifndef PVM_EXCEPTION_H
#define PVM_EXCEPTION_H


#include "vm/object.h"


struct pvm_exception_handler
{
    pvm_object_t   object;
    unsigned int   jump;
};


void pvm_exec_throw( const char *reason );


#endif // PVM_EXCEPTION_H

