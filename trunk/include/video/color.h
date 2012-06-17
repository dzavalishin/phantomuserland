/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Phantom OS team
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Colors.
 *
**/

#ifndef COLOR_H
#define COLOR_H

#ifndef VCONFIG_H
#include <video/vconfig.h>
#endif // VCONFIG_H

#include <sys/cdefs.h>
#include <phantom_types.h>



struct rgb_t
{
#if BGR
    unsigned char       b;
    unsigned char       g;
    unsigned char       r;
#else
    unsigned char       r;
    unsigned char       g;
    unsigned char       b;
#endif
} __packed;

typedef struct rgb_t rgb_t;


struct rgba_t
{
#if BGR
    unsigned char       b;
    unsigned char       g;
    unsigned char       r;
#else
    unsigned char       r;
    unsigned char       g;
    unsigned char       b;
#endif
    unsigned char       a;      // Alpha
} __packed;

typedef struct rgba_t rgba_t;
typedef struct rgba_t color_t;

extern rgba_t COLOR_TRANSPARENT;

extern rgba_t COLOR_BLACK;
extern rgba_t COLOR_WHITE;

extern rgba_t COLOR_LIGHTGRAY;
extern rgba_t COLOR_DARKGRAY;
extern rgba_t COLOR_YELLOW;
extern rgba_t COLOR_LIGHTRED;
extern rgba_t COLOR_RED;
extern rgba_t COLOR_BROWN;
extern rgba_t COLOR_BLUE;
extern rgba_t COLOR_CYAN;
extern rgba_t COLOR_LIGHTBLUE;
extern rgba_t COLOR_GREEN;
extern rgba_t COLOR_LIGHTGREEN;
extern rgba_t COLOR_MAGENTA;
extern rgba_t COLOR_LIGHTMAGENTA;


#define _C_COLOR_TRANSPARENT  { 0, 0, 0, 0 }

#define _C_COLOR_BLACK  { 0, 0, 0, 0xFF }
#define _C_COLOR_WHITE  { 0xFF, 0xFF, 0xFF, 0xFF }

#define _C_COLOR_RED    { 0, 0, 0x80, 0xFF }
#define _C_COLOR_LIGHTRED  { 0x40, 0x40, 0xFF, 0xFF }

#define _C_COLOR_GREEN  { 0, 0x80, 0, 0xFF }
#define _C_COLOR_LIGHTGREEN  { 0x40, 0xFF, 0x40, 0xFF }

#define _C_COLOR_BLUE   { 0x80, 0, 0, 0xFF }
#define _C_COLOR_LIGHTBLUE  { 0xFF, 0x40, 0x40, 0xFF }

#define _C_COLOR_DARKGRAY  { 0x40, 0x40, 0x40, 0xFF }
#define _C_COLOR_LIGHTGRAY { 0x80, 0x80, 0x80, 0xFF }

#define _C_COLOR_YELLOW { 0x00, 0xFF, 0xFF, 0xFF }

#define _C_COLOR_MAGENTA { 0xFF, 0x00, 0xFF, 0xFF }
#define _C_COLOR_CYAN  { 0xFF, 0xFF, 0x00, 0xFF }


//! Convert HSI color to RGB one
rgba_t Hsi2Rgb(double H, double S, double I );

// TODO what's on big endian?

//#define INT32_TO_RGBA(rgba,i) (  = (i) )
#define INT32_TO_RGBA(rgba,i) ( (rgba) = *((rgba_t*)&(i)) )
#define RGBA_TO_INT32(i,rgba) ( (i) = (*(u_int32_t*)(&(rgba))) )

void rgba_scroll_hor( rgba_t *pixels, int xs, int ys, int lstep, int s, rgba_t bg );
void rgba_fill_line( rgba_t *pixels, int xs, rgba_t bg );



#endif // COLOR_H

