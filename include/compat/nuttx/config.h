#ifndef _CONFIG_NUTTX_H
#define _CONFIG_NUTTX_H

#include <video/rect.h>
#include <stdint.h>

// ---------------------------
// VNC config
// ---------------------------


#define CONFIG_VNCSERVER_COLORFMT_RGB32
//#define CONFIG_VNCSERVER_PROTO3p8
#define CONFIG_VNCSERVER_PROTO3p3 // start with simplest one, enable p8 later


// ---------------------------
// Compat
// ---------------------------

#define pthread_t tid_t

#define nxgl_rect_s rect_t

#define sem_t hal_sem_t


#define FAR
#define CODE


#define OK 0
#define EXIT_FAILURE ENXIO // why?

#ifndef NULL
#define NULL 0
#endif


#define kmm_zalloc(__memsz) calloc(1,__memsz)
#define kmm_free free


#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif



#define psock_close tcp_close
#define psock_socket( __inet, __stream, __unkn, __sptr ) tcp_open( __sptr )



typedef int16_t nxgl_coord_t;



struct nxgl_point_s
{
  nxgl_coord_t x;         /* X position, range: 0 to screen width - 1 */
  nxgl_coord_t y;         /* Y position, range: 0 to screen height - 1 */
};



struct nxgl_rect_s
{
  struct nxgl_point_s pt1; /* Upper, left-hand corner */
  struct nxgl_point_s pt2; /* Lower, right-hand corner */
};



#endif // _CONFIG_NUTTX_H

