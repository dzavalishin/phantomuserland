#include "amp.h"
#include "audio.h"
#include "getopt.h"
#include <stdio.h>
#include <string.h>

void displayDisclaimer( void ) {
    /* print a warm, friendly message */
    msg( "\namp %d.%d.%d, (C) Tomislav Uzelac 1996,1997\n",
		MAJOR, MINOR, PATCH );
    msg( "THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY\n" );
    msg( "PLEASE READ THE DOCUMENTATION FOR DETAILS\n\n" );
}

void displayUsage( void ) {
    Print("\
usage: amp [options] [ MPEG audio streams... ]\n\
       amp -convert [ MPEG-audio stream ] [ output file ]\n\n\
  -h, -help           display this usage information and exit\n\
  -v, -version        display the version information and exit\n\
  -c, -convert        convert the MPEG audio stream to output file format\n\
  -p, -play           play the specified MPEG audio streams (default action)\n\
  -q, -quiet          supress printing of messages to STDERR\n\
  -d, -dump           dump binary data to STDERR\n\
  -s, -frame          show a frame counter\n\
  -t, -time           show time\n\
  -w                  wav output\n\
      -downmix        downmix stereo streams to one channel\n\
      -volume <vol>   set the volume to <vol> (0-100)\n" );

    ThreadExit( 0 );
}

void displayVersion( void ) {
    Print( "amp - %d.%d.%d\n", MAJOR, MINOR, PATCH );
    ThreadExit( 0 );
}

int argVal( char *name, char *num, int min, int max ) {
    int val;
	
    val = StringToInteger( num );
    if ( ( val < min ) || ( val > max ) ) {
	die( "%s parameter %d - out of range (%d-%d)\n", name, val, min, max );
    }
    return val;
}

int args( int argc, char **argv ) {
	int c;
	static int showusage = 0, showversion = 0;

	A_DUMP_BINARY = FALSE;
	A_QUIET = FALSE;
	A_FORMAT_WAVE = FALSE;
	A_SHOW_CNT = FALSE;
	A_SET_VOLUME = -1;
	A_SHOW_TIME = 0;
	A_AUDIO_PLAY = TRUE;
	A_WRITE_TO_FILE = FALSE;
	A_MSG_STDOUT = FALSE;
	A_DOWNMIX = FALSE;

	while ( 1 ) {
		static struct option long_options[] =	{
			{"help", no_argument, 0, 'h'},
			{"debug", required_argument, 0, 1},
			{"version", no_argument, 0, 'v'},
			{"quiet", no_argument, 0, 'q'},
			{"play", no_argument, 0, 'p'},
			{"convert", no_argument, 0, 'c'},
			{"volume", required_argument, 0, 2},
			{"time", no_argument, 0, 't'},
			{"frame", no_argument, 0, 's'},
			{"dump", required_argument, 0, 'd'},
			{"downmix",no_argument,0,'x'},
			{0, 0, 0, 0}
		};

		c = getopt_long_only( argc, argv, "xqstwdpcvh", long_options, 0 );

		if ( c == -1 ) {
			break;
		}
		switch ( c ) {
			case 1:
				debugSetup( optarg );
				break;
			case 2:
				A_SET_VOLUME = argVal( "Volume", optarg, 0, 100 );
				break;
	    case 'q':
				A_QUIET = TRUE;
				break;
	    case 's':
				A_SHOW_CNT = TRUE;
				break;
	    case 't':
		A_SHOW_TIME = TRUE;
		break;
	    case 'w':
		A_FORMAT_WAVE = TRUE;
		break;
	    case 'd':
		A_DUMP_BINARY = TRUE;
		break;
	    case 'p':
		A_AUDIO_PLAY = TRUE;
		A_WRITE_TO_FILE = FALSE;
		break;
	    case 'c':
		A_AUDIO_PLAY = FALSE;
		A_WRITE_TO_FILE = TRUE;
		break;
	    case 'v':
		showversion = 1;
		break;
	    case 'h':
		showusage = 1;
		break;
	    case 'x':
		Print( "Activate DOWNMIX\n" );
		A_DOWNMIX = TRUE;
		break;
	    case '?':
		ThreadExit( 1 );
	    case ':':
		Print( "Missing parameter for option -%c\n", c );
		break;
		}
	}
	if ( showversion ) {
		displayVersion();
	}
	if ( showusage ) {
		displayUsage();
	}
	displayDisclaimer();

	return optind;
}
