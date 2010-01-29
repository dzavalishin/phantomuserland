/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <string.h>
//#include <libc/ctype.h>

void *memscan(void *addr, int c, size_t size)
{
	unsigned char *p = (unsigned char *)addr;

	while(size) {
		if(*p == c)
			return (void *)p;
		p++;
		size--;
	}
  	return (void *)p;
}
