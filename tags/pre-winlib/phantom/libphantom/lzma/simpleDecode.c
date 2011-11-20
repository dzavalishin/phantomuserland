/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Static constructors runner. General init/stop functions runner.
 *
 *
**/

#define DEBUG_MSG_PREFIX "lzma"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include "LzmaDec.h"
#include <errno.h>
#include <malloc.h>
#include <stdio.h>

#include <lzma.h>


static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc alloc = { SzAlloc, SzFree };

errno_t plain_lzma_decode( void *dest, size_t *dest_len, void *src, size_t *src_len, int logLevel )
{
    Byte 		propData[5];
    ELzmaStatus 	status;

    SRes rc = LzmaDecode( dest, dest_len, src, src_len,
                          propData, sizeof(propData), LZMA_FINISH_END,
                          &status, &alloc);

    switch(rc)
    {
    case SZ_OK: break;

    case SZ_ERROR_DATA:
        SHOW_ERROR0( logLevel, "Broken data" );
        return EFTYPE;

    case SZ_ERROR_MEM:
        SHOW_ERROR0( logLevel, "Out of mem" );
        return ENOMEM;

    case SZ_ERROR_UNSUPPORTED:
        SHOW_ERROR0( logLevel, "Unsupported props" );
        return EINVAL;

    case SZ_ERROR_INPUT_EOF:
        SHOW_ERROR0( logLevel, "Premature data end" );
        return ENOSPC;
    }


    switch(status)
    {
    case LZMA_STATUS_FINISHED_WITH_MARK: break;

    case LZMA_STATUS_NOT_SPECIFIED: // impossible
    case LZMA_STATUS_NEEDS_MORE_INPUT:
    case LZMA_STATUS_NOT_FINISHED:
    case LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK:
        SHOW_ERROR0( logLevel, "Premature data end" );
        return ENOSPC;
    }

    return 0;
}

