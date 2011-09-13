/* simple glx like driver for TinyGL and Nano X */
#include <GL/gl.h>
#include "zgl.h"

#include <stdlib.h>

#include "tinyphantom.h"



TinyPhantomContext *PhantomCreateContext(int flags)
{
    TinyPhantomContext *ctx;

    ctx=gl_malloc(sizeof(TinyPhantomContext));
    if (!ctx)
        return NULL;
    ctx->gl_context=NULL;
    return ctx;
}

/*
 void glXDestroyContext( TinyPhantomContext ctx )
 {
 if (ctx->gl_context != NULL) {
 glClose();
 }
 gl_free(ctx);
 }
 */

/* resize the glx viewport : we try to use the xsize and ysize
 given. We return the effective size which is guaranted to be smaller */

static int glX_resize_viewport(GLContext *c,int *xsize_ptr,int *ysize_ptr)
{
    TinyPhantomContext *ctx;
    int xsize,ysize;

    ctx=(TinyPhantomContext *)c->opaque;

    xsize=*xsize_ptr;
    ysize=*ysize_ptr;

    /* we ensure that xsize and ysize are multiples of 2 for the zbuffer.
     TODO: find a better solution */
    xsize&=~3;
    ysize&=~3;

    if (xsize == 0 || ysize == 0) return -1;

    if (xsize > ctx->w->xsize || ysize > ctx->w->ysize) return -1;

    *xsize_ptr=xsize;
    *ysize_ptr=ysize;

    ctx->xsize=xsize;
    ctx->ysize=ysize;

    /* resize the Z buffer */
    ZB_resize(c->zb,NULL,xsize,ysize);
    return 0;
}



/* we assume here that drawable is a window */
int PhantomMakeCurrent( window_handle_t drawable, TinyPhantomContext *ctx )
{
    int mode, xsize, ysize;
    ZBuffer *zb;
    //GR_WINDOW_INFO win_info;

    if (ctx->gl_context == NULL) {
        /* create the TinyGL context */
        //GrGetWindowInfo(drawable, &win_info);

        //xsize = 400; //win_info.width;
        //ysize = 300; //win_info.height;

        xsize = drawable->xsize;
        ysize = drawable->ysize;

        /* currently, we only support 16 bit rendering */
        mode = ZB_MODE_5R6G5B;
        zb=ZB_open(xsize,ysize,mode,0,NULL,NULL,NULL);
        if (zb == NULL) {
            //fprintf(stderr, "Error while initializing Z buffer\n");
            printf( "Error while initializing Z buffer\n");
            exit(1);
        }

        //ctx->pixtype = MWPF_TRUECOLOR565;

        /* create a gc */
        //ctx->gc = GrNewGC();
        ctx->w = drawable;

        /* initialisation of the TinyGL interpreter */
        glInit(zb);
        ctx->gl_context=gl_get_context();
        ctx->gl_context->opaque=(void *) ctx;
        ctx->gl_context->gl_resize_viewport=glX_resize_viewport;

        /* set the viewport : we force a call to glX_resize_viewport */
        ctx->gl_context->viewport.xsize=-1;
        ctx->gl_context->viewport.ysize=-1;

        glViewport(0, 0, xsize, ysize);
    }

    return 1;
}

void PhantomSwapBuffers( window_handle_t drawable )
{
    GLContext *gl_context;
    TinyPhantomContext *ctx;

    /* retrieve the current NGLXContext */
    gl_context=gl_get_context();
    ctx=(TinyPhantomContext *)gl_context->opaque;

    //GrArea(drawable, ctx->gc, 0, 0, ctx->xsize, ctx->ysize, ctx->gl_context->zb->pbuf, ctx->pixtype);

    int565_to_rgba_move( ctx->w->pixel, (void *)ctx->gl_context->zb->pbuf, ctx->w->xsize*ctx->w->ysize );
//printf("blit ");
    drv_video_winblt( ctx->w );
}


