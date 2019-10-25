//#include <stdio.h>


//void winhal_setport( int sock, int port );

void winhal_debug_srv_thread(int *arg);

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




void win_hal_init( void );

void win_hal_sleep_msec( int miliseconds );

void win_hal_disable_preemption(void);
void win_hal_enable_preemption(void);

unsigned long win_hal_start_thread( void (*thread)(void *arg), void *arg );


void * gen_hal_mutex_init(const char *name);
int gen_hal_mutex_lock(void *_m);
int gen_hal_mutex_unlock(void *_m);
int gen_hal_mutex_is_locked(void *_m);

void *gen_hal_cond_init(const char * name);
void gen_hal_cond_wait( void *c_impl, void *m_impl );
void gen_hal_cond_twait( void *c_impl, void *m_impl, long msecTimeout );
void gen_hal_cond_signal( void *c_impl );
void gen_hal_cond_broadcast( void *c_impl );
void gen_hal_cond_destroy( void *c_impl );


void win_hal_open_kernel_log_file( const char *fn );
void win_hal_open_kernel_out_file( const char *fn );

