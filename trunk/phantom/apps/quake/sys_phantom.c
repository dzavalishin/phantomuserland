/*
 Copyright (C) 1996-1997 Id Software, Inc.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 */
// sys_null.h -- null system driver to aid porting efforts

#include "quakedef.h"
#include "errno.h"

#include <user/sys_fio.h>

#define BUFFERED_IO 0

#if BUFFERED_IO

/*
 ===============================================================================

 FILE IO

 ===============================================================================
 */

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             findhandle (void)
{
    int             i;

    for (i=1 ; i<MAX_HANDLES ; i++)
        if (!sys_handles[i])
            return i;
    Sys_Error ("out of handles");
    return -1;
}

/*
 ================
 filelength
 ================
 */
int filelength (FILE *f)
{
    int             pos;
    int             end;

    pos = ftell (f);
    fseek (f, 0, SEEK_END);
    end = ftell (f);
    fseek (f, pos, SEEK_SET);

    return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
    FILE    *f;
    int             i;

    i = findhandle ();

    f = fopen(path, "r");
    if (!f)
    {
        *hndl = -1;
        return -1;
    }
    sys_handles[i] = f;
    *hndl = i;

    return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
    FILE    *f;
    int             i;

    i = findhandle ();

    f = fopen(path, "wb");
    if (!f)
        Sys_Error ("Error opening %s: %s", path,strerror(errno));
    sys_handles[i] = f;

    return i;
}

void Sys_FileClose (int handle)
{
    fclose (sys_handles[handle]);
    sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
    fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
    return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
    return fwrite (data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime (char *path)
{
    FILE    *f;

    f = fopen(path, "rb");
    if (f)
    {
        fclose(f);
        return 1;
    }

    return -1;
}

void Sys_mkdir (char *path)
{
    mkdir(path);
}

#else
// unbuff

/*
 ===============================================================================

 FILE IO

 ===============================================================================
 */

#include <fcntl.h>
#include <sys/stat.h>


int Sys_FileOpenRead (char *path, int *hndl)
{
    //Con_Printf("open %s for read", path );
    int i = open(path, O_RDONLY, 0 );
    if (i < 0)
    {
        Con_Printf("Error opening %s for read: %s", path, strerror(errno));
    fail:
        *hndl = -1;
        return -1;
    }
    *hndl = i;

    struct stat st;

    if(fstat(i, &st) < 0){
        Con_Printf( "cannot stat %s\n", path);
        close(i);
        goto fail;
    }

    //Con_Printf("size = %d\n", st.st_size );

    return st.st_size;
}

int Sys_FileOpenWrite (char *path)
{
    int i = open(path, O_RDWR, 0666 );
    if (i < 0)
        Sys_Error("Error opening %s for write: %s", path, strerror(errno));

    return i;
}

void Sys_FileClose (int handle)
{
    close(handle);
}

void Sys_FileSeek (int handle, int position)
{
    lseek(handle, position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
    return read( handle, dest, count );
}

int Sys_FileWrite (int handle, void *data, int count)
{
    return write( handle, data, count );
}

int Sys_FileTime (char *path)
{

    int fd = open(path, O_RDONLY, 0 );
    if(fd>=0)
    {
        close(fd);
        return 1;
    }

    return -1;
}

void Sys_mkdir (char *path)
{
    mkdir(path);
}


#endif

/*
 ===============================================================================

 SYSTEM IO

 ===============================================================================
 */

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_Error (char *error, ...)
{
    va_list         argptr;

    printf ("Sys_Error: ");
    va_start (argptr,error);
    vprintf (error,argptr);
    va_end (argptr);
    printf ("\n");

    exit (1);
}

void Sys_Printf (char *fmt, ...)
{
    va_list         argptr;

    va_start (argptr,fmt);
    vprintf (fmt,argptr);
    va_end (argptr);
}

void Sys_Quit (void)
{
    exit (0);
}

double Sys_FloatTime (void)
{
    static double t;

    t += 0.1;

    return t;
}

char *Sys_ConsoleInput (void)
{
    return NULL;
}

/*
void Sys_Sleep (void)
{
    sleepmsec(1);
}
*/

void Sys_SendKeyEvents (void)
{
}

/*
 void Sys_HighFPPrecision (void)
 {
 }

 void Sys_LowFPPrecision (void)
 {
 }
 */

//=============================================================================

#define MEMSZ (12*1024*1024)

static char mem[MEMSZ];

void main (int argc, char **argv)
{
    static quakeparms_t    parms;

    parms.memsize = MEMSZ;
    parms.membase = mem;
    parms.basedir = "/amnt1";

    if(0 == parms.membase)
    {
        printf("out of mem\n");
        exit(1);
    }

    COM_InitArgv (argc, argv);

    parms.argc = com_argc;
    parms.argv = com_argv;

    printf ("Host_Init\n");
    Host_Init (&parms);
    while (1)
    {
        Host_Frame (0.1);
    }
}


