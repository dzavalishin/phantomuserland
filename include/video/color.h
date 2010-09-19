#ifndef COLOR_H
#define COLOR_H

#include <sys/cdefs.h>

// Windows 'screen' driver works in BGR format :(
#define BGR 1


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

#endif // COLOR_H

