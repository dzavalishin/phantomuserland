#include <GL/gl.h>
#include "zgl.h"
//#include <drv_video_screen.h>
#include <video/window.h>
#include <video/internal.h>
#include <video/vops.h>

typedef struct {
    GLContext *gl_context;
    //void *gl_context;
    int xsize,ysize;
    //GR_DRAW_ID drawable;
    //GR_GC_ID gc;
    window_handle_t w;
    //int pixtype; /* pixel type in TinyGL */
} TinyPhantomContext;


TinyPhantomContext *PhantomCreateContext(int flags);

void PhantomSwapBuffers( window_handle_t drawable );

int PhantomMakeCurrent( window_handle_t drawable,
                        TinyPhantomContext *ctx);

