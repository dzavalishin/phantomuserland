/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * LZMA decompression lib interface.
 *
**/

#ifndef LZMA_H
#define LZMA_H

#include <errno.h>

errno_t plain_lzma_decode( void *dest, size_t *dest_len, void *src, size_t *src_len, int logLevel );

#endif // LZMA_H
