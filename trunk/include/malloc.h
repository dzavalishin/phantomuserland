#ifndef MALLOC_H
#define MALLOC_H

#include <phantom_types.h>

void *malloc(size_t size);
void free(void *);

void *calloc(size_t n_elem, size_t elem_size);

#endif // MALLOC_H
