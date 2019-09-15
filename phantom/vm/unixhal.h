/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * HAL bindings for unix, common header for unix_hal.c and unix_hal_unix.c
 *
**/



void winhal_debug_srv_thread(int *arg);
//void unixhal_debug_srv_thread(int *arg);

void drv_win_screen_update(void);

long fast_time(void);



// windows interface

#define VSCREEN_WIDTH 1024
#define VSCREEN_HEIGHT 768

int win_scr_setup_window(void);
void win_scr_init_window(void);
void win_scr_event_loop(void);
void win_scr_mk_mouse_event(int wParam, int Xpos, int Ypos );
void win_scr_screen_update(void);
int win_src_make_thread( void *tfunc );

extern char * win_src_screen_image;




void unix_hal_init( void );

void unix_hal_sleep_msec( int miliseconds );

void unix_hal_disable_preemption(void);
void unix_hal_enable_preemption(void);

unsigned long unix_hal_start_thread( void (*thread)(void *arg), void *arg );


void * unix_hal_mutex_init(const char *name);
int unix_hal_mutex_lock(void *_m);
int unix_hal_mutex_unlock(void *_m);
int unix_hal_mutex_is_locked(void *_m);


void unix_hal_open_kernel_log_file( const char *fn );
void unix_hal_open_kernel_out_file( const char *fn );

