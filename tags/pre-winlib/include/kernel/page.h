/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Pagesize-related macros.
 *
 *
**/

#ifndef PAGE_H
#define PAGE_H

#include <arch/arch-page.h>

#define PAGE_ALIGN(x) (((x) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1))
#define PAGE_ALIGNED(x) ( 0 == ( (x)  & (PAGE_SIZE-1) ))

#define BYTES_TO_PAGES(x) ((((x)-1)/PAGE_SIZE)+1)

// Align to address of page which contains addr x
#define PREV_PAGE_ALIGN(x) ((x) & ~(PAGE_SIZE-1))

#define ALIGN_AT(x,___how) (((x) + ((___how)-1)) & ~((___how)-1))

#endif // PAGE_H
