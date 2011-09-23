/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Windowing system - painting.
 *
 *
**/


#include <drv_video_screen.h>
#include <video.h>

#include <assert.h>
#include <phantom_libc.h>
#include <event.h>
//#include <spinlock.h>

#include <sys/libkern.h>

#include "trig_func.inc"




void
drv_video_window_clear( drv_video_window_t *win )
{
    drv_video_window_fill( win, COLOR_BLACK );
}


void
drv_video_window_fill_rect( drv_video_window_t *w, rgba_t color, rect_t r )
{
    if( rect_win_bounds( &r, w ) )
        return;

    int yp = r.y + r.ysize - 1;
    for( ; yp >= r.y; yp-- )
    {
        rgba_t *dst = w->pixel + yp*w->xsize + r.x;
        rgba2rgba_replicate( dst, &color, r.xsize );
    }
}

void
drv_video_window_pixel( drv_video_window_t *w, int x, int y, rgba_t color )
{
    rect_t r;
    r.x = x;
    r.y = y;
    r.xsize = r.ysize = 1;

    if( rect_win_bounds( &r, w ) )
        return;

    rgba_t *dst = w->pixel + y*w->xsize + x;
    *dst = color;
}
/*
void
drv_video_window_draw_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp )
{
    bitmap2bitmap(
                  w->pixel, w->xsize, w->ysize, x, y,
                  bmp->pixel, bmp->xsize, bmp->ysize, 0, 0,
                  bmp->xsize, bmp->ysize
                 );

}
*/
// SLOOOW! Checks bounds on each pixel

#define _PLOT(w,x,y,c) do {\
    if((x) > 0 && (y) > 0 && (x) < (w)->xsize && (y) <= (w)->ysize)\
    (w)->pixel[(x)+(y)*(w)->xsize] = c;\
    } while(0)\

// fast, but can DAMAGE MEMORY - check bounds before calling

#define _UNCH_PLOT(w,x,y,c) do {\
    (w)->pixel[(x)+(y)*(w)->xsize] = c;\
    } while(0)\


static inline int SGN(int v) { return v == 0 ? 0 : ( (v > 0) ? 1 : -1); }



void drv_video_window_draw_line( drv_video_window_t *w,
                                 int x1, int y1, int x2, int y2, rgba_t c)
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



void drv_video_window_fill_ellipse( drv_video_window_t *w,
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
        drv_video_window_draw_line(w, x+r, y+i, x+lx-1-r, y+i, c);
        drv_video_window_draw_line(w, x+r, y+ly-i-1, x+lx-1-r, y+ly-i-1, c);
    }
}


void drv_video_window_fill_box( drv_video_window_t *w,
                                 int x,int y,int lx,int ly,
                                 rgba_t c)
{
    int i;

    for(i=y;i<y+ly;i++)
        drv_video_window_draw_line(w, x,i,x+lx-1,i,c);
}



void drv_video_window_draw_box( drv_video_window_t *w,
                                 int x,int y,int lx, int ly,
                                 rgba_t c)
{
    drv_video_window_draw_line(w,x,y,x+lx-1,y,c);
    drv_video_window_draw_line(w,x,y+ly-1,x+lx-1,y+ly-1,c);
    drv_video_window_draw_line(w,x,y,x,y+ly-1,c);
    drv_video_window_draw_line(w,x+lx-1,y,x+lx-1,y+ly-1,c);
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
























/**************************************************************/
/* transferts de blocs memoire */

void bmove(void *src,void *dst,long len)
{
    register char huge *hs;
    register char huge *hd;
    register char *s;
    register char *d;
    register long hi;
    register unsigned int i,l;

    if (len>65535L)
    {
        hs=(char huge *)src;
        hd=(char huge *)dst;
        if (hs>hd)
            for(hi=0L;hi<len;hi++) *(hd++)= *(hs++);
        else
            for(hd+=len,hs+=len,hi=0L;hi<len;hi++) *(--hd)= *(--hs);
    }
    else
    {
        l=(unsigned int)len;
        s=(char *)src;
        d=(char *)dst;
        if (s>d)
            for(i=0;i<l;i++) *(d++)= *(s++);
        else
            for(d+=l,s+=l,i=0;i<l;i++) *(--d)= *(--s);
    }
}


















/*****************************************************************/
/* instructions de gestion d'horloge                             */


unsigned long systime(void)
{
    unsigned long r;

    r=biostime(0,0L);
    return 54*r;
}


void waitdelay(unsigned int n)
{
    unsigned long start;

    start=systime();

    if (n<100)
        do {} while((unsigned long)(systime()-start)<(unsigned long)n);
    else
        do refresh(); while((unsigned long)(systime()-start)<(unsigned long)n);
}













/*****************************************************************/
/* fonctions aleatoires */


int randint(void)
{
    return(rand());
}

int randval(int n)
{
    return(randint()%n);
}
































/***************************************************************/
/* gestion de blocs graphiques */


typedef struct
{
    int lx;
    int ly;
    int id;
    long bloc;
}
pixbloc;


void initbloc(blocptr)
void **blocptr;
{
    pixbloc *bloc;
    *blocptr=memalloc((long)sizeof(pixbloc));
    bloc=(pixbloc *)*blocptr;
    if (*blocptr) bloc->id=0;
}


void freebloc(blocptr)
void **blocptr;
{
    pixbloc *bloc;
    if (*blocptr)
    {
        bloc=(pixbloc *)*blocptr;
        if (bloc->id) memfree((void **)&bloc->bloc);
        bloc->id=0;
        memfree(blocptr);
    }
}



void getbloc(blocptr,x,y,lx,ly)
void **blocptr;
int x,y,lx,ly;
{
    int i;

    pixbloc *bloc;
    if (*blocptr)
    {
        bloc=(pixbloc *)*blocptr;
        if ((!bloc->id)||(bloc->lx!=lx)||(bloc->ly!=ly))
        {
            if (bloc->id) memfree((void **)&bloc->bloc);
            bloc->id=0;
            if (bloc->bloc=(long)memalloc(ly*(long)lx))
            {
                bloc->id=1;
                bloc->lx=lx;
                bloc->ly=ly;
            }
        }


        if (bloc->id)
        {

            for(i=0;i<4;i++)
            {
                rmap(i);
                getplane(bloc->bloc+((lx>>2)*ly)*i,
                         pt_ecran_travail+(x>>2)+(y<<6)+(y<<4),
                         lx>>2,ly,0,80-(lx>>2));
            }

        }
    }
}


void copybloc(blocptrs,xs,ys,lxs,lys,blocptrd,xd,yd)
void **blocptrs;
int xs,ys,lxs,lys;
void **blocptrd;
int xd,yd;
{
    int i;

    pixbloc *blocs;
    pixbloc *blocd;
    if ((*blocptrs)&&(*blocptrd))
    {
        blocs=(pixbloc *)*blocptrs;
        blocd=(pixbloc *)*blocptrd;
        if ((blocs->id)&&(blocd->id))
            if ((xd>=0)&&(yd>=0)&&(xs>=0)&&(ys>=0))
                if ((xs+lxs<=blocs->lx)&&(ys+lys<=blocs->ly))
                    if ((xd+lxs<=blocd->lx)&&(yd+lys<=blocd->ly))
                    {
                        for(i=0;i<4;i++)
                        {
                            putplane(blocs->bloc+((blocs->lx>>2)*blocs->ly)*i
                                     +(xs>>2)+ys*(blocs->lx>>2),
                                     blocd->bloc+((blocd->lx>>2)*blocd->ly)*i
                                     +(xd>>2)+yd*(blocd->lx>>2),
                                     lxs>>2,lys,(blocs->lx-lxs)>>2,(blocd->lx-lxs)>>2);
                        }
                    }
    }
}



void putpbloc(blocptr,x,y,xs,ys,lxs,lys)
void **blocptr;
int x,y,xs,ys,lxs,lys;
{
    int i;

    pixbloc *bloc;
    if (*blocptr)
    {
        bloc=(pixbloc *)*blocptr;

        if ((y>=0)||(y+lys<=NBLIGNES))
        {
            if (y<0)
            {
                ys+=(-y);
                lys-=(-y);
                y=0;
            }
            else
                if (y+lys>NBLIGNES)
                {
                    lys=NBLIGNES-y;
                }
        }

        if ((lys>0)&&(lxs>0))
            if ((x>=0)&&(y>=0)&&(xs>=0)&&(ys>=0))
                if ((x<320)&&(y<NBLIGNES))
                    if ((x+lxs<=320)&&(y+lys<=NBLIGNES))
                        if (bloc->id==1)
                        {

                            for(i=0;i<4;i++)
                            {
                                wmap(x&3);
                                putplane(bloc->bloc+((bloc->lx>>2)*bloc->ly)*i
                                         +(xs>>2)+ys*(bloc->lx>>2),
                                         pt_ecran_travail+(x>>2)+(y<<6)+(y<<4),
                                         lxs>>2,lys,(bloc->lx-lxs)>>2,80-(lxs>>2));
                                x++;
                            }

                        }
                        else
                            if (bloc->id==2)
                            {

                                for(i=0;i<4;i++)
                                {
                                    wmap(x&3);
                                    putsplane(bloc->bloc+((bloc->lx>>2)*bloc->ly)*i
                                              +(xs>>2)+ys*(bloc->lx>>2),
                                              pt_ecran_travail+(x>>2)+(y<<6)+(y<<4),
                                              lxs>>2,lys,(bloc->lx-lxs)>>2,80-(lxs>>2));
                                    x++;
                                }

                            }


    }
}



void putbloc(blocptr,x,y)
void **blocptr;
int x,y;
{
    pixbloc *bloc;
    if (*blocptr)
    {
        bloc=(pixbloc *)*blocptr;
        putpbloc(blocptr,x,y,0,0,bloc->lx,bloc->ly);
    }
}



void getmask(blocptr,x,y)
void **blocptr;
int x,y;
{
    int i;

    pixbloc *bloc;
    if (*blocptr)
    {
        bloc=(pixbloc *)*blocptr;
        if (bloc->id)
        {
            bloc->id=2;

            for(i=0;i<4;i++)
            {
                rmap(i);
                getsplane(bloc->bloc+((bloc->lx>>2)*bloc->ly)*i,
                          pt_ecran_travail+(x>>2)+(y<<6)+(y<<4),
                          bloc->lx>>2,bloc->ly,0,80-(bloc->lx>>2));
            }

        }
    }
}


























#endif





