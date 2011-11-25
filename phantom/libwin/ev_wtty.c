

#define DEBUG_MSG_PREFIX "wtty"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <phantom_libc.h>
#include <malloc.h>

#include <kernel/mutex.h>
#include <kernel/cond.h>
#include <wtty.h>


static void wtty_wrap(wtty_t *w)
{
    if( w->getpos >= WTTY_BUFSIZE )        w->getpos = 0;
    if( w->putpos >= WTTY_BUFSIZE )        w->putpos = 0;
}


int wtty_getc(wtty_t *w)
{
    int ret;

    SHOW_FLOW( 11, "wtty getc %p", w );

    hal_mutex_lock(&w->mutex);

    while(w->getpos == w->putpos)
        hal_cond_wait( &w->cond, &w->mutex );

    wtty_wrap(w);

    ret = w->buf[w->getpos++];

    hal_mutex_unlock(&w->mutex);

    return ret;
}

int wtty_is_empty(wtty_t *w)
{
    return (w->getpos == w->putpos);
}


/*
void wtty_putc(wtty_t *w, int c)
{
    hal_mutex_lock(&w->mutex);
    wtty_wrap(w);

    hal_mutex_unlock(&w->mutex);
}
*/

errno_t wtty_putc_nowait(wtty_t *w, int c)
{
    int ret = 0;
    hal_mutex_lock(&w->mutex);
    wtty_wrap(w);

    SHOW_FLOW( 11, "wtty putc %p", w );

    if( (w->putpos+1 == w->getpos) || ( w->putpos+1 >= WTTY_BUFSIZE && w->getpos == 0 ) )
    {
        SHOW_ERROR0( 10, "wtty putc fail" );
        ret = ENOMEM;
    }
    else
    {
        w->buf[w->putpos++] = c;
        hal_cond_signal( &w->cond );
    }

    hal_mutex_unlock(&w->mutex);
    return ret;
}


wtty_t * wtty_init(void)
{
    wtty_t *w = calloc( 1, sizeof(wtty_t) );
    assert(w);

    //w->getpos = 0;
    //w->putpos = 0;
    hal_mutex_init( &w->mutex, "wtty" );
    hal_cond_init( &w->cond, "wtty" );

    return w;
}


