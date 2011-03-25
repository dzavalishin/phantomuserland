#ifndef RINGBUF_H
#define RINGBUF_H

typedef struct ringbuf
{
    unsigned int 	getindex;
    unsigned int 	putindex;
    unsigned int 	used;
    unsigned int 	total;
    unsigned char 	ringbuffer[];
} ringbuf_t;


int ring_put( ringbuf_t *r, char c )
{
    if ( r->used >= r->total )
        return -1;

    r->ringbuffer[ r->putindex++ ] = c;

    if( r->putindex >= r->total )
        r->putindex = 0;

    r->used++;

    return 0;
}

int ring_get( ringbuf_t *r )
{
    if ( !r->used )
        return -1;

    char c;

    r->used--;
    c = r->ringbuffer[ r->getindex++ ];

    if ( r->getindex >= r->total )
        r->getindex = 0;
    return c;
}

int ring_empty( ringbuf_t *r )
{
    return !r->used;
}

int ring_full( ringbuf_t *r )
{
    return r->used >= r->total;
}

ringbuf_t * ring_alloc( int size )
{
    ringbuf_t *r = calloc(1, sizeof(ringbuf_t) + size );

    r->getindex = 0;
    r->putindex = 0;
    r->used = 0;
    r->total = size;

    return r;
}

void ring_free( ringbuf_t *r )
{
    free(r);
}

#endif // RINGBUF_H
