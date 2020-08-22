#include "amp.h"
#include <sys/types.h>
//#include <sys/status.h>

//typedef uint32 uid_t;

#include <sys/unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <string.h>

#define AUDIO
#include "audio.h"
#include "formats.h"
#include "getbits.h"
#include "huffman.h"
#include "layer2.h"
#include "layer3.h"
#include "position.h"
#include "transform.h"



void statusDisplay( struct AUDIO_HEADER *header, int frameNo ) {
    int minutes,seconds;

    if ( ( A_SHOW_CNT || A_SHOW_TIME ) && !( frameNo % 10 ) ) {
	msg("\r");
    }
    if ( A_SHOW_CNT && !( frameNo % 10 ) ) {
	msg( "{ %d } ", frameNo );
    }
    if ( A_SHOW_TIME && !( frameNo % 10 ) ) {
	seconds = frameNo * 1152 /
	    t_sampling_frequency[ header -> ID ][ header -> sampling_frequency ];
	minutes = seconds / 60;
	seconds = seconds % 60;
	msg( "[%d:%02d]", minutes, seconds );
    }
    if ( A_SHOW_CNT || A_SHOW_TIME ) {
	//FileFlush( stderr );
    }
}

int decodeMPEG( void ) {
    struct AUDIO_HEADER header;
    int cnt, g, snd_eof;

    cnt = 0;

    initialise_globals();

    if ( A_FORMAT_WAVE ) {
	wav_begin();
    }
	
    if ( ( g = gethdr( &header ) ) != 0 ) {
	report_header_error( g );
	return -1;
    }

    if ( header.protection_bit == 0 ) {
	getcrc();
    }

    if ( setup_audio( &header ) != 0 ) {
	warn( "Cannot set up audio. Exiting\n" );
	return -1;
    }

    show_header( &header );

    if ( header.layer == 1 ) {
	if ( layer3_frame( &header,cnt ) ) {
		warn( " error. blip.\n" );
		return -1;
	}
    } else if ( header.layer == 2 ) {
	if ( layer2_frame( &header,cnt ) ) {
	    warn( " error. blip.\n" );
	    return -1;
	}
    }

    /* decoder loop */
    snd_eof = 0;
    cnt = 0;
    while ( !snd_eof ) {
	while ( !snd_eof && ready_audio() ) {
	    if ( ( g = gethdr( &header ) ) != 0 ) {
		report_header_error( g );
		if ( g == GETHDR_EOF && A_FORMAT_WAVE ) {
		    wav_end( &header );
		}
		snd_eof = 1;
		break;
            }

    	    if ( header.protection_bit == 0 ) {
		getcrc();
	    }

	    statusDisplay( &header,cnt );

	    if ( header.layer == 1 ) {
		if ( layer3_frame( &header, cnt ) ) {
		    warn( " error. blip.\n" );
		    return -1;
		}
	    } else if ( header.layer == 2 ) {
		if ( layer2_frame( &header, cnt ) ) {
		    warn(" error. blip.\n");
		    return -1;
		}
	    }
	    cnt++;
	}
    }
    return 0;
}

int main( int argc, char **argv ) {
    int argPos;

    /* process command line arguments */
    argPos = args( argc , argv );
    //Print( "argPos: %d\n", argPos );

    if ( argc == 1 ) {
	goto end;
    }

    /* initialise decoder */
    initialise_decoder();


    /* play each specified file */
    if ( A_AUDIO_PLAY ) {
	if ( argPos < argc ) {
	    for ( ; argPos < argc; argPos++ ) {
		play( argv[ argPos ], 0 );
	    }
	}  else {
	    displayUsage();
	}
    } else {
	/* convert the file to some format */
	if ( ( argPos + 2 ) == argc ) {
	    play( argv[ argPos ], argv[ argPos + 1 ] );
	} else {
	    warn( "Invalid number of parameters\n" );
	    displayUsage();
	    die( "" );
	}
    }

end:
    msg( "\nThank you for using amp!\n" );

    return 0;
}

/* call this once at the beginning */
void initialise_decoder( void ) {
    premultiply();
    imdct_init();
    calculate_t43();
}

/* call this before each file is played */
void initialise_globals( void ) {
    append = data = nch = 0; 
    f_bdirty = TRUE;
    bclean_bytes = 0;

    MemorySet( s, 0, sizeof s );
    MemorySet( res, 0, sizeof res );
}

void report_header_error( int err ) {
    switch ( err ) {
	case GETHDR_ERR:
	    die("error reading mpeg bitstream. exiting.\n");
	    break;
	case GETHDR_NS:
	    warn("this is a file in MPEG 2.5 format, which is not defined\n");
	     warn("by ISO/MPEG. It is \"a special Fraunhofer format\".\n");
	     warn("amp does not support this format. sorry.\n");
	    break;
	case GETHDR_FL1:
	    warn("ISO/MPEG layer 1 is not supported by amp (yet).\n");
	    break;
	case GETHDR_FF:
	    warn("free format bitstreams are not supported. sorry.\n");
	    break;	
	case GETHDR_SYN:
	    warn("oops, we're out of sync.\n");
	    break;
	case GETHDR_EOF: 
	default:
	    break;
    }
}

/* TODO: there must be a check here to see if the audio device has been opened
 * successfuly. This is a bitch because it requires all 6 or 7 OS-specific functions
 * to be changed. Is anyone willing to do this at all???
 */
int setup_audio( struct AUDIO_HEADER *header ) {
    if ( A_AUDIO_PLAY ) {
	audioOpen(
	    t_sampling_frequency[ header -> ID ][ header -> sampling_frequency ],
	    ( header -> mode != 3 && !A_DOWNMIX ), A_SET_VOLUME );
    }
    return 0;
}

void close_audio( void ) {
    if ( A_AUDIO_PLAY ) {
	audioClose();
    }
}

int ready_audio( void ) {
    return 1;
}

bool hasTag(void) {
    char tag[ 4 ];

    if ( lseek( in_fd, -128, SEEK_END ) < 0 ) {
	return FALSE;
    }

    if ( FileRead( tag, 1, 3, in_fd ) != 3 ) {
	return FALSE;
    }
    return StringNumCompare( tag, "TAG", 3 ) == 0 ? TRUE : FALSE;
}

struct sTag {
    char Title[ 31 ];
    char Artist[ 31 ];
    char Album[ 31 ];
    char Year[ 5 ];
    char Comment[ 31 ];
    char Genre;
};

void showTag() {
    struct sTag *sTag;

    if ( hasTag() == FALSE ) {
	Print( "NO TAG PRESENT\n" );
	FileSeek( in_fd, 0, SEEK_SET );
	return;
    }

    sTag = MemoryAllocation( sizeof( struct sTag ) );

    FileRead( sTag -> Title, 1, 30, in_fd );
    FileRead( sTag -> Artist, 1, 30, in_fd );
    FileRead( sTag -> Album, 1, 30, in_fd );
    FileRead( sTag -> Year, 1, 4, in_fd );
    FileRead( sTag -> Comment, 1, 30, in_fd );
    FileRead( &sTag -> Genre, 1, 1, in_fd );
    sTag -> Title[ 30 ] = '\0';
    sTag -> Artist[ 30 ] = '\0';
    sTag -> Album[ 30 ] = '\0';
    sTag -> Year[ 4 ] = '\0';
    sTag -> Comment[ 30 ] = '\0';

    Print( "title: %s\nartist: %s\nalbum: %s\n"
	"year: %s\ncomment: %s\ngenre: %d\n",
	sTag -> Title, sTag -> Artist, sTag -> Album, sTag -> Year, sTag -> Comment, sTag -> Genre );
    FileSeek( in_fd, 0, SEEK_SET );
    FreeMemory( sTag );
}

/* TODO: add some kind of error reporting here */
void play( char *inFileStr, char *outFileStr ) {
    if ( StringCompare( inFileStr, "-" ) == 0 ) {
	in_fd = 0;
    } else {
	if ( ( in_fd = open( inFileStr, O_RDONLY ) ) == NULL ) {
	    warn( "Could not open file: %s\n", inFileStr );
	    return;
	}
    }
    if ( outFileStr ) {
	if ( StringCompare( outFileStr, "-" ) == 0 ) {
	    out_fd = 1;
	} else if ( ( out_fd = open( outFileStr, O_WRONLY ) ) == NULL ) {
	    warn( "Could not write to file: %s\n", outFileStr );
	    return;
	}
	msg( "Converting: %s\n", inFileStr );
    }

    if ( A_AUDIO_PLAY ) {
	msg( "Playing: %s\n", inFileStr );
    }

    showTag();

    decodeMPEG();
	
    close_audio();
    close( in_fd );
    if ( !A_AUDIO_PLAY ) {
	close( out_fd );
    }
    msg( "\n" );
}
