#include <kernel/json.h>
#include <stdio.h>

void json_dump_token(jsmntok_t * t, const char *full_string)
{
			jsmntok_t tt = *t;

			const char *type = "?";
			switch(tt.type)
			{
			case JSMN_UNDEFINED: type = "undef"; break;
			case JSMN_OBJECT:	 type = "object"; break;
			case JSMN_ARRAY:	 type = "array"; break;
			case JSMN_STRING: 	 type = "string"; break;
			case JSMN_PRIMITIVE: type = "primitive"; break;				
			}

			printf( 
				"%s: '%.*s' parent %d pos %d end %d size %d\n", 
					type,
					tt.end-tt.start, 
					full_string+tt.start, 
					tt.parent,
					tt.start, tt.end, tt.size
					 );


}

void json_dump(jsmntok_t * tokens, size_t count, const char *full_string)
{
		jsmntok_t *t = tokens;
		while(count-- > 0)
		{
			json_dump_token(t++, full_string);
		}
	
}

//! tokens - array of tokens to scan in
//! count - size of tokens
//! parent_token - token we are children of
//! level - descend level
//! size - number of children we're supposed to find
//!
//! returns number of tokens processed
size_t json_descent( 
	jsmntok_t *tokens, size_t count, 
	jsmntok_t *parent_token, int level, int size, int parent,
	//int shift, // what pos we start at
	const char *full_string )
{
	size_t done = 0;

	while( size-- > 0 )
	{
		int i = level;
		while(i--) printf("\t");
		json_dump_token(tokens,full_string);
		/* incorrect */
		if( tokens->parent != parent )
		{
			printf("given parent %d, tokens->parent %d\n", parent, tokens->parent );
		} /* */

		if( tokens->size )
		{
			// have children
			size_t rc = json_descent( 
				tokens+1, count-done, 
				tokens+0, level+1, tokens->size, 
				parent+done+1,
				full_string );
			done += rc;
			tokens += rc;
		}

		done++;
		tokens++;
	}

	return done;
}

void parse_and_dump( const char * j1 )
{
	errno_t rc;
	jsmntok_t *tokens;
	size_t count = 0;

	rc = json_parse( j1, &tokens, &count );
	if( rc )
		printf("rc = %d for '%s'\n", rc, j1 );
	else
	{
		printf("parsed '%s', %d tokens\n\n", j1, count );
		json_dump(tokens,count,j1);
		printf("\n\n");
		json_descent( tokens+1, count-1, tokens+0, 0, tokens[0].size, 0, j1 );
		free(tokens);
	}
}

void pvm_json_test()
{
	sleep(1);
	printf("\n");

	const char j1[] = "{ \"a\" : \"hello\", \"b\" : 122, \"c\" : [ \"c1\", \"c2\" ] }";
	//const char j1[] = "{ \"a\":[\"hello\",\"goodbye\"] }";
	//const char j1[] = " \"a\" : \"hello\" ";
	parse_and_dump(j1);

}

#if 0
#include <vm/object.h>
#include <vm/p2c.h>
#include <vm/reflect.h>

static char *ep; // TODO killme

static const char *parse_string(char **item, const char *str);

/*

// TODO need some cache/hash or will be incredibly slow
static errno_t vm_field_ordinal_by_name( int *out, pvm_object_t oclass, const char *name )
{
	if( out == 0 )
		return ENOMEM;

	int sz = pvm_get_field_name_count( oclass );

	int ordinal = 0;
	for( ordinal = 0; ordinal < sz; ordinal++ )
	{
		pvm_object_t nm = pvm_get_field_name( oclass, ordinal );

		if( pvm_is_null(nm) || (!IS_PHANTOM_STRING(nm)))
			continue;

		if( EQ_STRING_P2C(nm,name) )
		{
			*out = ordinal;
			return 0;
		}
	}

	return ENOENT;

}

*/

static errno_t get_child( pvm_object_t *out, pvm_object_t parent, const char *name )
{
	if( pvm_is_null(parent) )
		return ENOTDIR;

	pvm_object_t oc = pvm_get_class( parent );


	int ord;
	errno_t rc = vm_field_ordinal_by_name( &ord, oc, name );

	int as = get_array_size(parent.data);


}

errno_t vm_json_parse( struct json_parse_context *jpc, pvm_object_t root, const char *value )
{

	if (*value!='{')	{ep=value;return 0;}	/* not an object! */

	//item->type=cJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	/* empty array. */

	//item->child=child=cJSON_New_Item();	if (!item->child) return 0;
	char *item_name;
	value=skip(parse_string( &item_name,skip(value)));
	if (!value) return 0;

	//child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') {ep=value;return 0;}	/* fail! */
	value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
	if (!value) return 0;

	while (*value==',')
	{
		cJSON *new_item;
		if (!(new_item=cJSON_New_Item()))	return 0; /* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_string(child,skip(value+1)));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') {ep=value;return 0;}	/* fail! */
		value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
		if (!value) return 0;
	}

	if (*value=='}') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */


}








/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(char **item, const char *str)
{
	const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
	if (*str!='\"') {ep=str;return 0;}	/* not a string! */

	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */

	out=(char*)malloc(len+1);	/* This is how long we need for the string, roughly. */
	if (!out) return 0;


	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
			case 'b': *ptr2++='\b';	break;
			case 'f': *ptr2++='\f';	break;
			case 'n': *ptr2++='\n';	break;
			case 'r': *ptr2++='\r';	break;
			case 't': *ptr2++='\t';	break;
			case 'u':	 /* transcode utf16 to utf8. */
				sscanf(ptr+1,"%4x",&uc);ptr+=4;	/* get the unicode char. */

				if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	// check for invalid.

				if (uc>=0xD800 && uc<=0xDBFF)	// UTF16 surrogate pairs.
				{
					if (ptr[1]!='\\' || ptr[2]!='u')	break;	// missing second-half of surrogate.
					sscanf(ptr+3,"%4x",&uc2);ptr+=6;
					if (uc2<0xDC00 || uc2>0xDFFF)		break;	// invalid second-half of surrogate.
					uc=0x10000 | ((uc&0x3FF)<<10) | (uc2&0x3FF);
				}

				len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;

				switch (len) {
				case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
				case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
				case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
				case 1: *--ptr2 =(uc | firstByteMark[len]);
				}
				ptr2+=len;
				break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;

	if(item)
		*item=out;

	return ptr;
}




#endif
