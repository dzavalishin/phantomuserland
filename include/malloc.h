#ifndef MALLOC_H
#define MALLOC_H

#include <phantom_types.h>

void *malloc(size_t size);
void free(void *);

void *calloc(size_t n_elem, size_t elem_size);
void *memalign(size_t alignment, size_t size);

// user tracks size
void *smalloc(size_t size);
void *smemalign(size_t alignment, size_t size);
void *scalloc(size_t size);
void *srealloc(void *buf, size_t old_size, size_t new_size);
void sfree(void *buf, size_t size);


#endif // MALLOC_H
