/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <string.h>
#include <phantom_types.h>

size_t
strnlen(char const *s, size_t count)
{
	const char *sc;

	for(sc = s; count-- && *sc != '\0'; ++sc)
		;
	return sc - s;
}
