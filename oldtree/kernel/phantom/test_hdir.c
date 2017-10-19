/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - hdir, directory impl used in VM
 *
 *
**/

#define DEBUG_MSG_PREFIX "test.hdir"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <phantom_libc.h>
#include <errno.h>
#include "test.h"

// hdir
#include <vm/internal_da.h>


// TODO can't test - need object land to run

int do_test_hdir(const char *test_parm)
{
    (void) test_parm;
/*
    pvm_object_t d = pvm_create_directory_object();
    struct data_area_4_directory *da = pvm_object_da( d, directory );

    pvm_object_t out;

    test_check_false(pvm_is_null( d ));

    const char key1[] = "abc";

    // must fail
    errno_t rc = hdir_find( da, key1, sizeof(key1), &out, 0 );
    test_check_ne( rc, 0 );
*/

    return 0;
}

