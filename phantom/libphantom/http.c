/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * HTTP related.
 *
 *
**/


#define DEBUG_MSG_PREFIX "http"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 1
#define debug_level_info 0



#include <kernel/net.h>


const char * http_skip_header( const char *buf )
{
    int newline = 0;

    for( ; *buf; buf++ )
    {
        if( *buf == '\r' ) buf++;

        if( *buf == '\n' ) 
        {
            newline++;
            if( newline > 1) return buf+1;
        }
        else
        {
            newline = 0;
        }        
    }

    return "";
}


