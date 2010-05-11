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
#include <spinlock.h>

#include <sys/libkern.h>

#include "trig_func.inc"



void
drv_video_window_fill( drv_video_window_t *win, rgba_t color )
{
    int i = (win->xsize * win->ysize) - 1;
    for( ; i >= 0; i-- )
        win->pixel[i] = color;
}


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
drv_video_window_draw_bitmap( drv_video_window_t *w, int x, int y, drv_video_bitmap_t *bmp )
{
    bitmap2bitmap(
                  w->pixel, w->xsize, w->ysize, x, y,
                  bmp->pixel, bmp->xsize, bmp->ysize, 0, 0,
                  bmp->xsize, bmp->ysize
                 );

}

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
                                 int x,int y,int lx,int ly,
                                 rgba_t c)
{
    drv_video_window_draw_line(w,x,y,x+lx-1,y,c);
    drv_video_window_draw_line(w,x,y+ly-1,x+lx-1,y+ly-1,c);
    drv_video_window_draw_line(w,x,y,x,y+ly-1,c);
    drv_video_window_draw_line(w,x+lx-1,y,x+lx-1,y+ly-1,c);
}








#if 0

/* pcvga.c : module de base pour PC a faible capacite mais vitesse elevee
 pour VGA 256 couleurs. A linker avec pcvga2.obj (tasm /mx pcvga2.asm)

 Guillaume LAMONOCA
 Supelec (94)

 Utiliser Turbo C.
 Compiler en modele LARGE ou HUGE. (huge) pour les pointeurs "balladeurs".

 Ram limitee a 640 ko.

 En maintenant E enfonce on simule la souris avec le pave numerique:
 1-9: deplacement souris
 '/': bouton gauche
 '*': bouton droit

 L'affichage du curseur de la souris est mis a jour a chaque appel
 des fonctions swap() et refresh(), getch(), hide(), show() et confirm().
 La fonction waitdelay() utilise refresh() pour que la souris soit raffraichie.

 Ce module de base est 100% compatible avec les autres.

 Mais il contient de nouvelles fonctions pour gerer les 256 couleurs:
 (dans le header definir EXTENSION). SVP ne pas utiliser ces fonctions pour
 des programmes domaine public: restez compatibles avec 16 couleurs (merci).

 Attention: Les sprites (blocs graphiques ayant un masque) ne peuvent pas
 utiliser la couleur no 255!

 Pour les samples le module utilise un petit fichier de configuration
 "SPEECH.CFG". Pour modifier le choix du speech device, il suffit
 d'effacer ce fichier (Le module en produira un nouveau).
 Si le sample depasse 64Ko, le surplus n'est pas joue.

 Vous pouvez definir DOUBLELIGNES pour avoir 400 lignes au lieu de 200
 (SVP ne pas utiliser cette option pour des softs domaine public.merci)

 Attention: a cause de la segmention les fichiers sont limites a 64ko.
 De meme les blocs graphiques ne doivent pas etre trop grands (lx*ly<65536).
 */






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
static unsigned int oldpal[16];
static unsigned int oldvgapal[3*256];

static unsigned int egapal[16]=
{
    0x0000,0x1008,0x3080,0x5088,
    0x2800,0x3808,0x4880,0x5ccc,
    0x3888,0x400f,0x50f0,0x60ff,
    0x4f00,0x5f0f,0x7ff0,0x8fff
};


int msebuffer[128];
static void *mousebloc;
static void *mousefond;
static char defaultmouse[256]=
{
    0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x0f,0x00,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x0f,0x00,0xff,0x00,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0x00,0xff,0xff,0x00,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x00,0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};


char keymap[256]=
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

char keybuffer[256];
int keybufptr=0;
int keybufend=0;
int keybufnbr=0;











/********************************************************************/
/* fontes */

static unsigned int sysfonte[64*16]=
{
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc3,0xc3,0xc3,0xc3,0xc7,0xe7,0xc7,0xe7,0x1c,0x1c,0x1c,0x18,0x18,0x00,0x18,0x00,
    0x81,0x81,0x81,0xdb,0xff,0xff,0xff,0xff,0x36,0x36,0x24,0x00,0x00,0x00,0x00,0x00,
    0xc8,0x88,0x00,0x81,0x80,0x00,0x81,0xdb,0x13,0x33,0x7e,0x36,0x37,0x7e,0x24,0x00,
    0x83,0x01,0x01,0x81,0xc0,0x00,0x80,0xc1,0x3c,0x6e,0x68,0x3e,0x0b,0x6b,0x3e,0x00,
    0x9f,0x09,0x81,0xc3,0xc1,0x80,0xd8,0xfd,0x20,0x52,0x24,0x08,0x12,0x25,0x02,0x00,
    0xc7,0x83,0x83,0x83,0x00,0x00,0x80,0xc6,0x18,0x24,0x34,0x38,0x4d,0x46,0x39,0x00,
    0x8f,0x8f,0xcf,0x8f,0xdf,0xff,0xff,0xff,0x30,0x30,0x10,0x20,0x00,0x00,0x00,0x00,
    0xf3,0xe3,0xc7,0xc7,0xc7,0xe7,0xf3,0xfb,0x04,0x08,0x18,0x10,0x18,0x08,0x04,0x00,
    0xcf,0xe7,0xe3,0xf3,0xe3,0xe3,0xc7,0xef,0x10,0x08,0x0c,0x04,0x0c,0x08,0x10,0x00,
    0xff,0xc9,0xe1,0x80,0xc0,0xc1,0xed,0xff,0x00,0x12,0x0c,0x3f,0x0c,0x12,0x00,0x00,
    0xff,0xe3,0xe3,0x80,0xc0,0xe3,0xf3,0xff,0x00,0x0c,0x0c,0x3f,0x0c,0x0c,0x00,0x00,
    0xff,0xff,0xff,0xff,0xc7,0xc7,0x87,0xcf,0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00,
    0xff,0xff,0xff,0x81,0xc1,0xff,0xff,0xff,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0x8f,0x8f,0xcf,0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,
    0xff,0xf9,0xf1,0xe3,0xc7,0x8f,0xdf,0xff,0x00,0x02,0x04,0x08,0x10,0x20,0x00,0x00,
    0xc3,0x81,0x00,0x00,0x00,0x80,0xc1,0xe3,0x1c,0x26,0x63,0x63,0x63,0x32,0x1c,0x00,
    0xe3,0xc3,0xe3,0xe3,0xe3,0xe3,0x80,0xc0,0x0c,0x1c,0x0c,0x0c,0x0c,0x0c,0x3f,0x00,
    0x81,0x00,0x80,0xc0,0x81,0x03,0x00,0x80,0x3e,0x63,0x07,0x1e,0x3c,0x70,0x7f,0x00,
    0x00,0x80,0xe1,0xc1,0xe0,0x00,0x80,0xc1,0x7f,0x06,0x0c,0x1e,0x03,0x63,0x3e,0x00,
    0xe1,0xc1,0x81,0x01,0x00,0x80,0xf1,0xf9,0x0e,0x1e,0x36,0x66,0x7f,0x06,0x06,0x00,
    0x01,0x01,0x01,0x80,0xf8,0x18,0x80,0xc1,0x7e,0x60,0x7e,0x03,0x03,0x63,0x3e,0x00,
    0xc1,0x81,0x0f,0x01,0x00,0x00,0x80,0xc1,0x1e,0x30,0x60,0x7e,0x63,0x63,0x3e,0x00,
    0x00,0x80,0xf0,0xe1,0xc3,0xc7,0xc7,0xe7,0x7f,0x03,0x06,0x0c,0x18,0x18,0x18,0x00,
    0x83,0x01,0x01,0x81,0x00,0x00,0x80,0xc1,0x3c,0x62,0x72,0x3c,0x4f,0x43,0x3e,0x00,
    0x81,0x00,0x00,0x80,0xc0,0xf0,0x81,0xc3,0x3e,0x63,0x63,0x3f,0x03,0x06,0x3c,0x00,
    0xff,0xc7,0xc7,0xe7,0xc7,0xc7,0xe7,0xff,0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00,
    0xff,0xc7,0xc7,0xe7,0xc7,0xc7,0x87,0xcf,0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00,
    0xf1,0xe1,0xc3,0x87,0xc7,0xe3,0xf1,0xf9,0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00,
    0xff,0xff,0x81,0xc1,0x81,0xc1,0xff,0xff,0x00,0x00,0x3e,0x00,0x3e,0x00,0x00,0x00,
    0x8f,0xc7,0xe3,0xf1,0xe1,0xc3,0x87,0xcf,0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00,
    0x81,0x00,0x00,0x80,0xe1,0xf3,0xe3,0xf3,0x3e,0x7f,0x63,0x06,0x0c,0x00,0x0c,0x00,
    0x83,0x01,0x00,0x00,0x00,0x00,0x00,0x81,0x3c,0x42,0x99,0xa1,0xa1,0x99,0x42,0x3c,
    0xc3,0x81,0x00,0x00,0x00,0x00,0x18,0x9c,0x1c,0x36,0x63,0x63,0x7f,0x63,0x63,0x00,
    0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x7e,0x63,0x63,0x7e,0x63,0x63,0x7e,0x00,
    0xc1,0x80,0x0c,0x1f,0x1f,0x88,0xc0,0xe1,0x1e,0x33,0x60,0x60,0x60,0x33,0x1e,0x00,
    0x03,0x01,0x00,0x00,0x00,0x00,0x01,0x83,0x7c,0x66,0x63,0x63,0x63,0x66,0x7c,0x00,
    0x00,0x00,0x1f,0x01,0x01,0x1f,0x00,0x80,0x7f,0x60,0x60,0x7e,0x60,0x60,0x7f,0x00,
    0x00,0x00,0x1f,0x01,0x01,0x1f,0x1f,0x9f,0x7f,0x60,0x60,0x7e,0x60,0x60,0x60,0x00,
    0xc0,0x80,0x0f,0x00,0x00,0x80,0xc0,0xe0,0x1f,0x30,0x60,0x6f,0x63,0x33,0x1f,0x00,
    0x18,0x18,0x18,0x00,0x00,0x18,0x18,0x9c,0x63,0x63,0x63,0x7f,0x63,0x63,0x63,0x00,
    0x80,0xc0,0xe3,0xe3,0xe3,0xe3,0x80,0xc0,0x3f,0x0c,0x0c,0x0c,0x0c,0x0c,0x3f,0x00,
    0xf8,0xf8,0xf8,0xf8,0xf8,0x18,0x80,0xc1,0x03,0x03,0x03,0x03,0x03,0x63,0x3e,0x00,
    0x18,0x10,0x01,0x03,0x03,0x01,0x10,0x98,0x63,0x66,0x6c,0x78,0x7c,0x6e,0x67,0x00,
    0x8f,0x8f,0x8f,0x8f,0x8f,0x8f,0x80,0xc0,0x30,0x30,0x30,0x30,0x30,0x30,0x3f,0x00,
    0x18,0x00,0x00,0x00,0x10,0x18,0x18,0x9c,0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00,
    0x18,0x08,0x00,0x00,0x00,0x10,0x18,0x9c,0x63,0x73,0x7b,0x7f,0x6f,0x67,0x63,0x00,
    0x81,0x00,0x00,0x00,0x00,0x00,0x80,0xc1,0x3e,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,
    0x01,0x00,0x00,0x00,0x00,0x01,0x1f,0x9f,0x7e,0x63,0x63,0x63,0x7e,0x60,0x60,0x00,
    0x81,0x00,0x00,0x00,0x00,0x00,0x80,0xc2,0x3e,0x63,0x63,0x63,0x6f,0x66,0x3d,0x00,
    0x01,0x00,0x00,0x00,0x00,0x01,0x10,0x98,0x7e,0x63,0x63,0x67,0x7c,0x6e,0x67,0x00,
    0x83,0x01,0x01,0x81,0xc0,0x00,0x80,0xc1,0x3c,0x66,0x60,0x3e,0x03,0x63,0x3e,0x00,
    0x80,0xc0,0xe3,0xe3,0xe3,0xe3,0xe3,0xf3,0x3f,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x00,
    0x18,0x18,0x18,0x18,0x18,0x18,0x80,0xc1,0x63,0x63,0x63,0x63,0x63,0x63,0x3e,0x00,
    0x18,0x18,0x18,0x00,0x80,0xc1,0xe3,0xf7,0x63,0x63,0x63,0x77,0x3e,0x1c,0x08,0x00,
    0x18,0x18,0x18,0x00,0x00,0x00,0x08,0x9c,0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00,
    0x18,0x00,0x80,0xc1,0x81,0x00,0x08,0x9c,0x63,0x77,0x3e,0x1c,0x3e,0x77,0x63,0x00,
    0x11,0x11,0x11,0x81,0xc3,0xc7,0xc7,0xe7,0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00,
    0x00,0x80,0xe0,0xc1,0x83,0x07,0x00,0x80,0x7f,0x07,0x0e,0x1c,0x38,0x70,0x7f,0x00,
    0xc3,0xc3,0xc7,0xc7,0xc7,0xc7,0xc3,0xe3,0x1c,0x18,0x18,0x18,0x18,0x18,0x1c,0x00,
    0xff,0x9f,0xcf,0xe7,0xf3,0xf9,0xfd,0xff,0x00,0x20,0x10,0x08,0x04,0x02,0x00,0x00,
    0xc3,0xe3,0xe3,0xe3,0xe3,0xe3,0xc3,0xe3,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1c,0x00,
    0xe7,0xc3,0x81,0x08,0x9c,0xff,0xff,0xff,0x08,0x1c,0x36,0x63,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x00
};






















/************************************************************/
/* initialisation du systeme */

int sndblast=0;
int dacport=0;
int speaker=0;








static int testce(void)
{
    union REGS regs;

    regs.x.ax=0x1200;
    int86(0x16,&regs,&regs );
    return (regs.x.ax!=0x1200);
}


int initsystem(void)
{
    int r=0;
    int i,j;
    unsigned char m;
    int cfg[3];

    srand(*(int *)0x0040006cL);

    clrscr();

    m=*(unsigned char *)0xf000fffeL;
    if ((m==0xfe)||(m==0xfb)||(m==0xff))
        for(i=0;i<256;i++) codekey[i]=codepcxt[i];

    if ((m==0xfc)&&(!testce()))
        for(i=0;i<256;i++) codekey[i]=codeat[i];

    initkbd();

    if (bexist("speech.cfg"))
    {
        bload("speech.cfg",cfg,0L,6L);
        sndblast=cfg[0];
        dacport=cfg[1];
        speaker=cfg[2];
    }
    else
    {
        printf("VGA 256Ko card required.\n\n");
        printf("   SPEECH.CFG not found. Creating it...\n");
        printf("   Erase it if you want to modify speech device.\n\n");
        printf("   Please select your speech device:\n\n");
        printf("0: no sound\n");
        printf("1: internal speaker  (only efficient on fast computers)\n");
        printf("2: soundblaster card (22ch)\n");
        printf("3: soundmaster+ card (22fh)\n");
        printf("4: card using 'LPT1' (%xh)\n",*(int *)0x00400008L);
        printf("5: modify soundblaster base address\n");
        printf("6: other DAC port\n");

        do
        {
            r=getch();
            if ((r>47)&&(r<55)) r-=48; else r=-1;
        }
        while(r==-1);

        if (r==1) { dacport=0x42; speaker=1; }
        if (r==2) { dacport=0x22c; sndblast=1; }
        if (r==3) dacport=0x22f;
        if (r==4) dacport=*(int *)0x00400008L;
        if (r==5) sndblast=1;

        if (r>4)
        {
            if (r==5) printf("soundblaster DAC port is base address + 00c\n");
            else printf("Enter your EXACT DAC port (base address + correct offset)\n");

            do
            {
                printf("Enter your DAC port:");
                dacport=0;
                for(i=0;i<3;i++)
                {
                    do
                    {
                        r=getch();
                        if ((r>47)&&(r<58)) r-=48; else
                            if ((r>96)&&(r<103)) r-=87; else
                                if ((r>64)&&(r<71)) r-=55; else r=-1;
                        if (r!=-1) if (r<10) printf("%c",r+48); else printf("%c",r+55);
                    }
                    while(r==-1);
                    dacport=dacport*16+r;
                }
                printf("\nIs %xh correct? (Yes/No)\n",dacport);
                r=getch();
            }
            while((r!='y')&&(r!='Y'));
        }


        cfg[0]=sndblast;
        cfg[1]=dacport;
        cfg[2]=speaker;
        bmake("speech.cfg",cfg,6L);
    }

    if (initscreen())
    {
        initmouse();
        if (sndblast) sendinit(0xd1);
        if (speaker) inithp();
        if (dacport) initspl();
        r=1;

        for(i=0;i<16;i++)
            for(j=0;j<16;j++)
            {
                m=defaultmouse[i*16+j];
                plot(j,i,(int)m);
                if (m==255) plot(j,i+16,0);
                else plot(j,i+16,255);
            }

        initbloc(&mousefond);
        initbloc(&mousebloc);
        getbloc(&mousebloc,0,0,16,16);
        getmask(&mousebloc,0,16);

        cls();
        show();
    }
    else
    {
        killkbd();
        printf("\nVGA card not found.\n");
    }

    return r;
}
























/************************************************************/
/* instructions de gestion des ecrans graphiques */



















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













/**************************************************************/
/* gestion des fichiers */

#define MAXFILE 16
FILE *ftab[MAXFILE];
char fused[MAXFILE]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};




#define MAXF 32768

static int flread(void *adr,long len,FILE *fp)
{
    unsigned int l;
    char huge *ptr;
    int r=1;

    ptr=(char huge *)adr;
    while((len)&&(r==1))
    {
        if (len>MAXF) l=MAXF; else l=(unsigned int)len;
        len-=(long)l;
        r=fread((void *)ptr,l,1,fp);
        ptr+=l;
    }
    return r;
}

static int flwrite(void *adr,long len,FILE *fp)
{
    unsigned int l;
    char huge *ptr;
    int r=1;

    ptr=(char huge *)adr;
    while((len)&&(r==1))
    {
        if (len>MAXF) l=MAXF; else l=(unsigned int)len;
        len-=(long)l;
        r=fwrite((void *)ptr,l,1,fp);
        ptr+=l;
    }
    return r;
}



static int nextfile(void)
{
    int r=0;
    int i;

    for(i=0;i<MAXFILE;i++) if (!fused[i]) r=i+1;
    return(r);
}


static void verifnom(char *s,char *s2)
{

    while(*s)
    {
        *s2= *s;
        if (*s=='/') *s2='\\';
        s++;
        s2++;
    }
    *s2=0;

}



int bexist(char *nom)
{
    FILE *fp;
    char nom2[200];
    verifnom(nom,nom2);

    fp = fopen(nom2,"r");
    if (fp)
    {
        fclose(fp);
        return(1);
    }
    else
        return(0);
}


long bsize(char *nom)
{
    FILE *fp;
    char nom2[200];
    long size=0L;

    verifnom(nom,nom2);

    fp = fopen(nom2,"r");
    if (fp)
    {
        fseek(fp, 0L, 2);
        size = ftell(fp);
        fclose(fp);
    }

    return(size);
}



int bsave(char *nom,void *adr,long offset,long len)
{
    void *buffer;
    long filesize;
    FILE *fp;
    int r= -1;
    char huge *d;
    char huge *s;
    long i;

    char nom2[200];
    verifnom(nom,nom2);
    fp = fopen(nom2,"r");
    if (fp)
    {
        r=fseek(fp, 0L, 2);
        filesize = ftell(fp);
        fclose(fp);

        if (!r)
        {
            r= -1;
            if (buffer=memalloc(filesize))
            {
                if (!bload(nom,buffer,0L,filesize))
                {
                    s=(char huge *)adr;
                    d=(char huge *)(buffer)+offset;
                    for(i=0L;i<len;i++) *(d++)= *(s++);
                    r=bmake(nom,buffer,filesize);
                }
                memfree(&buffer);
            }
        }
    }

    return(r);
}




int bload(char *nom,void *adr,long offset,long len)
{
    FILE *fp;
    int r= -1;
    char nom2[200];

    verifnom(nom,nom2);

    fp = fopen(nom2,"rb");
    if (fp)
    {
        fseek(fp, offset, 0);
        r=flread(adr,len,fp);
        if (r==1) r=0;
        else r= -1;
        fclose(fp);

    }

    return(r);
}



int bmake(char *nom,void *adr,long len)
{
    FILE *fp;
    int r= -1;
    char nom2[200];

    verifnom(nom,nom2);

    fp = fopen(nom2,"wb");
    if (fp)
    {
        r=flwrite(adr,len,fp);
        if (r==1) r=0;
        else r= -1;
        fclose(fp);
    }
    return(r);
}



int bcreate(char *nom)
{
    FILE *fp;
    int r=0;
    char nom2[200];

    r=nextfile();
    if (r)
    {
        verifnom(nom,nom2);
        fp = fopen(nom2,"wb");
        if (fp) { ftab[r-1]=fp; fused[r-1]=1; }
        else r=0;
    }
    return(r);
}


int bopen(char *nom)
{
    FILE *fp;
    int r=0;
    char nom2[200];

    r=nextfile();
    if (r)
    {
        verifnom(nom,nom2);
        fp = fopen(nom2,"rb");
        if (fp) { ftab[r-1]=fp; fused[r-1]=1; fseek(fp, 0L, 0); }
        else r=0;
    }
    return(r);
}



int bclose(int n)
{
    FILE *fp;
    int r=0;

    if (n)
    {
        fp=ftab[n-1];
        fclose(fp);
        fused[n-1]=0;
    }
    return(r);
}




int bwrite(int n,void *adr,long len)
{
    FILE *fp;
    int r= -1;

    if (n)
    {
        fp=ftab[n-1];
        if (fp)
        {
            r=flwrite(adr,len,fp);
            if (r==1) r=0;
            else r= -1;
        }
    }
    return(r);
}




int bread(int n,void *adr,long len)
{
    FILE *fp;
    int r= -1;

    if (n)
    {
        fp=ftab[n-1];
        if (fp)
        {
            r=flread(adr,len,fp);
            if (r==1) r=0;
            else r= -1;
        }
    }
    return(r);
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















/*****************************************************************/
/* instructions de gestion de fonte */


void affchar(int x,int y,char c)
{
    register char *ptr;
    register unsigned int n,i,j,k,m,f;

    if ((c>31)&&(c<=127))
    {
        if (c>95) c-=32;

        n=c-32;
        n<<=4;


        ptr=(char *)(pt_ecran_travail+(y<<6)+(y<<4)+(x>>2));

        for(k=128;k;k>>=1)
        {
            wmap(x&3);
            for(i=0;i<8;i++)
            {
                m=sysfonte[n+i];
                f=sysfonte[n+i+8];
                if (!(m&k))
                    if (f&k) *ptr=color; else *ptr=0;
                ptr+=80;
            }
            x++;
            if (!(x&3)) ptr++;
            ptr-=640;
        }

    }

}



void afftext(int x,int y,char *s)
{

    while(*s)
    {
        affchar(x,y,*s++);
        x+=8;
        if (x>312)
        {
            x=0;
            y+=8;
            if (y>(NBLIGNES-8)) y=0;
        }
    }

}



void printchar(char c)
{
    void *ecr;
    int t,i;
    char m;

    while(ycur>(NBLIGNES-8))
    {
        ycur-=8;

        copymode_on();
        bmove((void *)(pt_ecran_travail+640L),(void *)pt_ecran_travail,ECRTAILLE-640L);
        copymode_off();

        t=color;
        pbox(0,NBLIGNES-8,320,8,0);
        setcolor(t);
    }


    if (c==10) xcur= -8;
    else
        if (c==13)
        {
            ycur+=8;
            xcur= -8;
        }
        else
            affchar(xcur,ycur,c);

    xcur+=8;
    if (xcur>312)
    {
        xcur=0;
        ycur+=8;
    }


}




void print(char *s)
{
    while(*s) printchar(*s++);

    xcur=0;
    ycur+=8;

}

















/*******************************************************************/
/* gestion des samples */

/* sndblast: */
/* 0:simple dac port (soundmaster+:22fh) */
/* 1:soundblaster card (on:d1h,sample:10h+data,off:d3h (port 22ch)) */

int spkperiod=110;
int addcnt=65535;
int period=65535;
int voldigit,cntper,offstart,segdigit,offdigit,lendigit,repdigit,cntdigit;


void playsample(void *adr,long len,long freq)
{
    int v=15;
    if (len>65534L) len=65534L;

    if (dacport)
    {
        segdigit=0;
        offstart=offdigit=FP_OFF(adr);
        cntdigit=lendigit=(unsigned int)len;
        repdigit=1;
        voldigit=(15-v)/2;
        period=(int)(1200000L/freq);
        if (speaker) period=spkperiod;
        freqspl();
        segdigit=FP_SEG(adr);
    }
}



void stopsample(void)
{
    segdigit=0;
    period=65535;
    freqspl();
}



void fixsample(void *adr,long len)
{
    long i;
    char huge *ptr;

    ptr=(char huge *)adr;
    for(i=0L;i<len;i++) *(ptr++)-=128;
}




#endif





