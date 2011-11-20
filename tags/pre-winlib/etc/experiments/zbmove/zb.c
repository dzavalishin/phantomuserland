typedef unsigned zbuf_t;

struct rgba_t
{
    unsigned char       b;
    unsigned char       g;
    unsigned char       r;
    unsigned char       a;      // Alpha
} __packed;


void rgba2rgba_zbmove( struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos )
{
    while(nelem-- > 0)
    {
        if(src->a && (*zb <= zpos) )
        {
            *zb = zpos;
            *dest = *src;
        }

        dest++;
        src++;
        zb++;
    }
}
