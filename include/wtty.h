/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Window subsystem 'controlling terminal' data structures
 *
 *
**/

#ifndef WTTY_H
#define WTTY_H

#include <phantom_types.h>
#include <hal.h>

#define WTTY_BUFSIZE 128

// See 'tid_t owner' field of window to understand how key events reach some thread

struct wtty
{
	char			buf[WTTY_BUFSIZE];
	int			putpos;
	int			getpos;
	hal_mutex_t		mutex;
	hal_cond_t		cond;
};

typedef struct wtty wtty_t;


errno_t 	wtty_putc_nowait(wtty_t *w, int c);
int 		wtty_getc(wtty_t *w);
int             wtty_is_empty(wtty_t *w);

wtty_t * 	wtty_init(void);


#endif // WTTY_H
