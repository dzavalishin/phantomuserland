#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
 
#include <stdint.h>

#include <video/screen.h>
#include <video/font.h>


FT_Glyph getGlyph(FT_Face face, uint32_t charcode);
FT_Pos getKerning(FT_Face face, uint32_t leftCharcode, uint32_t rightCharcode);
//void savePNG(uint8_t *image, int32_t width, int32_t height);
 
struct Symbol
{
    int32_t posX;
    int32_t posY;
    int32_t width;
    int32_t height;
    FT_Glyph glyph;
};
 
const size_t MAX_SYMBOLS_COUNT = 128;
 
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))
 
 
void w_ttfont_draw_string(
                          window_handle_t win,
                          //const drv_video_font_t *font,
                          const char *s, const rgba_t color, const rgba_t bg,
                          int x, int y )
{
    FT_Library ftLibrary = 0;
    FT_Init_FreeType(&ftLibrary);
 
    FT_Face ftFace = 0;
    FT_New_Face(ftLibrary, "arial.ttf", 0, &ftFace);
 
    FT_Set_Pixel_Sizes(ftFace, 100, 0);
 
    const char *exampleString = "FreeType it's amazing!";
    const size_t exampleStringLen = strlen(exampleString);
 
    struct Symbol symbols[MAX_SYMBOLS_COUNT];
    size_t numSymbols = 0;
 
    int32_t left = INT_MAX;
    int32_t top = INT_MAX;
    int32_t bottom = INT_MIN;
    uint32_t prevCharcode = 0;
 
    size_t i = 0;
 
    int32_t posX = 0;
 
    for (i = 0; i < exampleStringLen; ++i)
    {
        const uint32_t charcode = exampleString[i];
 
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
 
        struct Symbol *symb = &(symbols[numSymbols++]);
 
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
 
    for (i = 0; i < numSymbols; ++i)
    {
        symbols[i].posX -= left;
    }
 
    const struct Symbol *lastSymbol = &(symbols[numSymbols - 1]);
 
    const int32_t imageW = lastSymbol->posX + lastSymbol->width;
    const int32_t imageH = bottom - top;
 
    uint8_t *image = malloc(imageW * imageH);
 
    for (i = 0; i < numSymbols; ++i)
    {
        const struct Symbol *symb = symbols + i;
 
        FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph) symb->glyph;
        FT_Bitmap bitmap = bitmapGlyph->bitmap;

        int32_t srcY;
        for(srcY = 0; srcY < symb->height; ++srcY)
        {
            const int32_t dstY = symb->posY + srcY - top;

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
                uint8_t *dst = image + dstX + dstY * imageW;
                dst[0] = (uint8_t)(a * 255 + (1 - a) * dst[0]);
            }
        }
    }
 
    //savePNG(image, imageW, imageH);
 
    free(image);
 
    for (i = 0; i < numSymbols; ++i)
    {
        FT_Done_Glyph(symbols[i].glyph);
    }
 
    FT_Done_Face(ftFace);
    ftFace = 0;
 
    FT_Done_FreeType(ftLibrary);
    ftLibrary = 0;
 
    return 0;
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
 
#if 0
void savePNG(uint8_t *image, int32_t width, int32_t height)
{
    FILE *f = fopen("output.png", "wb");
 
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
 
    png_infop info_ptr = png_create_info_struct(png_ptr);
 
    png_init_io(png_ptr, f);
 
    png_set_IHDR(
        png_ptr, 
        info_ptr, 
        width, 
        height, 
        8, 
        PNG_COLOR_TYPE_RGBA, 
        PNG_INTERLACE_NONE, 
        PNG_COMPRESSION_TYPE_BASE, 
        PNG_FILTER_TYPE_BASE);
 
    png_write_info(png_ptr, info_ptr);
 
    uint8_t *row = malloc(width * 4);
 
    for (int32_t y = 0; y < height; ++y)
    {
        for (int32_t x = 0; x < width; ++x)
        {
            row[x * 4 + 0] = 0xc0;
            row[x * 4 + 1] = 0xc0;
            row[x * 4 + 2] = 0xc0;
            row[x * 4 + 3] = image[y * width + x];
        }
 
        png_write_row(png_ptr, row);
    }
 
    free(row);
 
    png_write_end(png_ptr, 0);    
 
    png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&png_ptr, 0);
 
    fclose(f);
}
#endif


