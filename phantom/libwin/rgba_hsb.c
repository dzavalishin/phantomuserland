/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Color conversion
 *
 *
**/

#include <video/color.h>
#include <math.h>


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

