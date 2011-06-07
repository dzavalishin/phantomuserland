/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Generic rgba_t operations.
 *
**/

#ifndef ZBUF_H
#define ZBUF_H

typedef u_int8_t zbuf_t;
//typedef u_int32_t zbuf_t;
extern zbuf_t *zbuf;

//! hack - switches zbuf to be top->bottom or vice versa
void video_zbuf_turn_upside(int v);


#endif // ZBUF_H
