/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Memory allocation.
 *
 *
**/

#ifndef MALLOC_H
#define MALLOC_H

#include <phantom_types.h>

void *malloc(size_t size);
void free(void *);

void *calloc(size_t n_elem, size_t elem_size);

//! align = n bytes to align to
void *calloc_aligned(size_t n_elem, size_t elem_size, size_t align);


//! FIXME TODO rewrite it asap! Shit!
void *realloc(void *ptr, size_t size);

#endif // MALLOC_H
