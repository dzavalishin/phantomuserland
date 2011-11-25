#if 0
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Asm ia32 blitter
 *
 *
**/

//#include <drv_video_screen.h>
#include <video/screen.h>
#include <assert.h>
#include <sys/types.h>


#ifndef ARCH_ia32

void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
{
#if 0

#if 1
    //xmm[imm8] = r/m32
    // PINSRD xmm1, r/m32, imm8

    // put 4 copies of zpos to 4 dword slots of xmm4
    __asm__ volatile("\
                     PINSRD 0, %0, %%xmm4; \
                     PINSRD 1, %0, %%xmm4; \
                     PINSRD 2, %0, %%xmm4; \
                     PINSRD 3, %0, %%xmm4; \
                     "
                     : : "r" (zpos) );
#else
    u_int32_t zp4[4];

    zp4[0] = zp4[1] = zp4[2] = zp4[3] = zpos;

    __asm__ volatile("movupd %0, %%xmm4"
                     : : "m" (zp4) );

#endif

    while(nelem > 4)
    {
        nelem -= 4;

        // if( xmm1 int32 > xmm2 int32 ) xmm1 int32 = 0xFFFFFFFF, else xmm1 int32 = 0
        //PCMPGTD xmm1, xmm2/m128

        // if( xmm0 byte & 0x80 ) xmm1 byte = xmm2 byte
        //PBLENDVB xmm1, xmm2/m128, <XMM0>

        // todo use moveap and ensure aligned to 16 byte mem addr for src/dest
        __asm__ volatile("\
                         movupd %2, %%xmm5 ;  /* src */ \
                         movupd %0, %%xmm6 ;  /* zb */ \
                         movapd %%xmm4, %%xmm0 ; /* xmm4 = zpos */ \
                         PCMPGTD %%xmm6, %%xmm0 ; /* zb */ \
                         PBLENDVB %%xmm5, %1 ; /* xmm5 -> dst, mask in xmm0 */ \
                         PBLENDVB %%xmm4, %0 ; /* xmm4 -> zb, mask in xmm0 */ \
			 "
                         : : "m" (*(__float128 *)zb), "m" (*(__float128 *)dest), "m" (*(__float128 *)src)  );


        dest += 4;
        src  += 4;
        zb   += 4;

    }
#endif
    while(nelem-- > 0)
    {
        if(src->a && (*zb <= zpos) )
        {
            *zb = zpos;
            *dest = *src;
        }

        dest++;
        src++;
        zb++;
    }
}


#endif // ARCH_ia32





#endif

