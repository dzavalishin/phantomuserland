int     win_x11_init(int _xsize, int _ysize);

void    win_x11_stop(void);

//void * win_x11_update(const char *src);
void    win_x11_update(void);

//void * x11_prepare(Display *display, Window win, GC gc);

void    win_x11_set_screen( void *vmem );

void    win_x11_message_loop( void );

void    win_x11_key_event( int x, int y, unsigned int state, unsigned int keycode, int isRelease );
void    win_x11_mouse_event( int x, int y, unsigned int state );


