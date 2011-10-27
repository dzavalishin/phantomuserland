/*
 * Copyright (c) 1998-2001 Yoshihide SONODA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//#include <sys/ioctl.h>
//#include <system/drivers/sound/soundcard.h>

#include <sys/fcntl.h>
#include <sys/types.h>

#include "wavefmt.h"

#include <user/tmalloc.h>


//#define DEFAULT_DSP	"/dev/sb16"
//#define DEFAULT_DSP	"/dev/sound/sb/dsp"
#define DEFAULT_DSP	"/dev/pci/es1370.0"
//#define DEFAULT_DSP	"/dev/null"
#define DEFAULT_BUFFERSIZE (2048*8)
//#define perror	//
#define DEBUG 1

#define IOCTL 0


/* ؿץȥ */
int readWaveFile(int fd, PWAVEFORMAT pwavefmt, u_int *datasize);
int openDSP(const char* devname, PWAVEFORMAT pwf);
int CloseDSP(int fd);
int playWave(int data_fd, int dsp_fd, u_int datasize);
int playRaw(int data_fd, int dsp_fd);
int play(const char* filename);

/* ���Хѿ */
static int vflag = 1;
static int cflag = 0;
static int rflag = 0;
//static int sampling_rate = 44100;
//static int channels = 2;
//static int bits_per_sample = 16;

static size_t bsize = DEFAULT_BUFFERSIZE;
static char *dsp_fname = DEFAULT_DSP;
static char *sbuf = NULL;

int main( int argc, char **argv )
{
    // TODO bring in good malloc/free and implement sbrk()!
    static char arena[1024*1024];
    init_malloc( arena, sizeof(arena) );


    if ( argc == 1 ) {
        //printf( "Usage: wavplay [WAVE-File]\n" );
        //return -1;
        return play( "/amnt0/siren.wav" );

    }
    return play( argv[ 1 ] );
}

int play(const char* filename)
{
    int in_fd;
    int out_fd;
    WAVEFORMAT wf;
    u_int datasize;
    int rc;
    int stdin_flag = !strcmp(filename, "-");

    if (stdin_flag)
        in_fd = 0; //STDIN_FILENO;
    else
    {
        if ((in_fd = open(filename, O_RDONLY)) < 0)
        {
            printf("%s - ", filename);
            perror("Open");
            return in_fd;
        }
    }

    /*   if (rflag)
     {
     wf.nSamplesPerSec = sampling_rate;
     wf.wBitsPerSample = bits_per_sample;
     wf.nChannels = channels;
     }
     else*/
    {
        if (readWaveFile(in_fd, &wf, &datasize))
        {
            close(in_fd);
            return -1;
        }
    }

    if (vflag)
    {
        if (!stdin_flag)
            printf("File name     : %s\n", filename);
        printf("Sampling rate : %ld Hz\n", wf.nSamplesPerSec);
        printf("Bits/Sample   : %d Bits\n", wf.wBitsPerSample);
        printf("Channels      : %d\n", wf.nChannels);
        if (!rflag)
            printf("Size          : %d Bytes\n", datasize);
        printf("\n");
    }
#if 1
    char *pnTmp = dsp_fname;
#else
    char anName[ 256 ];
    char *pnTmp;

    if ( GetStandardOutput( anName ) ) {
        return -1;
    }
    int len = strlen( anName ) + 20;

    pnTmp = malloc( len );
    if( pnTmp == NULL )
        return -2;

    snprintf( pnTmp, len, "/dev/sound/%s/dsp", anName );
#endif
    /*if (cflag)
     out_fd = STDOUT_FILENO;
     else*/
    {
        if ((out_fd = openDSP(pnTmp, &wf)) < 0)
        {
            perror("openDSP");
            close(in_fd);
            return -1;
        }
    }

    sbuf = (char *)malloc(bsize);
    if (sbuf == NULL)
    {
        printf("Error: Can't alloc memory.");
        return -1;
    }

    if (rflag)
        rc = playRaw(in_fd, out_fd);
    else
        rc = playWave(in_fd, out_fd, datasize);

    close(in_fd);
    if (!cflag)
        CloseDSP(out_fd);

    free(sbuf);
    return rc;
}

int readWaveFile(int fd, PWAVEFORMAT pwavefmt, u_int *datasize)
{
    int header = 0;
    int size = 0;
    char *buff;

    *datasize = 0;
    read(fd, (char *)&header, sizeof(int));
    if (header != H_RIFF)
    {
        printf("Error: Not RIFF file.\n");
        return 1;
    }

    read(fd, (char *)&size, sizeof(int));
    read(fd, (char *)&header, sizeof(int));
    if (header != H_WAVE)
    {
        printf("Error: Not WAVE file.\n");
        return 2;
    }

    while(read(fd, (char *)&header, sizeof(int)) == sizeof(int))
    {
        read(fd, (char *)&size, sizeof(int));

        if (header == H_FMT)
        {
            if ((size_t)size < sizeof(WAVEFORMAT))
            {
                printf("Error: Illegal header.\n");
                return 3;
            }
            buff = malloc((size_t)size);
            read(fd, buff, size);
            memcpy((void *)pwavefmt, (void *)buff, sizeof(WAVEFORMAT));
            free(buff);
            if (pwavefmt->wFormatTag != 1)
            {
                printf("Error: Unsupported format(0x%x).\n",
                      pwavefmt->wFormatTag);
                return 4;
            }
        }
        else if (header == H_DATA)
        {
            /* եݥ󥿤data󥯤ãؿλ */
            *datasize = (u_int)size;
            return 0;
        }
        else
        {
            /* ;פʥ󥯤ɤФ */
            lseek(fd, size, SEEK_CUR);
        }
    }

    printf("Error: data chunk not found.\n");
    return 10;
}

int openDSP(const char* devname, PWAVEFORMAT pwf)
{
    int fd;
    //int status;
    int arg;
    /*
    char anName[ 256 ];

    if ( GetStandardOutput( anName ) ) {
        return -1;
    }
    */

    if ((fd = open( devname, O_WRONLY)) < 0)
        return fd;

    /* ͥ(STEREO or MONAURAL) */
    if ( (int)(pwf->nChannels) == 1 ) {
        arg = 0;
    } else {
        arg = 1;
    }

    if (fd < 0)
    {
        perror("openDSP");
        close(fd);
        return -1;
    }


    //status = ioctl(fd, SNDCTL_DSP_STEREO, arg);
    /*if (status < 0)
     {
     perror("openDSP");
     Close(fd);
     return -1;
     }*/

    /*if (arg != (int)(pwf->nChannels))
     {
     printf("Can't set channels.\n");
     Close(fd);
     return -1;
     }*/

#if IOCTL
    arg = (int)(pwf->nSamplesPerSec);
    status = ioctl(fd, SNDCTL_DSP_SPEED, arg);
    if (status < 0)
    {
        perror("openDSP");
        printf("Can't set sampling rate.\n");
        close(fd);
        return -1;
    }

    if (vflag && (arg != (int)pwf->nSamplesPerSec))
    {
        printf("Warning: Can't set sampling rate %d Hz.\n",
              (int)pwf->nSamplesPerSec);
        printf("Using %d Hz instead.\n", arg);
    }
#ifdef DEBUG
    printf("DSP - Sampling rate: %d\n", arg);
#endif
#endif


#if IOCTL
    /* ̻Ҳӥåȿ(8 or 16Bit) */
    arg = (int)(pwf->wBitsPerSample);
    status = ioctl(fd, SNDCTL_DSP_SETFMT, arg);
    if (status < 0)
    {
        perror("openDSP");
        close(fd);
        return -1;
    }
    if (arg != (int)(pwf->wBitsPerSample))
    {
        if (vflag)
            printf("Can't set bit width.\n");
        close(fd);
        return -1;
    }
#endif

    return fd;
}

int CloseDSP(int fd)
{
    //ioctl(fd, SNDCTL_DSP_SYNC, 0);
    return close(fd);
}

int playRaw(int data_fd, int dsp_fd)
{
    register int nr, nw, off;

    while ((nr = read(data_fd, sbuf, bsize)) > 0)
    {
        for (off = 0; nr; nr -= nw, off += nw)
        {
            nw = write(dsp_fd, sbuf + off, nr);
            if(nw < 0)
            {
                printf("Error: playRaw - Write data\n");
                return -1;
            }
        }
    }

    return 0;
}

int playWave(int data_fd, int dsp_fd, u_int datasize)
{
    register int i, nr, nw, off;
    int tr, rd;

#ifdef DEBUG
    printf("datasize = %d, bsize =  %d\n", datasize, bsize);
#endif
    tr = datasize / bsize;
    rd = datasize % bsize;

    for (i = 0; i < tr + 1; i++)
    {
        if (i == tr)
        {
            if (rd == 0)
                break;

            if ((nr = read(data_fd, sbuf, rd)) <= 0)
                break;
        }
        else
        {
            if ((nr = read(data_fd, sbuf, bsize)) <= 0)
                break;
        }
        for (off = 0; nr; nr -= nw, off += nw)
        {
            if ((nw = write(dsp_fd, sbuf + off, nr)) < 0)
            {
                printf("Error: playWave - Write data\n");
                return -1;
            }
            //printf("wrote %d\n", nw);
        }
    }

    return 0;
}
