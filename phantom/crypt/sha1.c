//  Support for Calculation of SHA1 in SW
//
//  Copyright (C) 2006-2011 IBM Corporation
//
//  Authors:
//      Stefan Berger <stefanb@linux.vnet.ibm.com>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.
//
//  See: http://www.itl.nist.gov/fipspubs/fip180-1.htm
//       RFC3174, Wikipedia's SHA1 alogrithm description
//

//#include "config.h"
//#include "byteorder.h" // cpu_to_*, __swab64
#include <compat/seabios/byteorder.h>
#include <kernel/crypt/sha1.h> // sha1
#include <string.h> // memcpy
//#include "x86.h" // rol

typedef struct _sha1_ctx {
    u_int32_t h[5];
} sha1_ctx;


#ifndef ARCH_ia32

static inline u_int32_t rol( u_int32_t x, u_int32_t n )
{
  return (x<<n) | (x>>(32-n));
}

#else

static inline u_int32_t rol( u_int32_t val, u_int16_t rol) {
    u_int32_t res;
    asm volatile("roll %%cl, %%eax"
                 : "=a" (res) : "a" (val), "c" (rol));
    return res;
}

#endif


static void
sha1_block(u_int32_t *w, sha1_ctx *ctx)
{
    u_int32_t i;
    u_int32_t a,b,c,d,e,f;
    u_int32_t tmp;
    u_int32_t idx;

    static const u_int32_t sha_ko[4] = {
        0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6 };

    /* change endianness of given data */
    for (i = 0; i < 16; i++)
        w[i] = be32_to_cpu(w[i]);

    for (i = 16; i <= 79; i++) {
        tmp = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
        w[i] = rol(tmp,1);
    }

    a = ctx->h[0];
    b = ctx->h[1];
    c = ctx->h[2];
    d = ctx->h[3];
    e = ctx->h[4];

    for (i = 0; i <= 79; i++) {
        if (i <= 19) {
            f = (b & c) | ((b ^ 0xffffffff) & d);
            idx = 0;
        } else if (i <= 39) {
            f = b ^ c ^ d;
            idx = 1;
        } else if (i <= 59) {
            f = (b & c) | (b & d) | (c & d);
            idx = 2;
        } else {
            f = b ^ c ^ d;
            idx = 3;
        }

        tmp = rol(a, 5) +
              f +
              e +
              sha_ko[idx] +
              w[i];
        e = d;
        d = c;
        c = rol(b, 30);
        b = a;
        a = tmp;
    }

    ctx->h[0] += a;
    ctx->h[1] += b;
    ctx->h[2] += c;
    ctx->h[3] += d;
    ctx->h[4] += e;
}


static void
sha1_do(sha1_ctx *ctx, const u_int8_t *data32, u_int32_t length)
{
    u_int32_t offset;
    u_int16_t num;
    u_int32_t bits = 0;
    u_int32_t w[80];
    u_int64_t tmp;

    /* treat data in 64-byte chunks */
    for (offset = 0; length - offset >= 64; offset += 64) {
        memcpy(w, data32 + offset, 64);
        sha1_block((u_int32_t *)w, ctx);
        bits += (64 * 8);
    }

    /* last block with less than 64 bytes */
    num = length - offset;
    bits += (num << 3);

    memcpy(w, data32 + offset, num);
    ((u_int8_t *)w)[num] = 0x80;
    if (64 - (num + 1) > 0)
        memset( &((u_int8_t *)w)[num + 1], 0x0, 64 - (num + 1));

    if (num >= 56) {
        /* cannot append number of bits here */
        sha1_block((u_int32_t *)w, ctx);
        memset(w, 0x0, 60);
    }

    /* write number of bits to end of block */
    tmp = __swab64(bits);
    memcpy(&w[14], &tmp, 8);

    sha1_block(w, ctx);

    /* need to switch result's endianness */
    for (num = 0; num < 5; num++)
        ctx->h[num] = cpu_to_be32(ctx->h[num]);
}


u_int32_t
sha1(const u_int8_t *data, u_int32_t length, u_int8_t *hash)
{

    sha1_ctx ctx = {
        .h[0] = 0x67452301,
        .h[1] = 0xefcdab89,
        .h[2] = 0x98badcfe,
        .h[3] = 0x10325476,
        .h[4] = 0xc3d2e1f0,
    };

    sha1_do(&ctx, data, length);
    memcpy(hash, &ctx.h[0], 20);

    return 0;
}
