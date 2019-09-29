#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
 
#include <stdint.h>

#include <utf8proc.h>

#include <video/screen.h>
#include <video/font.h>


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
                          //const drv_video_font_t *font,
                          const char *str, const rgba_t color, const rgba_t bg,
                          int win_x, int win_y )
{
    int rc;

    FT_Library ftLibrary = 0;
    FT_Init_FreeType(&ftLibrary);
 
    FT_Face ftFace = 0;
    rc = FT_New_Face(ftLibrary, "P:/phantomuserland/plib/resources/ttfonts/opensans/OpenSans-Regular.ttf", 0, &ftFace);
    if( rc )
    {
        printf("\ncan't load font\n");
        return;
    }
 
    FT_Set_Pixel_Sizes(ftFace, 200, 0);
    //FT_Set_Pixel_Sizes(ftFace, 100, 0);
    //FT_Set_Pixel_Sizes(ftFace, 50, 0);
 
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
        {
            continue;
        }
 
        if (prevCharcode)
        {
            posX += getKerning(ftFace, prevCharcode, charcode);
        }
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
            if( wy > win->ysize ) continue;

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
                //uint8_t *dst = image + dstX + dstY * imageW;
                //dst[0] = (uint8_t)(a * 255 + (1 - a) * dst[0]);

                // TODO opacity
                int wx = win_x + dstX;
                // wrong, makes bottom to be a baseline
                //w_draw_pixel( win, wx, wy, color );

                if( wx < 0 ) continue; 
                if( wx > win->xsize ) break;

                rgba_t old = win->w_pixel[wx+_wy];

                win->w_pixel[wx+_wy].r = W_BLEND_PIXEL( old.r, color.r, a );
                win->w_pixel[wx+_wy].g = W_BLEND_PIXEL( old.g, color.g, a );
                win->w_pixel[wx+_wy].b = W_BLEND_PIXEL( old.b, color.b, a );

                win->w_pixel[wx+_wy].a = 0xFF;

                //win->w_pixel[wx+_wy] = color;
            }
        }
    }
 
 
    for (i = 0; i < numSymbols; ++i)
    {
        FT_Done_Glyph(symbols[i].glyph);
    }
 
    FT_Done_Face(ftFace);
    ftFace = 0;
 
    FT_Done_FreeType(ftLibrary);
    ftLibrary = 0;
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
 

