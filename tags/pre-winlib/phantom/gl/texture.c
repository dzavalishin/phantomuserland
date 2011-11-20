/*
 * Texture Manager
 */

#include "zgl.h"

static GLTexture *find_texture(GLContext *c,int h)
{
    GLTexture *t;

    t=c->shared_state.texture_hash_table[h % TEXTURE_HASH_TABLE_SIZE];
    while (t!=NULL) {
        if (t->handle == h) return t;
        t=t->next;
    }
    return NULL;
}

static void free_texture(GLContext *c,int h)
{
    GLTexture *t,**ht;
    GLImage *im;
    int i;

    t=find_texture(c,h);
    if (t->prev==NULL) {
        ht=&c->shared_state.texture_hash_table
            [t->handle % TEXTURE_HASH_TABLE_SIZE];
        *ht=t->next;
    } else {
        t->prev->next=t->next;
    }
    if (t->next!=NULL) t->next->prev=t->prev;

    for(i=0;i<MAX_TEXTURE_LEVELS;i++) {
        im=&t->images[i];
        if (im->pixmap != NULL) gl_free(im->pixmap);
    }

    gl_free(t);
}

GLTexture *alloc_texture(GLContext *c,int h)
{
    GLTexture *t,**ht;

    t=gl_zalloc(sizeof(GLTexture));

    ht=&c->shared_state.texture_hash_table[h % TEXTURE_HASH_TABLE_SIZE];

    t->next=*ht;
    t->prev=NULL;
    if (t->next != NULL) t->next->prev=t;
    *ht=t;

    t->handle=h;

    return t;
}


void glInitTextures(GLContext *c)
{
    /* textures */

    c->texture_2d_enabled=0;
    c->current_texture=find_texture(c,0);
}

void glGenTextures(int n, unsigned int *textures)
{
    GLContext *c=gl_get_context();
    int max,i;
    GLTexture *t;

    max=0;
    for(i=0;i<TEXTURE_HASH_TABLE_SIZE;i++) {
        t=c->shared_state.texture_hash_table[i];
        while (t!=NULL) {
            if (t->handle>max) max=t->handle;
            t=t->next;
        }

    }
    for(i=0;i<n;i++) {
        textures[i]=max+i+1;
    }
}


void glDeleteTextures(int n, const unsigned int *textures)
{
    GLContext *c=gl_get_context();
    int i;
    GLTexture *t;

    for(i=0;i<n;i++) {
        t=find_texture(c,textures[i]);
        if (t!=NULL && t!=0) {
            if (t==c->current_texture) {
                glBindTexture(GL_TEXTURE_2D,0);
            }
            free_texture(c,textures[i]);
        }
    }
}


void glopBindTexture(GLContext *c,GLParam *p)
{
    int target=p[1].i;
    int texture=p[2].i;
    GLTexture *t;

    assert(target == GL_TEXTURE_2D && texture >= 0);

    t=find_texture(c,texture);
    if (t==NULL) {
        t=alloc_texture(c,texture);
    }
    c->current_texture=t;
}

#if 1
#define NEWXS 256
#define NEWYS 256
#else
#define NEWXS 512
#define NEWYS 512
#endif

void glopTexImage2D(GLContext *c,GLParam *p)
{
    int target=p[1].i;
    int level=p[2].i;
    int components=p[3].i;
    int width=p[4].i;
    int height=p[5].i;
    int border=p[6].i;
    int format=p[7].i;
    int type=p[8].i;
    void *pixels=p[9].p;
    GLImage *im;
    int do_free;

    int isRGBA = (format == GL_RGBA);

#if 0
    if (target == GL_TEXTURE_2D && level == 0 && components == 4 &&
          border == 0 && format == GL_RGBA &&
          type == GL_UNSIGNED_BYTE) {

        im=&c->current_texture->images[level];
        im->xsize=width;
        im->ysize=height;
        if (im->pixmap!=NULL) gl_free(im->pixmap);


#if TGL_FEATURE_RENDER_BITS == 32
        im->pixmap=gl_malloc(width*height*4);
        if(im->pixmap) {
            gl_convertRGBA_to_8A8R8G8B(im->pixmap,pixels,width,height);
        }

#elif TGL_FEATURE_RENDER_BITS == 16
        im->pixmap=gl_malloc(width*height*2);
        if(im->pixmap) {
            gl_convertRGBA_to_5R6G5B(im->pixmap,pixels,width,height);
        }
#else
#error 32 or 16
#endif

        return;
    }
#endif

    unsigned char *pixels1;

    if (!(target == GL_TEXTURE_2D && level == 0 && (components == 3 || components == 4) &&
          border == 0 && (format == GL_RGB || format == GL_RGBA ) &&
          type == GL_UNSIGNED_BYTE)) {
        gl_fatal_error("glTexImage2D: combinaison of parameters not handled");
    }

    do_free=0;
    if (width != NEWXS || height != NEWYS) {
        pixels1 = gl_malloc(NEWXS * NEWYS * 3);
        /* no interpolation is done here to respect the original image aliasing ! */
        if(isRGBA)
            gl_resizeImageNoInterpolate_rgba2rgb(pixels1,NEWXS,NEWYS,pixels,width,height);
        else
            gl_resizeImageNoInterpolate(pixels1,NEWXS,NEWYS,pixels,width,height);


        do_free=1;
        width=NEWXS;
        height=NEWYS;
    } else {
        pixels1=pixels;
    }

    im=&c->current_texture->images[level];
    im->xsize=width;
    im->ysize=height;
    if (im->pixmap!=NULL) gl_free(im->pixmap);
#if TGL_FEATURE_RENDER_BITS == 24
    im->pixmap=gl_malloc(width*height*3);
    if(im->pixmap) {
        memcpy(im->pixmap,pixels1,width*height*3);
    }
#elif TGL_FEATURE_RENDER_BITS == 32
    im->pixmap=gl_malloc(width*height*4);
    if(im->pixmap) {
        gl_convertRGB_to_8A8R8G8B(im->pixmap,pixels1,width,height);
    }
#elif TGL_FEATURE_RENDER_BITS == 16
    im->pixmap=gl_malloc(width*height*2);
    if(im->pixmap) {
        gl_convertRGB_to_5R6G5B(im->pixmap,pixels1,width,height);
    }
#else
#error TODO
#endif
    if (do_free) gl_free(pixels1);
}


/* TODO: not all tests are done */
void glopTexEnv(GLContext *c,GLParam *p)
{
    int target=p[1].i;
    int pname=p[2].i;
    int param=p[3].i;

    if (target != GL_TEXTURE_ENV) {
    error:
        gl_fatal_error("glTexParameter: unsupported option");
    }

    if (pname != GL_TEXTURE_ENV_MODE) goto error;

    if (param != GL_DECAL) goto error;
}

/* TODO: not all tests are done */
void glopTexParameter(GLContext *c,GLParam *p)
{
    int target=p[1].i;
    int pname=p[2].i;
    int param=p[3].i;

    if (target != GL_TEXTURE_2D) {
    error:
        gl_fatal_error("glTexParameter: unsupported option");
    }

    switch(pname) {
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
        if (param != GL_REPEAT) goto error;
        break;
    }
}

void glopPixelStore(GLContext *c,GLParam *p)
{
    int pname=p[1].i;
    int param=p[2].i;

    if (pname != GL_UNPACK_ALIGNMENT ||
        param != 1) {
        gl_fatal_error("glPixelStore: unsupported option");
    }
}
