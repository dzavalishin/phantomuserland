/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * JSON generator.
 *
 *
**/

#include <kernel/json.h>
#include <kunix.h> // default write func
#include <stdio.h> // default write func


static inline char hex_nibble( char c )
{
    return (c >= 10) ? 'A' + c - 10 : '0' + c;
}


// TODO UTF-8

void json_encode_string( json_output *jo, const char *in)
{
    jo->putc( jo, '"' );

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
            jo->putc( jo, '\\' );
            jo->putc( jo, c );
            break;

        case '\b':
            jo->putc( jo, '\\' );
            jo->putc( jo, 'b' );
            break;

        case '\t':
            jo->putc( jo, '\\' );
            jo->putc( jo, 't' );
            break;

        case '\n':
            jo->putc( jo, '\\' );
            jo->putc( jo, 'n' );
            break;

        case '\f':
            jo->putc( jo, '\\' );
            jo->putc( jo, 'f' );
            break;

        case '\r':
            jo->putc( jo, '\\' );
            jo->putc( jo, 'r' );
            break;

        default:
            if (c < ' ')
            {
                jo->putc( jo, '\\' );
                jo->putc( jo, 'u' );
                jo->putc( jo, hex_nibble( 0x0F & (c>>4) ) );
                jo->putc( jo, hex_nibble( 0x0F & c ) );
            }
            else
            {
                jo->putc( jo, c );
            }
        }
    }

    jo->putc( jo, '"' );
}

void json_encode_int( json_output *jo, unsigned long v)
{
    // TODO loop me
    if( v > 10 ) json_encode_int( jo, v/10 );
    jo->putc( jo, (v%10) + '0' );
}


void json_put_tabs( json_output *jo )
{
    int d = jo->depth;
    while( d-- )
        jo->putc( jo, '\t' );
}

void json_put_name( json_output *jo, const char *name )
{
    json_put_tabs( jo );
    json_encode_string( jo, name );
    jo->putc( jo, ':' );
    jo->putc( jo, ' ' );
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


void json_out_open_struct( json_output *jo, const char *name )
{
    jo->depth++;
    json_put_name( jo, name );
    jo->putc( jo, '{' );
    jo->putc( jo, '\n' );
}

void json_out_open_anon_struct( json_output *jo )
{
    jo->depth++;
    json_put_tabs( jo );
    jo->putc( jo, '{' );
    jo->putc( jo, '\n' );
}

void json_out_close_struct( json_output *jo  )
{
    jo->putc( jo, '\n' );
    json_put_tabs( jo );
    jo->putc( jo, '}' );
    //jo->putc( jo, '\n' );
    jo->depth--;
}

void json_out_open_array( json_output *jo, const char *name )
{
    jo->depth++;
    json_put_name( jo, name );
    jo->putc( jo, '[' );
    jo->putc( jo, '\n' );
}

void json_out_close_array( json_output *jo )
{
    jo->putc( jo, '\n' );
    json_put_tabs( jo );
    jo->putc( jo, ']' );
    //jo->putc( jo, '\n' );
    jo->depth--;
}


void json_start( json_output *jo )
{
    jo->depth = 0;
    jo->errno = 0;

    if( jo->putc == 0 )
    {
        jo->putc = json_putc_console;
        printf("JSON putc is missing\n");
    }

    jo->putc( jo, '{' );
    jo->putc( jo, '\n' );
}

void json_stop( json_output *jo )
{
    if( jo->depth )
        printf("JSON non-balanced\n");
    jo->putc( jo, '\n' );
    jo->putc( jo, '}' );
    jo->putc( jo, '\n' );

    if( jo->errno )
        printf("JSON errno %d\n", jo->errno );
}

void json_out_delimiter( json_output *jo )
{
    //json_put_tabs( jo );
    jo->putc( jo, ',' );
    jo->putc( jo, '\n' );
}


void json_foreach( json_output *jo, const char *name, void *array, size_t el_size, size_t count, void (*encoder)( json_output *jo, void *el ) )
{
    int i;

    json_out_open_array( jo, name );

    for( i = 0; i < count; i++ )
    {
        if( i > 0 )
            json_out_delimiter( jo );

        json_out_open_anon_struct( jo );
        encoder( jo, array+(i*el_size) );
        json_out_close_struct( jo );
    }
    json_out_close_array( jo );
}


void json_out_int_array( json_output *jo, const char *name, int *value, size_t count );

void json_out_string_array( json_output *jo, const char *name, const char * value, size_t count );






//! Default output function, assumes that jo->putc_arg contains kernel file descriptor for k_write()
void json_putc_kfd( json_output *jo, char c )
{
    int nwrite = 0;
    errno_t e = k_write( &nwrite, jo->putc_arg, (void *)&c, 1 );

    if( e )
        jo->errno = e;
    else if( nwrite != 1 )
        jo->errno = EIO;
}



//! Default output function, console
void json_putc_console( json_output *jo, char c )
{
    if( putchar( c ) < 0 )
        jo->errno = EIO;
}







