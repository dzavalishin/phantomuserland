/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Window subsystem 'controlling terminal' data structures. Actually just a simple byte FIFO.
 *
 *
**/

#ifndef WTTY_H
#define WTTY_H


#include <phantom_types.h>
#include <hal.h>

#define CONF_WTTY_SIZE 1

// Preset buffer sizes to use as _init() parameter

#define WTTY_SMALL_BUF 128



/** \ingroup Threads
 *  \defgroup Threads Window subsystem 'controlling terminal' data structures
 *  @{
 */


// See 'tid_t owner' field of window to understand how key events reach some thread

#if CONF_WTTY_SIZE

#define WTTY_MIN_BUFSIZE 128
#define WTTY_DEFAULT_BUFSIZE 5120

#define WTTY_BUFSIZE  (w->size)

struct wtty
{
    char            *buf;

    size_t          size;

    int	            putpos;
    int             getpos;

    hal_mutex_t     mutex;
    hal_cond_t      rcond; // reader sleeps here
    hal_cond_t      wcond;

    bool            started;
};

#else // CONF_WTTY_SIZE

//#define WTTY_BUFSIZE 128
#define WTTY_BUFSIZE 5120


struct wtty
{
    char            buf[WTTY_BUFSIZE];

    int	            putpos;
    int             getpos;

    hal_mutex_t     mutex;
    hal_cond_t      rcond; // reader sleeps here
    hal_cond_t      wcond;

    bool            started;
};

#endif


typedef struct wtty wtty_t;


errno_t         wtty_putc(wtty_t *w, int c);
errno_t         wtty_putc_nowait(wtty_t *w, int c);
int             wtty_write(wtty_t *w, const char *data, int cnt, bool nowait);

int 	        wtty_getc(wtty_t *w);
int             wtty_read(wtty_t *w, char *data, int cnt, bool nowait);

int             wtty_is_empty(wtty_t *w);
int             wtty_is_full(wtty_t *w);

void            wtty_clear(wtty_t * w);

void            wtty_stop(wtty_t * w);          // all blocked calls return EPIPE/-EPIPE/ 0 (getc)
void            wtty_start(wtty_t * w);
int             wtty_is_started(wtty_t *w);

wtty_t *        wtty_init( size_t bufferSize );
//wtty_t *        wtty_init_ext(void);
void            wtty_destroy(wtty_t * w);

void            wtty_dump( wtty_t * w );

/** @} */

#endif // WTTY_H
