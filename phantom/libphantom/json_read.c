/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * JSON parser interface.
 *
**/


#include <kernel/json.h>
#include <jsmn.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <malloc.h>

//! tokens allocated, must be freed by caller
errno_t json_parse(const char *js, jsmntok_t **tokens )
{
	jsmn_parser parser;

	assert(tokens);
	assert(js);

	jsmn_init( &parser );

	int jslen = strlen(js);

	int count = jsmn_parse( &parser, js, jslen, 0, 0 );

	*tokens = calloc( count, sizeof(jsmntok_t) );
	if( !*tokens )
		return ENOMEM;

	if( count != jsmn_parse( &parser, js, jslen, *tokens, count ) )
		{
		free( *tokens );
		*tokens = 0;
		return EINVAL;
		}

	return 0;
}
