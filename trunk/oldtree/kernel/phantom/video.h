#include <drv_video_screen.h>

extern struct drv_video_screen_t        video_driver_bochs_vesa_emulator;
extern struct drv_video_screen_t        video_driver_basic_vga;
extern struct drv_video_screen_t        video_driver_direct_vesa;
extern struct drv_video_screen_t        video_driver_bios_vesa;
extern struct drv_video_screen_t        video_driver_cirrus;

// TODO call from text vga console drvr?
// Can be called from any driver (or anywhere) to reset VGA to text mode
extern void video_drv_basic_vga_set_text_mode();

// Redirect console output to this window. TODO: input?
extern void phantom_init_console_window();
void phantom_stop_console_window();


void phantom_enforce_video_driver(struct drv_video_screen_t *vd);
void set_video_driver_bios_vesa_pa( physaddr_t pa, size_t size );
void set_video_driver_bios_vesa_mode( u_int16_t mode );

// If 1, VESA will be used if found, even if other driver is found
// If 0, VESA will fight for itself as usual driver
// Now using 1, kernel traps if trying to do VM86 too late in boot process
#define VESA_ENFORCE 1

void setTextVideoMode(); // Using int10
int setVesaMode( u_int16_t mode ); // Using int10


