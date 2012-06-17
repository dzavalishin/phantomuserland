/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2012 Dmitry Zavalishin, dz@dz.ru
 *
 * Video subsystem config.
 *
**/

#ifndef VCONFIG_H
#define VCONFIG_H

// Windows 'screen' driver works in BGR format :(
#define BGR 1


//! Max number of unprocessed events in window queue.
//! Extra events will be thrown away.
#define MAX_WINDOW_EVENTS 512



//! If 1, VESA will be used if found, even if other driver is found
//! If 0, VESA will fight for itself as usual driver
//! Now using 1, kernel traps if trying to do VM86 too late in boot process
#define VESA_ENFORCE 1


#define VIDEO_PARTIAL_WIN_BLIT 1

//! If window is not covered, we can ignore z buffer when painting
#define VIDEO_NOZBUF_BLIT 0


#define VIDEO_NEW_PAINTER 1


#endif // VCONFIG_H
