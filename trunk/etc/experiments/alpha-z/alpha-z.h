#include <stdint.h>

struct rgba_t {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

typedef uint32_t zbuf_t;
typedef int32_t int32x2 __attribute__((vector_size(8)));
typedef int32_t int32x4 __attribute__((vector_size(16)));
typedef uint32_t uint32x4 __attribute__((vector_size(16)));

extern void rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos);
extern void mmx_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos);
extern void sse_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos);
extern void sseu_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos);
extern void auto_rgba2rgba_zbmove(struct rgba_t *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos);
