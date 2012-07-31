/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tests - crypto
 *
 *
**/

#define DEBUG_MSG_PREFIX "test"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <kernel/config.h>
#include <phantom_libc.h>
#include <errno.h>
#include <assert.h>
#include "test.h"

#include <kernel/crypt/base64.h>
#include <kernel/crypt/md5.h>
#include <kernel/crypt/xtea.h>



int do_test_crypt(const char *test_parm)
{
    (void) test_parm;

    const char *str  = "b64 test string!"; // must be mult of 8 bytes for xtea
    const char *pass = "hell"; // 4 bytes

    {
        SHOW_FLOW0( 1, "base64" );

        char *enc = encode_base64(str);
        char *dec = decode_base64(enc);

        test_check_false( strcmp( str, dec ) );
        free( enc );
    }

#if 1
    {
        SHOW_FLOW0( 1, "md5" );

        const char *str = "big brown fox jumps over the lazy dog";
        const char md5[] = {0xb8, 0x0e, 0xf7, 0xf0, 0x46, 0x47, 0xc1, 0xfb, 0x06, 0x42, 0x45, 0xb3, 0x2d, 0x98, 0x4b, 0xea};

        u_int8_t digest[16];

        MD5CONTEXT context;

        MD5_Init  ( &context );
        MD5_Update( &context, (const u_int8_t *)str, strlen(str) );
        MD5_Final ( &context, digest );

        int res = memcmp( digest, md5, 16 );
        if( res )
            test_fail_msg(-1,"md5");
    }
#endif

    {
        SHOW_FLOW0( 1, "xtea" );

        char enc[128];
        char dec[128];
        int l = strlen(str);
        assert( !(l%8) );

        XTeaCryptStr  ( enc, str, l, pass);
        XTeaDecryptStr( dec, enc, l, pass);

        test_check_false( strncmp( str, dec, l ) );
    }

#if 0
    // tested above
    {
        u_int32_t enc[128];
        u_int32_t dec[128];
        XTeaKeyBlock_t k = { 1, 2, 3, 4 };

        XTeaCrypt  ( enc,  const u_int32_t *v, const XTeaKeyBlock_t k);
        XTeaDecrypt(u_int32_t *w, const u_int32_t *v, const XTeaKeyBlock_t k);
    }
#endif

    return 0;
}

