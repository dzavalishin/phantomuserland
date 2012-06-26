#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS Team
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Bitblt bits - SSE versions
 *
 *
**/

#include "drv_video_screen.h"
#include <assert.h>
#include <sys/types.h>
#include <video/color.h>
#include <video/zbuf.h>

typedef int32_t int32x2 __attribute__((vector_size(8)));
typedef int32_t int32x4 __attribute__((vector_size(16)));
typedef uint32_t uint32x4 __attribute__((vector_size(16)));



void sse_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos)
{
    const int32x4 acmp = {
        0x00ffffff,
        0x00ffffff,
        0x00ffffff,
        0x00ffffff,
    };
    const int32x4 zcmp = {
        zpos, zpos, zpos, zpos,
    };

    assert(((intptr_t)dest & 15) == 0);
    assert(((intptr_t)src & 15) == 0);
    assert(((intptr_t)zb & 15) == 0);

    while (nelem >= 4) {
        int32x4 alpha_mask = __builtin_ia32_pcmpgtd128(*(int32x4*)src, acmp);
        int32x4 z_mask = __builtin_ia32_pcmpgtd128(*(int32x4*)zb, zcmp);
        int32x4 mask = alpha_mask & z_mask;
        *(int32x4*)dest = (*(int32x4*)src & mask) | (*(int32x4*)dest & ~mask);
        *(int32x4*)zb = (zcmp & mask) | (*(int32x4*)zb & ~mask);

        src += 4;
        dest += 4;
        zb += 4;
        nelem -= 4;
    }
}

#endif
