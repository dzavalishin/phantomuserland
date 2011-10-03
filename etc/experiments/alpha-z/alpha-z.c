#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "alpha-z.h"

extern void mmx_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos)
{
    const int32x2 acmp = {
        0x00ffffff,
        0x00ffffff,
    };
    const int32x2 zcmp = {
        zpos, zpos,
    };

    while (nelem >= 2) {
        int32x2 alpha_mask = __builtin_ia32_pcmpgtd(*(int32x2*)src, acmp);
        int32x2 z_mask = __builtin_ia32_pcmpgtd(*(int32x2*)zb, zcmp);
        int32x2 mask = alpha_mask & z_mask;
        *(int32x2*)dest = (*(int32x2*)src & mask) | (*(int32x2*)dest & ~mask);
        *(int32x2*)zb = (zcmp & mask) | (*(int32x2*)zb & ~mask);

        src += 2;
        dest += 2;
        zb += 2;
        nelem -= 2;
    }
}

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

void sseu_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos)
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
    assert(((intptr_t)zb & 15) == 0);

    while (nelem >= 4) {
        int32x4 srcv;
        memcpy(&srcv, src, sizeof(srcv));
        int32x4 alpha_mask = __builtin_ia32_pcmpgtd128(srcv, acmp);
        int32x4 z_mask = __builtin_ia32_pcmpgtd128(*(int32x4*)zb, zcmp);
        int32x4 mask = alpha_mask & z_mask;
        *(int32x4*)dest = (srcv & mask) | (*(int32x4*)dest & ~mask);
        *(int32x4*)zb = (zcmp & mask) | (*(int32x4*)zb & ~mask);

        src += 4;
        dest += 4;
        zb += 4;
        nelem -= 4;
    }
}

void rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos)
{   
    while(nelem-- > 0)
        if(src->a)
        {   
            if( *zb > zpos ) { zb++; dest++; src++; continue; }
            *zb++ = zpos;

            *dest++ = *src++;
        }
        else
        {   
            dest++;
            src++;
            zb++;
        }
}

void auto_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos)
{
    unsigned n1 = (intptr_t)dest & 15;

    if ((intptr_t)zb & 15 != n1) {
        rgba2rgba_zbmove(dest, src, zb, nelem, zpos);
    } else {
        if (n1) {
            n1 = nelem < (16 - n1) ? nelem : (16 - n1);
            rgba2rgba_zbmove(dest, src, zb, n1, zpos);
            dest += n1;
            src += n1;
            zb += n1;
            nelem -= n1;
        }

        if (nelem) {
            unsigned n2 = nelem & ~3;

            if ((intptr_t)src & 15) {
                sseu_rgba2rgba_zbmove(dest, src, zb, n2, zpos);
            } else {
                sse_rgba2rgba_zbmove(dest, src, zb, n2, zpos);
            }
            dest += n1;
            src += n1;
            zb += n1;
            nelem -= n2;
        }

        if (nelem) {
            rgba2rgba_zbmove(dest, src, zb, nelem, zpos);
        }
    }
}
