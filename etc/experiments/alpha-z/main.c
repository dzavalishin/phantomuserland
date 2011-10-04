#include "alpha-z.h"
#include <stdio.h>

#define N_PIX (16384*1)
#define LOOP  (100000*2)

int main()
{
    volatile unsigned i;
    static struct rgba_t dest[N_PIX] = {0};
    static struct rgba_t src[N_PIX + 1] = {
        {1, 1, 1, 0},
        {1, 1, 1, 1},
        {1, 1, 1, 0},
        {1, 1, 1, 1},
    };
    static zbuf_t zb[N_PIX + 2] = {
        0,
        0,
        2,
        2,
    };
    for (i = 4; i < N_PIX; ++i) {
        src[i] = src[i - 4];
        zb[i] = zb[i - 4];
    }

#ifndef V
#define V 4
#endif

#if V == 0
        printf("rgba2rgba_zbmove\n");
#elif V == 1
        printf("mmx_rgba2rgba_zbmove\n");
#elif V == 2
        printf("sse_rgba2rgba_zbmove\n");
#elif V == 3
        printf("sseu_rgba2rgba_zbmove\n");
#elif V == 4
        printf("auto_rgba2rgba_zbmove\n");
#endif


    for (i = 0; i < LOOP; ++i) {
#if V == 0
        rgba2rgba_zbmove(dest, src + 1, zb, N_PIX, 1);
#elif V == 1
        //mmx_rgba2rgba_zbmove(dest, src + 1, zb, N_PIX, 1);
#elif V == 2
        sse_rgba2rgba_zbmove(dest, src, zb, N_PIX, 1);
#elif V == 3
        sseu_rgba2rgba_zbmove(dest, src + 1, zb, N_PIX, 1);
#elif V == 4
        auto_rgba2rgba_zbmove(dest, src + 1, zb, N_PIX, 1);
#endif
    }
    return 0;
}
