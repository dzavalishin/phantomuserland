
drv_video_window_t *private_drv_video_window_create(int xsize, int ysize);
void defaultWindowEventProcessor( drv_video_window_t *w, struct ui_event *e );
int drv_video_window_get_event( drv_video_window_t *w, struct ui_event *e, int wait );
void win_make_decorations(drv_video_window_t *w);

