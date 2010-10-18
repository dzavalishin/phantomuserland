/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Tetris, original code (C) Copyright 1995, Vadim Antonov (@VG)
 *
 *
**/



//---------------------------------------------------------------------------

#define DEBUG_MSG_PREFIX "tetris"
#include "debug_ext.h"
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>
#include <drv_video_screen.h>
#include <video/color.h>
#include <hal.h>
#include <threads.h>


#include "dev/key_event.h"
#include "misc.h"

//#include <stream/stream.h>
//#include <gpanel/gpanel.h>
//#include <random/rand15.h>
//#include "board-1986be91.h"

static int rbits = 0;

static int k_down = 0, k_up = 0, k_right = 0, k_left = 0;

#define joystick_left() (rbits&1)
#define joystick_right() (rbits&2)
#define joystick_up() (rbits&4)
//#define joystick_down() (k_down > 0 )

static drv_video_window_t *tetris_window;


#define PITWIDTH        12
#define PITDEPTH        24

#define NSHAPES         7
#define NBLOCKS         5

#define FIN             999

typedef struct {
    int     x, y;
} coord_t;

typedef struct {
    int     dx, dy;
    coord_t p [NBLOCKS];
} shape_t;

const shape_t shape [NSHAPES] = {
    /* OOOO */      { 0, 3, { {0,0}, {0,1}, {0,2}, {0,3}, {FIN,FIN} } },

    /* O   */       { 1, 2, { {0,0}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },
    /* OOO */

    /*  O  */       { 1, 2, { {0,1}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },
    /* OOO */

    /*   O */       { 1, 2, { {0,2}, {1,0}, {1,1}, {1,2}, {FIN,FIN} } },
    /* OOO */

    /*  OO */       { 1, 2, { {0,0}, {0,1}, {1,1}, {1,2}, {FIN,FIN} } },
    /* OO  */

    /* OO  */       { 1, 2, { {0,1}, {0,2}, {1,0}, {1,1}, {FIN,FIN} } },
    /*  OO */

    /* OO */        { 1, 1, { {0,0}, {0,1}, {1,0}, {1,1}, {FIN,FIN} } },
    /* OO */
};

int pit [PITDEPTH+1] [PITWIDTH];
int pitcnt [PITDEPTH];
coord_t old [NBLOCKS], new [NBLOCKS], chk [NBLOCKS];

//gpanel_t display;

/*
 * Output piece coordinates given its center and angle
 */
void translate (const shape_t *t, coord_t c, int a, coord_t *res)
{
    coord_t org, tmp;
    int yw, xw, i;

    if (a & 1) {            /* 90 deg */
        xw = t->dy;
        yw = t->dx;
    } else {
        xw = t->dx;
        yw = t->dy;
    }
    org = c;
    org.x -= (xw + 1) / 2;
    org.y -= yw / 2;
    if (org.y < 0)
        org.y = 0;
    if (org.y + yw >= PITWIDTH && c.y <= PITWIDTH)
        org.y = PITWIDTH-1 - yw;
    for (i=0; t->p[i].x!=FIN; i++) {
        switch (a) {
        case 0:
            res[i].x = t->p[i].x;
            res[i].y = t->p[i].y;
            break;
        case 3:
            res[i].x = xw - t->p[i].y;
            res[i].y = t->p[i].x;
            break;
        case 2:
            res[i].x = xw - t->p[i].x;
            res[i].y = yw - t->p[i].y;
            break;
        case 1:
            res[i].x = t->p[i].y;
            res[i].y = yw - t->p[i].x;
        }
        res[i].x += org.x;
        res[i].y += org.y;
    }
    res[i].x = res[i].y = FIN;

    do {
        xw = 0;
        for (i=0; res[i+1].x!=FIN; i++) {
            if (res[i].x < res[i+1].x)
                continue;
            if (res[i].x == res[i+1].x && res[i].y <= res[i+1].y)
                continue;
            xw++;
            tmp = res[i];
            res[i] = res[i+1];
            res[i+1] = tmp;
        }
    } while (xw);
}

/*
 * Draw the block
 */
void draw_block (int h, int w, int visible)
{
    h = PITDEPTH - h - 1;

    h *= 5;
    w *= 5;

    rect_t r;
    r.x = w;
    r.y = h;
    r.xsize = r.ysize = 4;

    if (visible) {
        drv_video_window_fill_rect( tetris_window, COLOR_LIGHTRED, r );
    } else {
        drv_video_window_fill_rect( tetris_window, COLOR_BLACK, r );

        if (h == (PITDEPTH-1)*5)
            drv_video_window_pixel ( tetris_window, w + 2, h + 4, COLOR_LIGHTBLUE);

        if (w == 0)
            drv_video_window_pixel ( tetris_window, w, h + 2, COLOR_LIGHTGREEN);
        else if (w % 20 == 15)
            drv_video_window_pixel ( tetris_window, w + 4, h + 2, COLOR_LIGHTGREEN);
    }
}

/*
 * Move the piece
 */
void move (coord_t *old, coord_t *new)
{
    for (;;) {
        if (old->x == FIN) {
        draw:                   if (new->x == FIN)
            break;
        if (new->x >= 0)
            draw_block (new->x, new->y, 1);
        new++;
        continue;
        }
        if (new->x == FIN) {
        clear:                  if (old->x >= 0)
            draw_block (old->x, old->y, 0);
        old++;
        continue;
        }
        if (old->x > new->x)
            goto draw;
        if (old->x < new->x)
            goto clear;
        if (old->y > new->y)
            goto draw;
        if (old->y != new->y)
            goto clear;
        /* old & new at the same place */
        old++;
        new++;
    }

    drv_video_winblt( tetris_window );
}

/*
 * Draw the pit
 */
static void tetris_clear ()
{
    int h, w;

    for (h=0; h<PITDEPTH; h++) {
        for (w=0; w<PITWIDTH; w++) {
            draw_block (h, w, 0);
            pit[h][w] = 0;
        }
        pitcnt[h] = 0;
    }
    for (w=0; w<PITWIDTH; w++)
        pit[PITDEPTH][w] = 1;
}

/*
 * The piece reached the bottom
 */
static void stopped (coord_t *c)
{
    int h, nfull, w, k;

    /* Count the full lines */
    nfull = 0;
    for (; c->x!=FIN; c++) {
        if (c->x <= 0) {
            /* Game over. */
            tetris_clear();
            return;
        }
        pit[c->x][c->y] = 1;
        ++pitcnt[c->x];
        if (pitcnt[c->x] == PITWIDTH)
            nfull++;
    }
    if (! nfull)
        return;

    /* Clear upper nfull lines */
    for (h=0; h<nfull; h++) {
        for (w=0; w<PITWIDTH; w++) {
            if (pit[h][w]) {
                draw_block (h, w, 0);
            }
        }
    }

    /* Move lines down */
    k = nfull;
    for (h=nfull; h<PITDEPTH && k>0; h++) {
        if (pitcnt[h-k] == PITWIDTH) {
            k--;
            h--;
            continue;
        }
        for (w=0; w<PITWIDTH; w++) {
            if (pit[h][w] != pit[h-k][w]) {
                draw_block (h, w, pit[h-k][w]);
            }
        }
    }

    /* Now fix the pit contents */
    for (h=PITDEPTH-1; h>0; h--) {
        if (pitcnt[h] != PITWIDTH)
            continue;
        memmove (pit[0]+PITWIDTH, pit[0], h * sizeof(pit[0]));
        memset (pit[0], 0, sizeof(pit[0]));
        memmove (pitcnt+1, pitcnt, h * sizeof(pitcnt[0]));
        pitcnt[0] = 0;
        h++;
    }
}


static void tetris_window_loop(void *arg)
{
    (void) arg;

    hal_set_thread_name("KerTetris");
    SHOW_FLOW0( 2, "running");

    int ptype;              /* Piece type */
    int angle, anew;        /* Angle */
    int msec;               /* Timeout */
    coord_t c, cnew, *cp;
    //unsigned up_pressed = 0, left_pressed = 0;
    //unsigned right_pressed = 0; //, down_pressed = 0;

    newpiece:
        SHOW_FLOW0( 2, "newpiece");

        ptype = random() % NSHAPES;
        angle = random() % 3;

        c.y = PITWIDTH/2 - 1;
        for (c.x= -2; ; c.x++) {
            translate (&shape[ptype], c, angle, new);
            for (cp=new; cp->x!=FIN; cp++) {
                if (cp->x >= 0)
                    goto ok;
            }
        }
        ok:
        /* Draw new piece */
        for (cp=new; cp->x!=FIN; cp++) {
            if (cp->x >= 0) {
                draw_block (cp->x, cp->y, 1);
            }
        }
        memcpy (old, new, sizeof old);

        msec = 500;
        for (;;) {
            cnew = c;
            anew = angle;

            rbits = 0;
            if (msec <= 0) {
                rbits = random(); // self-control :)
                /* Timeout: move down */
                SHOW_FLOW0( 7, "down");
                msec = 500;
                cnew.x++;
                translate (&shape[ptype], cnew, anew, chk);
                for (cp=chk; cp->x!=FIN; cp++) {
                    if (cp->x >= 0 && pit[cp->x][cp->y]) {
                        stopped (new);
                        goto newpiece;
                    }
                }
                goto check;
            }

            if ( k_up > 0 ) { 	// up: rotate
                k_up--;

                if (--anew < 0)
                    anew = 3;
                translate (&shape[ptype], cnew, anew, chk);
                goto check;
            }

            if ( k_left > 0 ) {
                k_left--;

                /* move left. */
                if (cnew.y <= 0)
                    continue;
                cnew.y--;
                translate (&shape[ptype], cnew, anew, chk);
                goto check;
            }

            if ( k_right > 0 ) {
                k_right--;

                /* move right */
                if (cnew.y >= PITWIDTH-1)
                    continue;
                cnew.y++;
                translate (&shape[ptype], cnew, anew, chk);
                goto check;
            }

            if( k_down > 0 ) {
                k_down--;
                /* drop */
                for (;;) {
                    cnew.x++;
                    translate (&shape[ptype], cnew, anew, chk);
                    for (cp=chk; cp->x!=FIN; cp++) {
                        if (cp->x >= 0 && pit[cp->x][cp->y]) {
                            cnew.x--;
                            translate (&shape[ptype], cnew, anew, chk);
                            move (new, chk);
                            stopped (chk);
                            goto newpiece;
                        }
                    }
                }
            }

            hal_sleep_msec(10);
            msec -= 10;
            continue;

        check:          for (cp=chk; cp->x!=FIN; cp++) {
            if (cp->y < 0 || cp->y >= PITWIDTH)
                goto done;
        }
        for (cp=chk; cp->x!=FIN; cp++) {
            if (cp->x >= 0 && pit[cp->x][cp->y])
                goto done;
        }
        c = cnew;
        angle = anew;
        memcpy (old, new, sizeof old);
        memcpy (new, chk, sizeof new);
        move (old, new);
    done:           ;
        }

}

static void (*oldEventProcessor)( drv_video_window_t *w, struct ui_event *e );

static void eventProcessor( drv_video_window_t *w, struct ui_event *e )
{

    switch(e->type)
    {
    //case UI_EVENT_TYPE_MOUSE: 	defaultMouseEventProcessor(w, &e); break;

    case UI_EVENT_TYPE_KEY:

        // Ignore key up events
        if(e->modifiers & UI_MODIFIER_KEYUP)
            return;

        switch(e->k.vk)
        {
        case KEY_ARROW_DOWN:
            k_down++;
            SHOW_FLOW( 2, "KEY__DOWN %d", k_down );
            break;

        case KEY_ARROW_UP:
            k_up++;
            SHOW_FLOW( 2, "KEY__UP %d", k_down );
            break;

        case KEY_ARROW_LEFT:
            k_left++;
            SHOW_FLOW( 2, "KEY__LEFT %d", k_down );
            break;

        case KEY_ARROW_RIGHT:
            k_right++;
            SHOW_FLOW( 2, "KEY__RIGHT %d", k_down );
            break;
        }
        return;

    }

    if(oldEventProcessor)
        oldEventProcessor(w, e);
}


void init_tetris ()
{
    tetris_window =
        drv_video_window_create( 60, 120, 700, 550, COLOR_BLACK, "Tetris" );

    oldEventProcessor = tetris_window->inKernelEventProcess;
    tetris_window->inKernelEventProcess = eventProcessor;

    /* Draw the pit */
    tetris_clear();

    hal_start_kernel_thread((void*)tetris_window_loop);

}

