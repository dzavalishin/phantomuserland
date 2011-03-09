#include "amp.h"
#include "transform.h"
#include "audio.h"
#include <sys/types.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <stdio.h>

void printout( void ) {
    int j;

    if ( nch == 2 ) {
        j = 32 * 18 * 2;
    } else {
        j = 32 * 18;
    }

    audioWrite( ( char * ) sample_buffer, j * sizeof( short ) );
}

//#include <system/drivers/sound/soundcard.h>

static int audio_fd;
static int mixer_fd;
//static int volumeIoctl;

/* audioOpen() */
/* should open the audio device, perform any special initialization		 */
/* Set the frequency, no of channels and volume. Volume is only set if */
/* it is not -1 */
void audioOpen( int frequency, int stereo, int volume ) {
    //int supportedMixers;
    //    int play_format = AFMT_S16_LE;
    char anName[ 256 ] = "es1370";
    char *pnTmp;

    /*
     if ( GetStandardOutput( anName ) ) {
     die( "Unable to get standard audio output\n" );
     }
     */

    Print( "Default Output: [%s]\n", anName );

    int ll = StringLength( anName ) + 40;

    pnTmp = MemoryAllocation( ll );
    if ( pnTmp == NULL ) {
        die( "Unable to allocate memory\n" );
    }

    StringPrint( pnTmp, ll, "/dev/pci/%s", anName );

    if ( ( audio_fd = Open( pnTmp, O_WRONLY ) ) < 0 ) {
        die( "Unable to open the audio device\n" );
    }
    DB( audio, msg( "Audio device opened on %d\n", audio_fd ); )

    int play_format;

    play_format = 1;
    if ( ioctl( audio_fd, IOCTL_SOUND_SIGNED, &play_format, sizeof(int) ) < 0 ) {
        die( "Unable to set required audio format\n" );
    }

    StringPrint( pnTmp, ll, "/dev/sound/%s/mixer", anName );
    if ( ( mixer_fd = open( pnTmp, O_RDWR ) ) < 0 ) {
        warn( "Unable to open mixer device\n" );
    }
    DB( audio, msg( "Mixer device opened on %d\n", mixer_fd ) );

#if 0
    if ( IoControl( mixer_fd, SOUND_MIXER_READ_DEVMASK,
                    &supportedMixers, sizeof(int) ) == -1 ) {
        warn( "Unable to get mixer info assuming master volume\n" );
        volumeIoctl = SOUND_MIXER_WRITE_VOLUME;
    } else {
        if ( ( supportedMixers & SOUND_MASK_PCM ) != 0 ) {
            volumeIoctl = SOUND_MIXER_WRITE_PCM;
        } else {
            volumeIoctl = 0;
        }
    }
#endif

    /* Set 1 or 2 channels */
    stereo = ( stereo ? 2 : 1 );
    DB( audio, msg( "Setting stereo to %d\n", stereo ) )
        if ( ioctl( audio_fd, IOCTL_SOUND_NCHANNELS, &stereo, sizeof(int) ) < 0 ) {
            die( "Unable to set stereo/mono\n" );
        }

    /* Set the output frequency */
    DB( audio, msg( "Setting freq to %d Hz\n", frequency ) )
        if ( ioctl( audio_fd, IOCTL_SOUND_SAMPLERATE, &frequency, sizeof(int) ) < 0 ) {
            die( "Unable to set frequency: %d\n", frequency );
        }

    if ( volume != -1 ) {
        audioSetVolume(volume);
    }

    //if ( IoControl( audio_fd, SNDCTL_DSP_GETBLKSIZE, &AUSIZ ) == -1 ) {
    //	die( "Unable to get fragment size\n" );
    //}
}

/* audioSetVolume - only code this if your system can change the volume while
 * playing. sets the output volume 0-100
 */
void audioSetVolume( int volume ) {
    DB( audio, msg( "Setting volume to: %d\n", volume ); )

    volume = ( volume << 8 ) + volume;
    if ( ioctl( audio_fd, IOCTL_SOUND_VOLUME, &volume, sizeof(int) ) < 0 )
    {
        warn( "Unable to set sound volume\n" );
    }
}

/* audioFlush() */
/* should flush the audio device */
inline void audioFlush() {
    DB( audio, msg( "audio: flush %d\n", audio_fd ) );

    if ( IoControl( audio_fd, IOCTL_SOUND_RESET, 0 ) == -1) {
        die( "Unable to reset audio device\n" );
    }
}

/* audioClose() */
/* should close the audio device and perform any special shutdown */
void audioClose() {
    Close( audio_fd );
    if ( mixer_fd != -1 ) {
        Close( mixer_fd );
    }
    DB( audio, msg( "audio: closed %d\n", audio_fd ) );
}

/* audioWrite */
/* writes count bytes from buffer to the audio device */
/* returns the number of bytes actually written */
inline int audioWrite( char *buffer, int count ) {
    DB( audio, msg( "audio: Writing %d bytes to audio descriptor %d\n",
                    count, audio_fd ) );
    return Write( audio_fd, buffer,count );
}
