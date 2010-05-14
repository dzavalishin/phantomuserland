#include <assert.h>
#include <malloc.h>
#include <string.h>


void *
calloc(size_t n_elem, size_t elem_size)
{
	size_t allocsize = n_elem * elem_size;

	void *ptr = malloc(allocsize);
	if (!ptr)
		return 0;

	memset(ptr, 0, allocsize);

	return ptr;
}

