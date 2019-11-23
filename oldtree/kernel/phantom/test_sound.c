/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - sound output
 *
 *
**/

#define DEBUG_MSG_PREFIX "test.snd"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <phantom_libc.h>
#include <errno.h>
#include "test.h"

#include <kunix.h>
#include <sys/fcntl.h>

//#ifndef LOG_FLOW0
//#define LOG_FLOW0 SHOW_FLOW0
//#endif

int do_test_sound(const char *test_parm)
{
    (void) test_parm;

    int i, j, fd;
    errno_t rc;

    LOG_FLOW0( 0, "start sound test" );

    rc = k_open( &fd, "/dev/pci/es1370.0", O_RDONLY, 0 );
    test_check_false(rc);
    
    for( j = 0; j < 2048; j++ )
    {
        // Generate meander
        for( i = 0; i < 1024; i++ )
        {
            char buf[2];

            if( i > 512 )  buf[0] = buf[1] = 0xFF;
            else           buf[0] = buf[1] = 0;

            int nwr;
            rc = k_write( &nwr, fd, buf, sizeof buf );

            test_check_false(rc);
            test_check_eq( nwr, sizeof buf );
        }
    }

    rc = k_close( fd );
    test_check_false(rc);

    LOG_FLOW0( 0, "end sound test" );

    return 0;
}


