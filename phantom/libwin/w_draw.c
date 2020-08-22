/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - painting.
 *
**/



#include <video/window.h>
#include <video/internal.h>
#include <video/vops.h>

#include <assert.h>
#include <phantom_libc.h>
//#include <event.h>
//#include <spinlock.h>

#include <sys/libkern.h>

#include "trig_func.inc"



void
w_clear( window_handle_t win )
{
    w_fill( win, COLOR_BLACK );
}



// SLOOOW! Checks bounds on each pixel

#define _PLOT(w,x,y,c) do {\
    if((x) >= 0 && (y) >= 0 && (x) < (w)->xsize && (y) < (w)->ysize)\
    (w)->w_pixel[(x)+(y)*(w)->xsize] = c;\
    } while(0)\

// fast, but can DAMAGE MEMORY - check bounds before calling

#define _UNCH_PLOT(w,x,y,c) do {\
    (w)->w_pixel[(x)+(y)*(w)->xsize] = c;\
    } while(0)\


void
w_draw_pixel( window_handle_t w, int x, int y, rgba_t color )
{
    _PLOT(w, x, y, color);
}

static inline int SGN(int v) { return v == 0 ? 0 : ( (v > 0) ? 1 : -1); }



void w_draw_line( window_handle_t w, int x1, int y1, int x2, int y2, rgba_t c)
{
    int a,x,y;
    int i;
    int d;
    int dx=x2-x1;
    int dy=y2-y1;

    //if (!dx) { vline(w,x1,y1,y2,c); return }
    //if (!dy) { hline(w,x1,y1,x2,c); return }

    if (abs(dx) > abs(dy))
    {
        d=SGN(dx);

        a = (dx == 0) ? 0 : (dy<<16) / abs(dx);

        for( i=x1,y=32768+(y1<<16); i != x2; i += d,y += a )
            _PLOT(w,i,(int)(y>>16),c);
    }
    else
    {
        d=SGN(dy);

        a = (dy == 0) ? 0 : ((dx<<16) / abs(dy));

        for( i=y1,x=32768+(x1<<16); i != y2; i += d,x += a )
            _PLOT(w,(int)(x>>16),i,c);
    }

    _PLOT(w,x2,y2,c);

}



void w_fill_ellipse( window_handle_t w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c)
{
    int i,r,a,s;
    int ry=(ly+1)>>1;
    int rx=lx>>1;

    for(i=0,a=64;i<ry;i++)
    {
        s = ((ry-i)<<14) / ry;


        while(sn[a]>s)
            a++;

        r = rx + ((cs[a]*rx)>>14);
        w_draw_line(w, x+r, y+i, x+lx-1-r, y+i, c);
        w_draw_line(w, x+r, y+ly-i-1, x+lx-1-r, y+ly-i-1, c);
    }
}





#if 0



/********************************************************************/
/* variables globales */


static int vp=0;
static int nbscreen=2;
static long pt_ecran_actif,pt_ecran_travail,tempo_pt_ecran;

int mousex,mousey,mousek,vblclock;
int msex,msey,msek,mclk;

int nbrbuffer=0;
int msebufptr=0;
static int lstbufptr=0;
static int oldvp,oldx,oldy;
static int oldmselevel= -1;
static int mouselevel= -1;

int statek=0;
char bufferflag=1;
char kbufferflag=1;

static char color=15;

static xcur=0;
static ycur=0;

#ifdef DOUBLELIGNES
#define NBLIGNES 400
#define ECRTAILLE 0x8000L
#else
#define NBLIGNES 200
#define ECRTAILLE 0x4000L
#endif

static int filltab[NBLIGNES];





































/******************************************************************/
/* instructions graphiques de base */


#define SGN(x) ((x==0)?(0):((x<0)?(-1):(1)))
#define ABS(x) ((x<0)?(-(x)):(x))

/*
void pellipse(int x,int y,int lx,int ly,int c)
{
    register int i,r,a,s;
    register int ry=(ly+1)>>1;
    register int rx=lx>>1;

    if (c>=0) setcolor(c);
    for(i=0,a=64;i<ry;i++)
    {
        s=(int)(((long)(ry-i)<<14)/ry);
        while(sn[a]>s) a++;
        r=rx+(int)((cs[a]*(long)rx)>>14);
        hline(x+r,y+i,x+lx-1-r,-1);
        hline(x+r,y+ly-i-1,x+lx-1-r,-1);
    }
}
*/

/*
void pbox(int x,int y,int lx,int ly,int c)
{
    register int i;

    if (c>=0) setcolor(c);
    for(i=y;i<y+ly;i++) hline(x,i,x+lx-1,-1);
}
*/



void rline(int x1,int y1,int x2,int y2,int c)
{
    register long a,x,y;
    register int i;
    register int d;
    register int dx=x2-x1;
    register int dy=y2-y1;

    if (c>=0) setcolor(c);

    if (!dx) vline(x1,y1,y2,-1);
    else
        if (!dy) hline(x1,y1,x2,-1);
        else
        {

            if (ABS(dx)>ABS(dy))
            {
                d=SGN(dx);
                a=((long)dy<<16)/ABS(dx);
                for(i=x1,y=32768L+((long)y1<<16);i!=x2;i+=d,y+=a)
                    plot(i,(int)(y>>16),-1);
            }
            else
            {
                d=SGN(dy);
                a=((long)dx<<16)/ABS(dy);
                for(i=y1,x=32768L+((long)x1<<16);i!=y2;i+=d,x+=a)
                    plot((int)(x>>16),i,-1);
            }

            plot(x2,y2,-1);
        }

}


/*
void dline(x1,y1,x2,y2,c)
int x1,y1,x2,y2,c;
{
    register long a,x,y;
    register int i;
    register int d;
    register int dx=x2-x1;
    register int dy=y2-y1;

    if (c>=0) setcolor(c);

    if (!dx) vline(x1,y1,y2,-1);
    else
        if (!dy) hline(x1,y1,x2,-1);
        else
        {

            if (ABS(dx)>ABS(dy))
            {
                d=SGN(dx);
                a=((long)dy<<16)/ABS(dx);
                for(i=x1,y=32768L+((long)y1<<16);i!=x2;i+=d,y+=a)
                    plot(i,(int)(y>>16),-1);
            }
            else
            {
                d=SGN(dy);
                a=((long)dx<<16)/ABS(dy);
                for(i=y1,x=32768L+((long)x1<<16);i!=y2;i+=d,x+=a)
                    plot((int)(x>>16),i,-1);
            }

            plot(x2,y2,-1);
        }

}
*/

/*
void dbox(x,y,lx,ly,c)
int x,y,lx,ly,c;
{
    if (c>=0) setcolor(c);
    hline(x,y,x+lx-1,-1);
    hline(x,y+ly-1,x+lx-1,-1);
    vline(x,y,y+ly-1,-1);
    vline(x+lx-1,y,y+ly-1,-1);
}
*/


/*
void plot(int x,int y,int c)
{
    register char *ptr;

    if (c>=0) setcolor(c);
    wmap(x&3);
    ptr=(char *)(pt_ecran_travail+(y<<6)+(y<<4)+(x>>2));
    *ptr=color;
}
*/

void vline(int x,int y,int y1,int c)
{
    register char *ptr;
    register int i;
    register int t;
    register col;
    register ey;

    if (c>=0) setcolor(c);

    col=color;
    if (y>y1)
    {
        t=y;
        y=y1;
        y1=t;
    }
    ey=y1;
    ptr=(char *)(pt_ecran_travail+(y<<6)+(y<<4)+(x>>2));
    wmap(x&3);
    for(i=y;i<ey;i++,ptr+=80) *ptr=col;
}




void hline(x,y,x1,c)
int x,y,x1,c;
{
    register char *ptr;
    register unsigned int m;
    register int i,cx,cx1,bx,bx1;
    register char col;

    if (x<x1)
    {
        cx=x>>2;
        cx1=x1>>2;
        bx=x&3;
        bx1=x1&3;
    }
    else
    {
        cx1=x>>2;
        cx=x1>>2;
        bx1=x&3;
        bx=x1&3;
    }

    if (c>=0) setcolor(c);
    col=color;
    ptr=(char *)(pt_ecran_travail+(y<<6)+(y<<4)+cx);

    if (cx==cx1)
    {
        m=15;
        m>>=(3-bx1+bx);
        m<<=bx;
        wbit(m);
        *ptr=col;
    }
    else
    {
        m=15;
        m<<=bx;
        wbit(m);
        *(ptr++)=col;
        cx++;
        if (cx!=cx1)
        {
            wall();
            memset(ptr,col,cx1-cx);
            ptr+=cx1-cx;
        }
        m=15;
        m>>=3-bx1;
        wbit(m);
        *ptr=col;
    }
}



void polyline(int n,int *tp,int c)
{
    register int i;

    if (c>=0) setcolor(c);
    for(i=0;i<n-1;i++) rline(tp[i*2],tp[i*2+1],tp[i*2+2],tp[i*2+3],-1);
}


static void fplot(int x,int y)
{
    register int l=filltab[y];
    register int d;

    if (l>=0)
    {
        d=l-x;
        if ((!d)||(d==1)||(d== -1))
        {
            plot(x,y,-1);
            filltab[y]=x;
        }
        else
        {
            hline(x,y,l,-1);
            filltab[y]= -1;
        }
    }
    else
    {
        plot(x,y,-1);
        filltab[y]=x;
    }
}


static void fline(int x1,int y1,int x2,int y2)
{
    register long a,x,y;
    register int i;
    register int d;
    register int dx=x2-x1;
    register int dy=y2-y1;


    if (dx|dy)
        if (ABS(dx)>ABS(dy))
        {
            d=SGN(dx);
            a=((long)dy<<16)/ABS(dx);
            for(i=x1,y=32768L+((long)y1<<16);i!=x2;i+=d,y+=a)
                fplot(i,(int)(y>>16));
        }
        else
        {
            d=SGN(dy);
            a=((long)dx<<16)/ABS(dy);
            for(i=y1,x=32768L+((long)x1<<16);i!=y2;i+=d,x+=a)
                fplot((int)(x>>16),i);
        }

    fplot(x2,y2);
}





void polyfill(int n,int *tp,int c)
{
    register int i;

    if (c>=0) setcolor(c);

    memset((void *)filltab,255,2*NBLIGNES);
    for(i=0;i<n-1;i++) fline(tp[i*2],tp[i*2+1],tp[i*2+2],tp[i*2+3]);
}











#endif





