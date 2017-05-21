/**
 *
 * Phantom OS
 *
 * Character buffer implementation.
 *
 * (C) 2005-2016 dz.
 *
**/



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

// -----------------------------------------------------------------------
// Internal
// -----------------------------------------------------------------------


static __inline__ void wtty_wrap(wtty_t *w)
{
    assert(w);

    if( w->getpos >= WTTY_BUFSIZE )        w->getpos = 0;
    if( w->putpos >= WTTY_BUFSIZE )        w->putpos = 0;
}

static __inline__ void wtty_doputc(wtty_t *w, int c)
{
    w->buf[w->putpos++] = c;
    hal_cond_broadcast( &w->rcond );
}

// -----------------------------------------------------------------------
// Info
// -----------------------------------------------------------------------

static __inline__ int _wtty_is_empty(wtty_t *w)
{
    return w->putpos == w->getpos;
}


int wtty_is_empty(wtty_t *w)
{
    assert(w);
    return w->getpos == w->putpos;
}



static __inline__ int _wtty_is_full(wtty_t *w)
{
    return (w->putpos+1 == w->getpos) || ( w->putpos+1 >= WTTY_BUFSIZE && w->getpos == 0 );
}


int wtty_is_full(wtty_t *w)
{
    wtty_wrap(w);
    assert(w);
    return _wtty_is_full(w);
}

// -----------------------------------------------------------------------
// Read
// -----------------------------------------------------------------------


int wtty_getc(wtty_t *w)
{
    int ret;

    assert(w);
    SHOW_FLOW( 11, "wtty getc %p", w );

    hal_mutex_lock(&w->mutex);

    if(!w->started) { ret = 0; goto exit; }

    while(w->getpos == w->putpos)
    {
        hal_cond_wait( &w->rcond, &w->mutex );
        if(!w->started) { ret = 0; goto exit; }
    }

    wtty_wrap(w);

    ret = w->buf[w->getpos++];
    hal_cond_broadcast( &w->wcond ); // signal writers to continue

exit:
    hal_mutex_unlock(&w->mutex);
    return ret;
}


int wtty_read(wtty_t *w, char *data, int cnt, bool nowait)
{
    int done = 0;
    assert(w);
    hal_mutex_lock(&w->mutex);

    SHOW_FLOW( 11, "wtty rd %p", w );
    if(!w->started) { done = -EPIPE; goto exit; }

    while( cnt > 0 )
    {
        if( nowait && _wtty_is_empty(w) )
            break;

        if(!w->started) goto exit;

        while( _wtty_is_empty(w) )
        {
            hal_cond_broadcast( &w->wcond );
            hal_cond_wait( &w->rcond, &w->mutex );
            if(!w->started) goto exit;
        }

        *data++ = w->buf[w->getpos++];
        done++;
        cnt--;
        wtty_wrap(w);
    }

    hal_cond_broadcast( &w->wcond );
exit:
    hal_mutex_unlock(&w->mutex);
    return done;
}







// -----------------------------------------------------------------------
// Write
// -----------------------------------------------------------------------







errno_t wtty_putc(wtty_t *w, int c)
{
    errno_t ret = 0;
    assert(w);

    hal_mutex_lock(&w->mutex);
    wtty_wrap(w);

    SHOW_FLOW( 11, "wtty putc %p", w );
    if(!w->started) { ret = EPIPE; goto exit; }

    while( _wtty_is_full(w) )
    {
        hal_cond_wait( &w->wcond, &w->mutex );
        if(!w->started) { ret = EPIPE; goto exit; }
    }

    wtty_doputc(w, c);
exit:
    hal_mutex_unlock(&w->mutex);
    return ret;
}


errno_t wtty_putc_nowait(wtty_t *w, int c)
{
    int ret = 0;

    assert(w);
    if(!w->started) return EPIPE;

    hal_mutex_lock(&w->mutex);
    wtty_wrap(w);

    SHOW_FLOW( 11, "wtty putc %p", w );

    if( _wtty_is_full(w) )
    {
        SHOW_ERROR0( 10, "wtty putc fail" );
        ret = ENOMEM;
    }
    else
    {
        wtty_doputc(w, c);
    }

    hal_mutex_unlock(&w->mutex);
    return ret;
}

int wtty_write(wtty_t *w, const char *data, int cnt, bool nowait)
{
    int done = 0;

    assert(w);
    if(!w->started) return -EPIPE;

    hal_mutex_lock(&w->mutex);
    wtty_wrap(w);

    SHOW_FLOW( 11, "wtty wr %p", w );

    while( cnt > 0 )
    {
        if( nowait && _wtty_is_full(w) )
            break;

        while( _wtty_is_full(w) )
        {
            hal_cond_broadcast( &w->rcond );
            hal_cond_wait( &w->wcond, &w->mutex );
            if(!w->started) goto exit;
        }

        w->buf[w->putpos++] = *data++;
        done++;
        cnt--;
        wtty_wrap(w);
    }

    hal_cond_broadcast( &w->rcond );
exit:
    hal_mutex_unlock(&w->mutex);
    return done;
}



// -----------------------------------------------------------------------
// Empty
// -----------------------------------------------------------------------


void wtty_clear(wtty_t * w)
{
    assert(w);
    hal_mutex_lock(&w->mutex);
    w->getpos = 0;
    w->putpos = 0;
    hal_cond_broadcast( &w->wcond );
    hal_mutex_unlock(&w->mutex);
}


// -----------------------------------------------------------------------
// Dump
// -----------------------------------------------------------------------


void wtty_dump( wtty_t * w )
{
    assert(w);
    printf("wtty putpos %d, getpos %d\n", w->putpos, w->getpos );

}



// -----------------------------------------------------------------------
// Init/kill
// -----------------------------------------------------------------------


wtty_t * wtty_init(void)
{
    wtty_t *w = calloc( 1, sizeof(wtty_t) );
    assert(w);

#if CONF_WTTY_SIZE
    size_t wsize = WTTY_BUFSIZE; // make to be param, check
    if( wsize < WTTY_MIN_BUFSIZE ) wsize = WTTY_MIN_BUFSIZE;

    w->buf = calloc( 1, wsize );
#endif

    //w->getpos = 0;
    //w->putpos = 0;
    hal_mutex_init( &w->mutex, "wtty" );
    hal_cond_init( &w->rcond, "wtty.r" );
    hal_cond_init( &w->wcond, "wtty.w" );

    w->started = 1;

    return w;
}

// all blocked calls return
void wtty_stop(wtty_t * w)
{
    w->started = 0;
    hal_cond_broadcast( &w->rcond );
    hal_cond_broadcast( &w->wcond );
}

void wtty_start(wtty_t * w)
{
    w->started = 1;
}

int wtty_is_started(wtty_t *w)
{
    return w->started;
}


void wtty_destroy(wtty_t * w)
{
    //wtty_stop(w); // not really good idea
    // Check unstopped
    if( w->started ) SHOW_ERROR0( 0, "unstopped" );
    hal_mutex_destroy( &w->mutex );
    hal_cond_destroy( &w->rcond );
    hal_cond_destroy( &w->wcond );
    free(w);
#if CONF_WTTY_SIZE
    free(w->buf);
#endif
}

