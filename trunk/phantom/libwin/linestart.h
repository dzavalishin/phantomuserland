

// Find pointer to the line on screen. As addresses go from left to right in
// both screen and sys coords, this is the only strange calculation here.

// Zeroth line will have index of (scr y size - 1), right?

//#define DRV_VIDEO_REVERSE_LINESTART(ypos) ( (video_drv->xsize * ((video_drv->ysize -1) - ypos) ) * bit_mover_byte_step + video_drv->screen)
//#define DRV_VIDEO_FORWARD_LINESTART(ypos) ( (video_drv->xsize * ypos) * bit_mover_byte_step + video_drv->screen)


#define DRV_VIDEO_REVERSE_LINESTART(ypos) \
    ( bit_mover_byte_step ? \
      ( (video_drv->xsize * ((video_drv->ysize-1) - ypos) ) * bit_mover_byte_step + video_drv->screen ) \
      : \
      ( (video_drv->xsize * ((video_drv->ysize-1) - ypos) ) / 4 + video_drv->screen ) \
    )
#define DRV_VIDEO_FORWARD_LINESTART(ypos) ( (video_drv->xsize * ypos) * bit_mover_byte_step + video_drv->screen)

/*
extern void (*bit_zbmover_to_screen)( void *dest, const struct rgba_t *src, zbuf_t *zb, int nelem, zbuf_t zpos );
extern void (*bit_mover_to_screen)( void *dest, const struct rgba_t *src, int nelem );
extern void (*bit_mover_to_screen_noalpha)( void *dest, const struct rgba_t *src, int nelem );


extern void (*bit_mover_from_screen)( struct rgba_t *dest, void *src, int nelem );
extern int      bit_mover_byte_step;
*/

