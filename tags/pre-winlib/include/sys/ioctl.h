#ifndef IOCTL_H
#define IOCTL_H
#include <sys/ioccom.h>



// Video 

#define IOCTL_G_VIDEO 'V'
#define _IOV_VIDEO(__n) _IOC(IOC_VOID,IOCTL_G_VIDEO,__n,0)
#define _IOI_VIDEO(__n) _IOWINT(IOCTL_G_VIDEO,__n)
// rect_t
#define _IOR_VIDEO(__n) _IOW(IOCTL_G_VIDEO,__n,sizeof(rect_t))


#define IOCTL_FB_FLUSH               _IOV_VIDEO(1)

#define IOCTL_FB_SETBOUNDS           _IOR_VIDEO(2)
#define IOCTL_FB_SETCOLOR            _IOI_VIDEO(3)

#define IOCTL_FB_DRAWBOX            _IOR_VIDEO(16)
#define IOCTL_FB_FILLBOX            _IOR_VIDEO(17)
#define IOCTL_FB_DRAWLINE           _IOR_VIDEO(18)
#define IOCTL_FB_DRAWPIXEL          _IOW(IOCTL_G_VIDEO,19,sizeof(point_t))

//#undef _I
//#undef _R
//#undef _V


// Sound

#define IOCTL_G_SOUND 'S'
#define _IOI_SOUND(__n) _IOWINT(IOCTL_G_SOUND,__n)

#define IOCTL_SOUND_RESET            _IOC(IOC_VOID,IOCTL_G_SOUND,0,0)

#define IOCTL_SOUND_SAMPLERATE       _IOI_SOUND(1)
#define IOCTL_SOUND_NCHANNELS        _IOI_SOUND(2)
#define IOCTL_SOUND_BITS             _IOI_SOUND(3)
#define IOCTL_SOUND_SIGNED           _IOI_SOUND(4)
#define IOCTL_SOUND_VOLUME           _IOI_SOUND(5)

//#undef _I

// TABOS compat
//#define SNDCTL_DSP_STEREO	    16
//#define SNDCTL_DSP_SPEED            17 // Freq
//#define SNDCTL_DSP_RESET            18
//#define SOUND_MIXER_WRITE_VOLUME    19
//#define SNDCTL_DSP_SETFMT           20

// Sample formats
//#define AFMT_S16_LE                  1


#endif // IOCTL_H

