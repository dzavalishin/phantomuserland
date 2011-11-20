/* 
** Copyright 2004, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <string.h>

size_t
strxfrm(char *dest, const char *src, size_t n)
{
	size_t len = strlen(src);

	if(n) {
		size_t copy_len = len < n ? len : n - 1;
		memcpy(dest, src, copy_len);
		dest[copy_len] = 0;
	}
	return len;
}

