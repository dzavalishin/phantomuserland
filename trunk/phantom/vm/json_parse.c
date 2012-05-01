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
