#include <stdio.h>
//#include "json.h"

#include <kernel/json.h>

int _out( json_output *jo, char c )
{
    if( putchar( c ) < 0 )
        jo->errno = -1;
}

json_output jo =
{
    _out,
    0
};



struct test_struct
{
    int tid;
    char *name;
};



void json_out_int_array( json_output *jo, const char *name, int *value, size_t count );

void json_out_string_array( json_output *jo, const char *name, const char * value, size_t count );


struct test_struct test[] =
{
    { 1, "Init" },
    { 2, "DPC Runner" },
    { 3, "VM Thread" },
    { 4, "Snap" },
};


void encode_array( json_output *jo, void *el )
{
    struct test_struct *tp = el;

    json_out_string( jo, "name", tp->name );
    json_out_delimiter( jo );
    json_out_int( jo, "tid", tp->tid );
}


int main( int ac, char **av )
{
    json_start( &jo );

    //jenc("hello \"world\"");

    /*
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
    */

    json_foreach( &jo, "threads", test, sizeof(struct test_struct), sizeof(test)/sizeof(struct test_struct), encode_array );


    json_stop( &jo );

    return 0;
}

