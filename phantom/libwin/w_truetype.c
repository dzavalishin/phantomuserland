/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Truetype fonts support.
 *
 * Wrapper for FreeType library.
 *
**/

#if CONF_TRUETYPE

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define DEBUG_MSG_PREFIX "w.ttf"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <stdint.h>

#include <utf8proc.h>

#include <video/screen.h>
#include <video/internal.h>
#include <video/font.h>

#include <kernel/init.h>
#include <kernel/debug.h>
#include <kernel/pool.h>

#include <errno.h>


#define CACHE_FT_FACE 0


static FT_Library ftLibrary = 0;
static int running = 0;

static pool_t   *tt_font_pool;

static hal_mutex_t faces_mutex;

//static void * 	do_ttf_create(void *arg);
static void  	do_ttf_destroy(void *arg);


struct ttf_pool_el
{
    //char          font_type; // enum font_types_t { ft_binmap, ft_truetype };
    const char *    font_name;
    int 	    font_size; // pixels
    FT_Face	    face;
    pool_handle_t   fhandle;
};




// NB!! FT_New_Face and FT_Done_Face are not thread safe!

//static
void init_truetype(void)
{
    hal_mutex_init( &faces_mutex, "faces_mutex" );

    int rc = FT_Init_FreeType(&ftLibrary);
    if( rc )
    {
        lprintf("Error starting truetype: %d\n", rc );
        return;
    }

    tt_font_pool = create_pool();

    //tt_font_pool->init    = do_ttf_create;
    tt_font_pool->destroy = do_ttf_destroy;

    tt_font_pool->flag_autodestroy = 1;

    running = 1;
}

//INIT_ME(0,init_truetype,0)


static int w_load_tt_from_file( FT_Face *ftFace, const char *file_name );

pool_handle_t w_get_tt_font_mem( void *mem_font, size_t mem_font_size, const char* diag_font_name, int font_size );
pool_handle_t w_get_tt_font_file( const char *file_name, int size );

static pool_handle_t w_store_tt_to_pool( struct ttf_pool_el *req );




const size_t MAX_SYMBOLS_COUNT = 256;


FT_Glyph getGlyph(FT_Face face, uint32_t charcode);
FT_Pos getKerning(FT_Face face, uint32_t leftCharcode, uint32_t rightCharcode);
 
struct ttf_symbol
{
    int32_t posX;
    int32_t posY;
    int32_t width;
    int32_t height;
    FT_Glyph glyph;
};
 
 
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define W_BLEND_PIXEL( old, new, newalpha ) \
    ((unsigned char) \
      ( \
        ( ((unsigned char)(new)) * (newalpha) ) \
        + \
        ( ((unsigned char)(old)) * (1.0f - (newalpha)) ) \
      )\
    )




void w_ttfont_draw_string(
                          window_handle_t win,
                          pool_handle_t font,
                          const char *str, const rgba_t color, const rgba_t bg,
                          int win_x, int win_y )
{

    if(!running) return;

    int rc;
    /*
    FT_Face ftFace = 0;
    rc = FT_New_Face(ftLibrary, "P:/phantomuserland/plib/resources/ttfonts/opensans/OpenSans-Regular.ttf", 0, &ftFace);
    if( rc )
    {
        lprintf("\ncan't load font\n");
        return;
    }
 
    FT_Set_Pixel_Sizes(ftFace, 200, 0);
    //FT_Set_Pixel_Sizes(ftFace, 100, 0);
    //FT_Set_Pixel_Sizes(ftFace, 50, 0);
    */

    struct ttf_pool_el *pe = pool_get_el( tt_font_pool, font );
    if( 0 == pe )
    {
        lprintf("\ncan't get font for handle %x\n", font);
        return;
    }
    printf( "w_ttfont_draw_string f '%s' sz %d\n", pe->font_name, pe->font_size );

    FT_Face ftFace = pe->face;;

    //const char *str = s;
    const size_t strLen = strlen(str);
 
    struct ttf_symbol symbols[MAX_SYMBOLS_COUNT];
    size_t numSymbols = 0;
 
    int32_t left = INT_MAX;
    int32_t top = INT_MAX;
    int32_t bottom = INT_MIN;
    uint32_t prevCharcode = 0;
 
    int32_t posX = 0;
    int skip_total = 0;
 
    while( (skip_total < strLen) && (numSymbols < MAX_SYMBOLS_COUNT))
    {
        //const uint32_t charcode = str[i];
        int32_t charcode;
        ssize_t skip = utf8proc_iterate( (const uint8_t *)(skip_total+str), strLen, &charcode);

        skip_total += skip;

 
        FT_Glyph glyph = getGlyph(ftFace, charcode);
 
        if (!glyph)
            continue;
 
        if (prevCharcode)
            posX += getKerning(ftFace, prevCharcode, charcode);

        prevCharcode = charcode;
 
        struct ttf_symbol *symb = &(symbols[numSymbols++]);
 
        FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph) glyph;
        symb->posX = (posX >> 6) + bitmapGlyph->left;
        symb->posY = -bitmapGlyph->top;
        symb->width = bitmapGlyph->bitmap.width;
        symb->height = bitmapGlyph->bitmap.rows;
        symb->glyph = glyph;
 
        posX += glyph->advance.x >> 10;
 
        left = MIN(left, symb->posX);
        top = MIN(top, symb->posY);
        bottom = MAX(bottom, symb->posY + symb->height);
    }
 
    size_t i = 0;
    for (i = 0; i < numSymbols; ++i)
    {
        symbols[i].posX -= left;
    }
 
    //const struct ttf_symbol *lastSymbol = &(symbols[numSymbols - 1]);
    //const int32_t imageW = lastSymbol->posX + lastSymbol->width;
    //const int32_t imageH = bottom - top;
 
 
    for (i = 0; i < numSymbols; ++i)
    {
        const struct ttf_symbol *symb = symbols + i;
 
        FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph) symb->glyph;
        FT_Bitmap bitmap = bitmapGlyph->bitmap;

        int32_t srcY;
        for(srcY = 0; srcY < symb->height; ++srcY)
        {
            const int32_t dstY = symb->posY + srcY - top;

            // we need not total image height, but height above baseline
            //int wy = win_y + imageH - dstY;
            int wy = win_y + (-top) - dstY; 

            if( wy < 0 ) continue; // break? we go down
            if( wy >= win->ysize ) continue;

            int _wy = wy * win->xsize;

            int32_t srcX;
            for (srcX = 0; srcX < symb->width; ++srcX)
            {
                const uint8_t c = bitmap.buffer[srcX + srcY * bitmap.pitch];
 
                if (0 == c)
                {
                    continue;
                }
 
                const float a = c / 255.0f;
                const int32_t dstX = symb->posX + srcX;

                int wx = win_x + dstX;

                if( wx < 0 ) continue; 
                if( wx >= win->xsize ) break;

                rgba_t old = win->w_pixel[wx+_wy];
                rgba_t *p = &(win->w_pixel[wx+_wy]);

                p->r = W_BLEND_PIXEL( old.r, color.r, a );
                p->g = W_BLEND_PIXEL( old.g, color.g, a );
                p->b = W_BLEND_PIXEL( old.b, color.b, a );

                p->a = 0xFF;

                //win->w_pixel[wx+_wy] = color;
            }
        }
    }
 
 
    for (i = 0; i < numSymbols; ++i)
    {
        FT_Done_Glyph(symbols[i].glyph);
    }

    rc = pool_release_el( tt_font_pool, font );
    if( rc )
        lprintf("\ncan't release font for handle %x\n", font);

/*
    FT_Done_Face(ftFace);
    ftFace = 0;
 
    FT_Done_FreeType(ftLibrary);
    ftLibrary = 0;
*/
}
 
 
FT_Glyph getGlyph(FT_Face face, uint32_t charcode)
{
    FT_Load_Char(face, charcode, FT_LOAD_RENDER);
    FT_Glyph glyph = 0;
    FT_Get_Glyph(face->glyph, &glyph);
    return glyph;
}
 
 
FT_Pos getKerning(FT_Face face, uint32_t leftCharcode, uint32_t rightCharcode)
{
    FT_UInt leftIndex = FT_Get_Char_Index(face, leftCharcode);
    FT_UInt rightIndex = FT_Get_Char_Index(face, rightCharcode);
 
    FT_Vector delta;
    FT_Get_Kerning(face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &delta);
    return delta.x;
}




// -----------------------------------------------------------------------
//
// Fonts pool
//
// -----------------------------------------------------------------------



/**
 * 
 * Called by pool code to create a new pool element.
 * 
** /
static void * 	do_ttf_create(void *arg)
{
    return arg;
} */

/**
 * 
 * Called by pool code to destroy pool element.
 * 
**/
static void  	do_ttf_destroy(void *arg)
{
    struct ttf_pool_el *cur = arg;

    lprintf("\ndestroy tt font %s sz %d\n", cur->font_name, cur->font_size );

    hal_mutex_lock( &faces_mutex );
    FT_Done_Face( cur->face );
    hal_mutex_unlock( &faces_mutex );

    free((void *)cur->font_name);
    free( arg );
}


#if CACHE_FT_FACE
static errno_t tt_lookup(pool_t *pool, void *el, pool_handle_t handle, void *arg)
{
    struct ttf_pool_el *req = arg;
    struct ttf_pool_el *cur = el;

    if( (0 == strcmp( req->font_name, cur->font_name )) && (req->font_size == cur->font_size) )
    {
        req->face = cur->face;
        req->fhandle = handle;
        return 1; // found
    }

    return 0;
}
#endif // CACHE_FT_FACE

pool_handle_t w_get_tt_font_file( const char *file_name, int size )
{
    struct ttf_pool_el req;

    req.font_name = file_name;
    req.font_size = size;

    int rc;

    // Can't reuse FT_Face
#if CACHE_FT_FACE
    rc = pool_foreach( tt_font_pool, tt_lookup, &req );

    if( rc == 1 )
        return req.fhandle; // found
#endif
    // Attempt to load

    FT_Face ftFace = 0;

    rc = w_load_tt_from_file( &ftFace, file_name );
    if( rc  )
        return INVALID_POOL_HANDLE;

    FT_Set_Pixel_Sizes(ftFace, size, 0);

    req.face = ftFace;

    return w_store_tt_to_pool( &req );
}



static pool_handle_t w_store_tt_to_pool( struct ttf_pool_el *req )
{

    struct ttf_pool_el *newel = calloc( sizeof(struct ttf_pool_el), 1 );
    if( 0 == newel )
    {
        lprintf("\nout of mem loading font %s\n", req->font_name );
        return INVALID_POOL_HANDLE;
    }

    *newel = *req;
    newel->font_name = strdup(req->font_name);
    if( 0 == newel->font_name )
    {
        lprintf("\nout of mem strdup loading font %s\n", req->font_name );
        free(newel);
        return INVALID_POOL_HANDLE;
    }

    pool_handle_t newh = pool_create_el( tt_font_pool, newel );

    if( newh == INVALID_POOL_HANDLE )
    {
        lprintf("\npool_create_el failed loading font %s\n", req->font_name );
        free( (void *)newel->font_name );
        free( newel );
    }

    return newh;
}








static int w_load_tt_from_file( FT_Face *ftFace, const char *file_name )
{
   // tmp run in FS? user mode?

    char buf[1024];
    snprintf( buf, sizeof(buf)-1, "P:/phantomuserland/plib/resources/ttfonts/opensans/%s", file_name );

    hal_mutex_lock( &faces_mutex );
    int rc = FT_New_Face(ftLibrary, buf, 0, ftFace);
    hal_mutex_unlock( &faces_mutex );

    if( rc )
        lprintf("\ncan't load font %s, rc = %d\n", buf, rc );

    return rc;
}


static int w_load_tt_from_mem( FT_Face *ftFace, void *mem_font, size_t mem_font_size, const char* diag_font_name )
{
    FT_Open_Args args;

    memset( &args, 0, sizeof(args) );

    args.flags = FT_OPEN_MEMORY;
    args.memory_base = mem_font;
    args.memory_size = mem_font_size;

    *ftFace = 0;

    hal_mutex_lock( &faces_mutex );
    int rc = FT_Open_Face( ftLibrary, &args, 0, ftFace );
    hal_mutex_unlock( &faces_mutex );

    if(rc)
        lprintf("\ncan't load font %s, rc = %d\n", diag_font_name, rc );

    return rc;
}


pool_handle_t w_get_tt_font_mem( void *mem_font, size_t mem_font_size, const char* diag_font_name, int font_size )
{
    FT_Face ftFace;

    int rc = w_load_tt_from_mem( &ftFace, mem_font, mem_font_size, diag_font_name );
    if( rc )
        return INVALID_POOL_HANDLE;

    struct ttf_pool_el req;

    req.font_name = diag_font_name;
    req.font_size = font_size;
    req.face = ftFace;

    return w_store_tt_to_pool( &req );
}


extern char OpenSans_Regular_ttf_font[];
extern unsigned int OpenSans_Regular_ttf_font_size;

//pool_handle_t OpenSans_Regular_ttf_font_handle;

/*
static void w_preload_compiled_fonts(void)
{
    OpenSans_Regular_ttf_font_handle = w_get_tt_font_mem( OpenSans_Regular_ttf_font, OpenSans_Regular_ttf_font_size, "OpenSans Regular", 14 );
}
*/

pool_handle_t w_get_system_font_ext( int font_size )
{
    //SHOW_FLOW( 1, "w_get_system_font_ext %d", font_size );
    printf( "w_get_system_font_ext %d\n", font_size );
    return w_get_tt_font_mem( OpenSans_Regular_ttf_font, OpenSans_Regular_ttf_font_size, "OpenSans Regular", font_size );
}


pool_handle_t w_get_system_font( void )
{
    return w_get_system_font_ext( 14 );
}





#endif // CONF_TRUETYPE
