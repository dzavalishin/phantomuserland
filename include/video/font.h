#ifndef FONT_H
#define FONT_H

typedef struct drv_video_font_t
{
    int         	xsize;
    int 		ysize;
    char *              font;
} drv_video_font_t;

extern struct drv_video_font_t         drv_video_16x16_font;
extern struct drv_video_font_t         drv_video_8x16ant_font;
extern struct drv_video_font_t         drv_video_8x16bro_font;
extern struct drv_video_font_t         drv_video_8x16cou_font;
extern struct drv_video_font_t         drv_video_8x16med_font;
extern struct drv_video_font_t         drv_video_8x16rom_font;
extern struct drv_video_font_t         drv_video_8x16san_font;
extern struct drv_video_font_t         drv_video_8x16scr_font;

extern struct drv_video_font_t         drv_video_kolibri1_font;
extern struct drv_video_font_t         drv_video_kolibri2_font;

#endif // FONT_H
