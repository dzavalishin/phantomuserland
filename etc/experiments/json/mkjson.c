#include <stdio.h>
#include "json.h"

static inline char hex_nibble( char c ) { return (c >= 10) ? 'A' + c - 10 : '0' + c; }

int _out( char c )
{
    putchar( c );
}

json_output jo =
{
    _out,
    0
};

// TODO UTF-8

void json_encode_string( json_output *jo, const char *in)
{
    jo->putc( '"' );

    while( *in )
    {
        const char c = *in++;

        if(c == 0)
            break;

        switch (c)
        {

        case '\\':
        case '"':
        case '/':
            jo->putc( '\\' );
            jo->putc( c );
            break;

        case '\b':
            jo->putc( '\\' );
            jo->putc( 'b' );
            break;

        case '\t':
            jo->putc( '\\' );
            jo->putc( 't' );
            break;

        case '\n':
            jo->putc( '\\' );
            jo->putc( 'n' );
            break;

        case '\f':
            jo->putc( '\\' );
            jo->putc( 'f' );
            break;

        case '\r':
            jo->putc( '\\' );
            jo->putc( 'r' );
            break;

        default:
            if (c < ' ')
            {
                jo->putc( '\\' );
                jo->putc( 'u' );
                hex_nibble( 0x0F & (c>>4) );
                hex_nibble( 0x0F & c );
            }
            else
            {
                jo->putc( c );
            }
        }
    }

    jo->putc( '"' );
}

void json_encode_int( json_output *jo, long v)
{
    // TODO loop me
    if( v > 10 ) json_encode_int( jo, v/10 );
    jo->putc( (v%10) + '0' );
}


void json_put_name( json_output *jo, const char *name )
{
    int d = jo->depth + 1;

    while( d-- )     jo->putc( '\t' );

    json_encode_string( jo, name );
    jo->putc( ':' );
    jo->putc( ' ' );
}




void json_out_int( json_output *jo, const char *name, int value )
{
    json_put_name( jo, name );
    json_encode_int( jo, value );
}

void json_out_long( json_output *jo, const char *name, long value )
{
    json_put_name( jo, name );
    json_encode_int( jo, value );
}

void json_out_string( json_output *jo, const char *name, const char * value )
{
    json_put_name( jo, name );
    json_encode_string( jo, value );
}


void json_out_int_array( json_output *jo, const char *name, int *value );
void json_out_string_array( json_output *jo, const char *name, const char * value );

void json_out_open_struct( json_output *jo, const char *name )
{
    jo->depth++;
    json_put_name( jo, name );
    jo->putc( '{' );
    jo->putc( '\n' );
}
void json_out_open_anon_struct( json_output *jo )
{
    jo->depth++;
    jo->putc( '{' );
    jo->putc( '\n' );
}

void json_out_close_struct( json_output *jo  )
{
    jo->putc( '}' );
    jo->putc( '\n' );
    jo->depth--;
}

void json_out_open_array( json_output *jo, const char *name )
{
    jo->depth++;
    json_put_name( jo, name );
    jo->putc( '[' );
    jo->putc( '\n' );
}

void json_out_close_array( json_output *jo )
{
    jo->putc( ']' );
    jo->putc( '\n' );
    jo->depth--;
}


void json_start( json_output *jo )
{
    jo->depth = 0;
    jo->putc( '{' );
    jo->putc( '\n' );
}

void json_stop( json_output *jo )
{
    if( jo->depth )
        printf("JSON non-balanced\n");
    jo->putc( '\n' );
    jo->putc( '}' );
    jo->putc( '\n' );
}

void json_out_delimiter( json_output *jo )
{
    jo->putc( ',' );
    jo->putc( '\n' );
}


int main( int ac, char **av )
{
    json_start( &jo );

    //jenc("hello \"world\"");

    json_out_open_array( &jo, "threads" );

    //json_out_open_struct( &jo, "thread" );
    json_out_open_anon_struct( &jo );

    json_out_string( &jo, "name", "Init" );
    json_out_delimiter( &jo );
    json_out_int( &jo, "tid", 1 );

    json_out_close_struct( &jo );

    json_out_delimiter( &jo );

    //json_out_open_struct( &jo, "thread" );
    json_out_open_anon_struct( &jo );

    json_out_string( &jo, "name", "DPC Runner" );
    json_out_delimiter( &jo );
    json_out_int( &jo, "tid", 2 );

    json_out_close_struct( &jo );



    json_out_close_array( &jo );

    json_stop( &jo );

    return 0;
}

