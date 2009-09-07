#include <GL/gl.h>
#include "zgl.h"
#include <drv_video_screen.h>

typedef struct {
    GLContext *gl_context;
    //void *gl_context;
    int xsize,ysize;
    //GR_DRAW_ID drawable;
    //GR_GC_ID gc;
    drv_video_window_t *w;
    //int pixtype; /* pixel type in TinyGL */
} TinyPhantomContext;


TinyPhantomContext *PhantomCreateContext(int flags);

void PhantomSwapBuffers( drv_video_window_t * drawable );

int PhantomMakeCurrent( drv_video_window_t * drawable,
                        TinyPhantomContext *ctx);

