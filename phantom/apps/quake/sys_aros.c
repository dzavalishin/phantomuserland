#if 0

#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <exec/lists.h>
#include <exec/memory.h>

#include <dos/dos.h>
#include <utility/tagitem.h>

#include "quakedef.h"




qboolean			isDedicated;

int nostdout = 0;

char *basedir = ".";
char *cachedir = "/tmp";

cvar_t  sys_linerefresh = {"sys_linerefresh","0"};// set for entity display


// =======================================================================
// AROS Specific types
// =======================================================================

struct FileNode {
    struct MinNode node;
    int handle;
    BPTR file;
};


// =======================================================================
// AROS Specific variables
// =======================================================================

struct MinList filelist;
ULONG current_handleno;
APTR aros_membase = NULL;

// =======================================================================
// AROS Specific functions
// =======================================================================

static void AROS_Sys_Init()
{
    NEWLIST(&filelist);
    current_handleno = 0;
}

static int AROS_Sys_AddOpenFile(BPTR file)
{
    struct FileNode *fn;
    fn = AllocVec(sizeof (*fn), MEMF_ANY);
    if (NULL == fn)
    	return -1;
    
    fn->file = file;
    fn->handle = current_handleno ++;
    
    AddTail((struct List *)&filelist, (struct Node *)fn);
    
    return fn->handle;
}

static BPTR AROS_Sys_GetOpenFile(int handle)
{
    struct FileNode *fn;
    ForeachNode(&filelist, fn) {
    	if (handle == fn->handle)
	    return fn->file;
    }
    return NULL;
}

static BPTR AROS_Sys_RemoveOpenFile(int handle)
{
    struct FileNode *fn, *safefn;
    ForeachNodeSafe(&filelist, fn, safefn) {
    	if (handle == fn->handle) {
	    BPTR file;
	    
	    file = fn->file;
	    Remove((struct Node *)fn);
	    FreeVec(fn);
	    
	    return file;
	}
    }
    return NULL;
}

// =======================================================================
// General routines
// =======================================================================



void Sys_DebugNumber(int y, int val)
{
}

/*
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	
	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);
	fprintf(stderr, "%s", text);
	
	Con_Print (text);
}

void Sys_Printf (char *fmt, ...)
{

    va_list     argptr;
    char        text[1024], *t_p;
    int         l, r;

	if (nostdout)
		return;

    va_start (argptr,fmt);
    vsprintf (text,fmt,argptr);
    va_end (argptr);

    l = strlen(text);
    t_p = text;

// make sure everything goes through, even though we are non-blocking
    while (l)
    {
        r = write (1, text, l);
        if (r != l)
            sleep (0);
        if (r > 0)
        {
            t_p += r;
            l -= r;
        }
    }

}
*/

void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;
	
	va_start (argptr,fmt);
	vsprintf (text,fmt,argptr);
	va_end (argptr);

	if (strlen(text) > sizeof(text))
		Sys_Error("memory overwrite in Sys_Printf");

    if (nostdout)
        return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
}

#if 0
static char end1[] =
"\x1b[?7h\x1b[40m\x1b[2J\x1b[0;1;41m\x1b[1;1H                QUAKE: The Doomed Dimension \x1b[33mby \x1b[44mid\x1b[41m Software                      \x1b[2;1H  ----------------------------------------------------------------------------  \x1b[3;1H           CALL 1-800-IDGAMES TO ORDER OR FOR TECHNICAL SUPPORT                 \x1b[4;1H             PRICE: $45.00 (PRICES MAY VARY OUTSIDE THE US.)                    \x1b[5;1H                                                                                \x1b[6;1H  \x1b[37mYes! You only have one fourth of this incredible epic. That is because most   \x1b[7;1H   of you have paid us nothing or at most, very little. You could steal the     \x1b[8;1H   game from a friend. But we both know you'll be punished by God if you do.    \x1b[9;1H        \x1b[33mWHY RISK ETERNAL DAMNATION? CALL 1-800-IDGAMES AND BUY NOW!             \x1b[10;1H             \x1b[37mRemember, we love you almost as much as He does.                   \x1b[11;1H                                                                                \x1b[12;1H            \x1b[33mProgramming: \x1b[37mJohn Carmack, Michael Abrash, John Cash                \x1b[13;1H       \x1b[33mDesign: \x1b[37mJohn Romero, Sandy Petersen, American McGee, Tim Willits         \x1b[14;1H                     \x1b[33mArt: \x1b[37mAdrian Carmack, Kevin Cloud                           \x1b[15;1H               \x1b[33mBiz: \x1b[37mJay Wilbur, Mike Wilson, Donna Jackson                      \x1b[16;1H            \x1b[33mProjects: \x1b[37mShawn Green   \x1b[33mSupport: \x1b[37mBarrett Alexander                  \x1b[17;1H              \x1b[33mSound Effects: \x1b[37mTrent Reznor and Nine Inch Nails                   \x1b[18;1H  For other information or details on ordering outside the US, check out the    \x1b[19;1H     files accompanying QUAKE or our website at http://www.idsoftware.com.      \x1b[20;1H    \x1b[0;41mQuake is a trademark of Id Software, inc., (c)1996 Id Software, inc.        \x1b[21;1H     All rights reserved. NIN logo is a registered trademark licensed to        \x1b[22;1H                 Nothing Interactive, Inc. All rights reserved.                 \x1b[40m\x1b[23;1H\x1b[0m";
static char end2[] =
"\x1b[?7h\x1b[40m\x1b[2J\x1b[0;1;41m\x1b[1;1H        QUAKE \x1b[33mby \x1b[44mid\x1b[41m Software                                                    \x1b[2;1H -----------------------------------------------------------------------------  \x1b[3;1H        \x1b[37mWhy did you quit from the registered version of QUAKE? Did the          \x1b[4;1H        scary monsters frighten you? Or did Mr. Sandman tug at your             \x1b[5;1H        little lids? No matter! What is important is you love our               \x1b[6;1H        game, and gave us your money. Congratulations, you are probably         \x1b[7;1H        not a thief.                                                            \x1b[8;1H                                                           Thank You.           \x1b[9;1H        \x1b[33;44mid\x1b[41m Software is:                                                         \x1b[10;1H        PROGRAMMING: \x1b[37mJohn Carmack, Michael Abrash, John Cash                    \x1b[11;1H        \x1b[33mDESIGN: \x1b[37mJohn Romero, Sandy Petersen, American McGee, Tim Willits        \x1b[12;1H        \x1b[33mART: \x1b[37mAdrian Carmack, Kevin Cloud                                        \x1b[13;1H        \x1b[33mBIZ: \x1b[37mJay Wilbur, Mike Wilson     \x1b[33mPROJECTS MAN: \x1b[37mShawn Green              \x1b[14;1H        \x1b[33mBIZ ASSIST: \x1b[37mDonna Jackson        \x1b[33mSUPPORT: \x1b[37mBarrett Alexander             \x1b[15;1H        \x1b[33mSOUND EFFECTS AND MUSIC: \x1b[37mTrent Reznor and Nine Inch Nails               \x1b[16;1H                                                                                \x1b[17;1H        If you need help running QUAKE refer to the text files in the           \x1b[18;1H        QUAKE directory, or our website at http://www.idsoftware.com.           \x1b[19;1H        If all else fails, call our technical support at 1-800-IDGAMES.         \x1b[20;1H      \x1b[0;41mQuake is a trademark of Id Software, inc., (c)1996 Id Software, inc.      \x1b[21;1H        All rights reserved. NIN logo is a registered trademark licensed        \x1b[22;1H             to Nothing Interactive, Inc. All rights reserved.                  \x1b[23;1H\x1b[40m\x1b[0m";

#endif
void Sys_Quit (void)
{
	struct FileNode *node, *nextnode;
	printf("Sys_Quit\n");
	
	/* Free the open files in the filenode list */
	node = (struct FileNode *)filelist.mlh_Head;
	
	ForeachNodeSafe(&filelist, node, nextnode) {
		Remove((struct Node *)node);
		Close(node->file);
		FreeVec(node);
	}
	
	Host_Shutdown();
#if 0
#warning What does this mean ?
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	if (registered.value)
		printf("%s", end2);
	else
		printf("%s", end1);
#endif

	if (NULL != aros_membase) {
		FreeVec(aros_membase);
	}
	fflush(stdout);
	exit(0);
}

void Sys_Init(void)
{
#if id386
	Sys_SetFPCW();
#endif
}

void Sys_Error (char *error, ...)
{ 
    va_list     argptr;
    char        string[1024];
    
// change stdin to non blocking
#if 0
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif
    
    va_start (argptr,error);
    vsprintf (string,error,argptr);
    va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

    Sys_Quit();

#if 0
	Host_Shutdown ();
	exit (1);
#endif	

} 

void Sys_Warn (char *warning, ...)
{ 
    va_list     argptr;
    char        string[1024];

    va_start (argptr,warning);
    vsprintf (string,warning,argptr);
    va_end (argptr);
	fprintf(stderr, "Warning: %s", string);
} 

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{

    struct FileInfoBlock *fib;
    int time = -1;
    BPTR lock;
    
    printf("Sys_FileTime(path=%s)\n", path);
	
    fib = AllocDosObject(DOS_FIB, 0);
    if (fib) {
	    
	lock = Lock(path, ACCESS_READ);
	if (lock) {
	
	    if (Examine(lock, fib)) {
#warning Fix this time conversion	    
	    	// DateStamp(&fib->fib_Date);
		time = 123;
		
	    }
	    UnLock(lock);
	}
	
	FreeDosObject(DOS_FIB, fib);
    }
	
    return time;

#if 0
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;

	return buf.st_mtime;
#endif	
	
}


void Sys_mkdir (char *path)
{
    BPTR lock;
    
    printf("Sys_mkdir(%s)\n", path);
    
    lock = CreateDir(path);
    if (lock) {
    	UnLock(lock);
    }
    
#if 0    
    mkdir (path, 0777);
#endif    
}



int Sys_FileOpenRead (char *path, int *handle)
{
	BPTR lock, file;
	int size = -1;
	int h = -1;
	
	printf("Sys_FileOpenRead(%s)\n", path);

	lock = Lock(path, ACCESS_READ);
	if (lock) {
		struct FileInfoBlock *fib;
	    
		fib = AllocDosObject(DOS_FIB, NULL);
		if (fib) {
			if (Examine(lock, fib)) {
				size = fib->fib_Size;
			}
			FreeDosObject(DOS_FIB, fib);
		}
		UnLock(lock);
	}
	
	if (-1 == size)
		return -1;
	
	file = Open(path, MODE_OLDFILE);
	if (!file)
		return -1;
	
	h = AROS_Sys_AddOpenFile(file);
	if (-1 == h) {
		Close(file);
		return -1;
	}
	
	*handle = h;
	
	printf("returned size: %d, handle=%d\n", size, h);
		
	return size;

#if 0
	int	h;
	struct stat	fileinfo;
    
	
	h = open (path, O_RDONLY, 0666);
	*handle = h;
	if (h == -1)
		return -1;
	
	if (fstat (h,&fileinfo) == -1)
		Sys_Error ("Error fstating %s", path);

	return fileinfo.st_size;
	
#endif	
}

int Sys_FileOpenWrite (char *path)
{
	BPTR file;
	int handle;
	printf("Sys_FileOpenWrite(%s)\n", path);
	
	file = Open(path, MODE_READWRITE);
	if (!file)
		return -1;
		
	handle = AROS_Sys_AddOpenFile(file);
	if (-1 == handle) {
		Close(file);
		return -1;
	}
	
	return handle;
	
#if 0	
	int     handle;

	umask (0);
	
	handle = open(path,O_RDWR | O_CREAT | O_TRUNC
	, 0666);

	if (handle == -1)
		Sys_Error ("Error opening %s: %s", path,strerror(errno));

	return handle;
#endif	
}

int Sys_FileWrite (int handle, void *src, int count)
{
	BPTR file;
	
	printf("Sys_FileWrite\n");
	file = AROS_Sys_GetOpenFile(handle);
	

	return Write (file, src, count);
#if 0
	return write (handle, src, count);
#endif	
}

void Sys_FileClose (int handle)
{
	BPTR file;
	
	printf("Sys_FileClose\n");
	
	file = AROS_Sys_RemoveOpenFile(handle);
	
	Close (file);

#if 0 
	close (handle);
#endif	
	
}

void Sys_FileSeek (int handle, int position)
{
	BPTR file;
	
	printf("Sys_FileSeek\n");
	
	file = AROS_Sys_GetOpenFile(handle);

	Seek (file, position, OFFSET_BEGINNING);
#if 0
	lseek (handle, position, SEEK_SET);
#endif	
}

int Sys_FileRead (int handle, void *dest, int count)
{

	BPTR file;
	
	int retval;
	
	printf("Sys_FileRead(handle=%d, dest=%p, count=%d\n"
		, handle, dest, count);
	
	file = AROS_Sys_GetOpenFile(handle);


	retval = Read (file, dest, count);
	
	printf("Returning %d\n", retval);
	
	return retval;
#if 0
	return read (handle, dest, count);
#endif    
}

void Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr; 
    static char data[1024];
    
    
#if 0    
    int fd;
#endif    
    BPTR fh;
    printf("Sys_DebugLog\n");
    
    va_start(argptr, fmt);
    vsprintf(data, fmt, argptr);
    va_end(argptr);
//    fd = open(file, O_WRONLY | O_BINARY | O_CREAT | O_APPEND, 0666);

    fh = Open(file, MODE_READWRITE);
    if (file) {
    	Write(fh, data, strlen(data));
	Close(fh);
    }

#if 0
    fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    write(fd, data, strlen(data));
    close(fd);
#endif    
}

void Sys_EditFile(char *filename)
{

	printf("!!! Sys_EditFile: not implemented uet\n");
#if 0
	char cmd[256];
	char *term;
	char *editor;
	
	term = getenv("TERM");
	if (term && !strcmp(term, "xterm"))
	{
		editor = getenv("VISUAL");
		if (!editor)
			editor = getenv("EDITOR");
		if (!editor)
			editor = getenv("EDIT");
		if (!editor)
			editor = "vi";
		sprintf(cmd, "xterm -e %s %s", editor, filename);
		system(cmd);
	}
#endif
}

double Sys_FloatTime (void)
{
    struct timeval tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;


#if 0
    struct timeval tp;
    struct timezone tzp; 
    static int      secbase; 
    
    gettimeofday(&tp, &tzp);  

    if (!secbase)
    {
        secbase = tp.tv_sec;
        return tp.tv_usec/1000000.0;
    }

    return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
#endif
    
}

// =======================================================================
// Sleeps for microseconds
// =======================================================================

static volatile int oktogo;

void alarm_handler(int x)
{
	oktogo=1;
}

void Sys_LineRefresh(void)
{
}

#if 0
void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
	signal(SIGFPE, floating_point_exception_handler);
}
#endif
char *Sys_ConsoleInput(void)
{
    static char text[256];
    
//    printf("Sys_ConsoleInput\n");
    
    if (cls.state == ca_dedicated) {
   
	FGets(Input(), text, 256);
    
	return text;
    }
    
    return NULL;
	    
    
#if 0
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (cls.state == ca_dedicated) {
		FD_ZERO(&fdset);
		FD_SET(0, &fdset); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
			return NULL;

		len = read (0, text, sizeof(text));
		if (len < 1)
			return NULL;
		text[len-1] = 0;    // rip off the /n and terminate

		return text;
	}
	return NULL;
#endif	
}

#if !id386
void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}
#endif

int main (int c, char **v)
{

	double		time, oldtime, newtime;
	quakeparms_t parms;
	extern int vcrFile;
	extern int recording;
	int j;

//	static char cwd[1024];

	AROS_Sys_Init();

//	signal(SIGFPE, floating_point_exception_handler);
#if 0
	signal(SIGFPE, SIG_IGN);
#endif	

	memset(&parms, 0, sizeof(parms));

	COM_InitArgv(c, v);
	parms.argc = com_argc;
	parms.argv = com_argv;

#ifdef GLQUAKE
	parms.memsize = 16*1024*1024;
#else
	parms.memsize = 8*1024*1024;
#endif

	j = COM_CheckParm("-mem");
	if (j)
		parms.memsize = (int) (Q_atof(com_argv[j+1]) * 1024 * 1024);
	parms.membase = AllocVec (parms.memsize, MEMF_ANY);
	
	aros_membase = parms.membase;
	printf("parms.membase: %p\n", parms.membase);

	parms.basedir = basedir;
// caching is disabled by default, use -cachedir to enable
//	parms.cachedir = cachedir;

#warning What does this mean ?
#if 0
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif	

printf("Initing host, memsize=%d\n", parms.memsize);
    Host_Init(&parms);

printf("Initing syst\n");
	Sys_Init();

printf("Check for nostdout\n");
	if (COM_CheckParm("-nostdout"))
		nostdout = 1;
	else {
#if 0	
#warning What does this mean ?
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif
		printf ("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
	}
printf("Entering main loop\n");
    oldtime = Sys_FloatTime () - 0.1;
    while (1)
    {
// find time spent rendering last frame
        newtime = Sys_FloatTime ();
        time = newtime - oldtime;

        if (cls.state == ca_dedicated)
        {   // play vcrfiles at max speed
            if (time < sys_ticrate.value && (vcrFile == -1 || recording) )
            {
#warning Fix this

//				usleep(1);
				
                continue;       // not time to run a server only tic yet
            }
            time = sys_ticrate.value;
        }

        if (time > sys_ticrate.value*2)
            oldtime = newtime;
        else
            oldtime += time;

        Host_Frame (time);

// graphic debugging aids
        if (sys_linerefresh.value)
            Sys_LineRefresh ();
    }

}


/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{

#if 0

	int r;
	unsigned long addr;
	int psize = getpagesize();

	addr = (startaddr & ~(psize-1)) - psize;

//	fprintf(stderr, "writable code %lx(%lx)-%lx, length=%lx\n", startaddr,
//			addr, startaddr+length, length);

	r = mprotect((char*)addr, length + startaddr - addr + psize, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");

#endif
}

#endif
