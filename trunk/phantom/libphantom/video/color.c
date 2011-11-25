#if 1

#include <video/color.h>
#include <math.h>

/*
rgba_t Hsi2Rgb(double H, double S, double I )
{
    double T;
    double Rv, Gv, Bv;

    T = H;
    Rv = 1 + S * sin(T - 2 * M_PI / 3);
    Gv = 1 + S * sin(T);
    Bv = 1 + S * sin(T + 2 * M_PI / 3);

    T = 63.999 * I / 2;

    rgba_t C;

    C.a = 0xFF;

    C.r = (int)(Rv * T);
    C.g = (int)(Gv * T);
    C.b = (int)(Bv * T);

    return C;
}

*/
struct rgba_t COLOR_TRANSPARENT = { 0, 0, 0, 0 };

struct rgba_t COLOR_BLACK = { 0, 0, 0, 0xFF };
struct rgba_t COLOR_WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };

struct rgba_t COLOR_RED = { 0, 0, 0x80, 0xFF };
struct rgba_t COLOR_LIGHTRED = { 0x40, 0x40, 0xFF, 0xFF };

struct rgba_t COLOR_GREEN = { 0, 0x80, 0, 0xFF };
struct rgba_t COLOR_LIGHTGREEN = { 0x40, 0xFF, 0x40, 0xFF };

struct rgba_t COLOR_BLUE = { 0x80, 0, 0, 0xFF };
struct rgba_t COLOR_LIGHTBLUE = { 0xFF, 0x40, 0x40, 0xFF };

struct rgba_t COLOR_DARKGRAY = { 0x40, 0x40, 0x40, 0xFF };
struct rgba_t COLOR_LIGHTGRAY = { 0x80, 0x80, 0x80, 0xFF };

//struct rgba_t COLOR_LIGHTRED = { 0x40, 0x40, 0xFF, 0xFF };
struct rgba_t COLOR_YELLOW = { 0x00, 0xFF, 0xFF, 0xFF };


#endif
