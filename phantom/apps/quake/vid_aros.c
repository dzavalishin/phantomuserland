#if 0
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
// vid_aros.c -- general AROS video driver



#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/cybergraphics.h>

#include <intuition/screens.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>

#include <cybergraphx/cybergraphics.h>

#include <exec/memory.h>

#include <devices/inputevent.h>

#include "quakedef.h"
#include "d_local.h"

static UBYTE *aros_framebuffer;
static long aros_highhunkmark;
static long aros_buffersize;
static struct Screen *aros_screen = NULL;

BOOL aros_size_changed = FALSE;

BOOL aros_use_wb_screen = TRUE;
BOOL aros_use_cgfx = FALSE;

static ULONG cgfx_coltab[256];

struct Window *aros_win = NULL;
LONG aros_win_innerleft, aros_win_innertop
	, aros_win_innerbottom, aros_win_innerright
	, aros_win_innerwidth, aros_win_innerheight;

	

int vid_surfcachesize;
void *vid_surfcache;

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase *GfxBase = NULL;
struct Library *CyberGfxBase = NULL;
struct Library *KeymapBase = NULL;

void aros_vid_shutdown();


/*  SYMBOLS NEEDED FROM OTHER FILES */
viddef_t vid; // global video state
unsigned short d_8to16table[256];

static void calc_window_dims()
{
     aros_win_innerleft   = aros_win->BorderLeft;
     aros_win_innertop	  = aros_win->BorderTop;
     aros_win_innerright  = aros_win->Width - aros_win->BorderRight - 1;
     aros_win_innerbottom = aros_win->Height - aros_win->BorderBottom - 1;
				
     aros_win_innerwidth  = aros_win_innerright  - aros_win_innerleft + 1;
     aros_win_innerheight = aros_win_innerbottom - aros_win_innertop  + 1;
     
     /* NOTE !! Quake needs the alignment below */
     aros_win_innerwidth &= ~7;
     aros_win_innerright = aros_win_innerleft + aros_win_innerwidth - 1;
     
}

static void reset_framebuffer()
{
	aros_size_changed = FALSE;
	
	if (aros_framebuffer) {
		FreeVec(aros_framebuffer);
	}
	
	if (d_pzbuffer) {
		D_FlushCaches();
		Hunk_FreeToHighMark(aros_highhunkmark);
		d_pzbuffer = NULL;
		
	}
	
	aros_highhunkmark = Hunk_HighMark();
	
// alloc an extra line in case we want to wrap, and allocate the z-buffer
	aros_buffersize = vid.width * vid.height * sizeof (*d_pzbuffer);

/* fprintf(stderr, "RFB: Size for each pixel: %d, total buffer size: %ld\n"
	, sizeof (*d_pzbuffer), aros_buffersize);
*/	
	vid_surfcachesize = D_SurfaceCacheForRes(vid.width, vid.height);
	
	aros_buffersize +=vid_surfcachesize;
	
	d_pzbuffer = Hunk_HighAllocName(aros_buffersize, "video");
	if (NULL == d_pzbuffer) {
		Sys_Error("Not enough memory for video mode\n");
	}
	
	vid_surfcache = (byte *)d_pzbuffer
		+ vid.width * vid.height *sizeof (*d_pzbuffer);
	
	D_InitCaches(vid_surfcache, vid_surfcachesize);
	
	aros_framebuffer = AllocVec(vid.width * vid.height, MEMF_ANY);

printf("New Quake dims: %dx%d\n", vid.width, vid.height);

	if (NULL == aros_framebuffer) {
		Sys_Error("Not enough memory for framebuffer\n");
	}
	
	vid.buffer = aros_framebuffer;
	vid.conbuffer = vid.buffer;
	
}

// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	VID_Init (unsigned char *palette)
{
   int verbose;
   int pnum;
   
   printf("VID_INIT\n");

   if ((pnum=COM_CheckParm("-cgfx"))) {
	     aros_use_cgfx = TRUE;
   }
   
   

#warning Move this to main() ?
   IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
   if (NULL == IntuitionBase) {
   	Sys_Error("Error opening intuition.library");
	return;
   }
   
   if (aros_use_cgfx) {
   	CyberGfxBase = OpenLibrary("cybergraphics.library", 0);
	if (NULL == CyberGfxBase) {
   	    Sys_Error("Error opening cybergraphics.library");
	    return;
	}
   } else {
   
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (NULL == GfxBase) {
	    Sys_Error("Error opening graphics.library");
	    return;
	}
   }
   
   KeymapBase = OpenLibrary("keymap.library", 37);
   if (NULL == KeymapBase) {
   	Sys_Error("Error opening keymap.library");
	return;
   }
   
   
   
//    ignorenext=0;
   vid.width = 320;
   vid.height = 200;
   vid.maxwarpwidth = WARP_WIDTH;
   vid.maxwarpheight = WARP_HEIGHT;
   vid.numpages = 2;
   vid.colormap = host_colormap;
   //	vid.cbits = VID_CBITS;
   //	vid.grades = VID_GRADES;
   vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
   
    verbose=COM_CheckParm("-verbose");


// check for command-line window size
     if ((pnum=COM_CheckParm("-winsize")))
     {
     	     if (pnum >= com_argc-2)
     		     Sys_Error("VID: -winsize <width> <height>\n");
     	     vid.width = Q_atoi(com_argv[pnum+1]);
     	     vid.height = Q_atoi(com_argv[pnum+2]);
     	     if (!vid.width || !vid.height)
     		     Sys_Error("VID: Bad window width/height\n");
     }
     if ((pnum=COM_CheckParm("-width"))) {
     	     if (pnum >= com_argc-1)
     		     Sys_Error("VID: -width <width>\n");
     	     vid.width = Q_atoi(com_argv[pnum+1]);
     	     if (!vid.width)
     		     Sys_Error("VID: Bad window width\n");
     }
     if ((pnum=COM_CheckParm("-height"))) {
     	     if (pnum >= com_argc-1)
     		     Sys_Error("VID: -height <height>\n");
     	     vid.height = Q_atoi(com_argv[pnum+1]);
     	     if (!vid.height)
     		     Sys_Error("VID: Bad window height\n");
     }
     

     if ((pnum=COM_CheckParm("-ownscreen"))) {
	     aros_use_wb_screen = FALSE;
     }

    if (aros_use_wb_screen) {
   
	aros_screen = LockPubScreen(NULL);
	if (NULL == aros_screen) {
	    Sys_Error("Could not get WB screen\n");
	   return;
	}

	UnlockPubScreen(NULL, aros_screen);
	
    } else {
   

	aros_screen = OpenScreenTags(NULL
		, SA_Width, 640
		, SA_Height, 480
		, TAG_DONE);
    }
			
   if (NULL == aros_screen) {
   	Sys_Error("Could not get screen\n");
	return;
   }
	
	 
     
     aros_win = OpenWindowTags(NULL
     	, WA_InnerWidth, 	vid.width
	, WA_InnerHeight, 	vid.height
	, WA_DragBar,		TRUE
	, WA_SimpleRefresh,	TRUE
	, WA_IDCMP,		IDCMP_RAWKEY|IDCMP_NEWSIZE|IDCMP_CLOSEWINDOW
	, WA_CustomScreen,	aros_screen
	, WA_Title,		"AROS Quake !"
	, WA_DepthGadget,	TRUE
	, WA_SizeGadget,	TRUE
	, WA_CloseGadget,	TRUE
	, WA_SizeBBottom,	TRUE
	, WA_Activate,		TRUE
	, TAG_DONE
     );
     
     
     if (!aros_win) {
     	Sys_Error("Could not open window\n");
	return;
     }

     WindowLimits(aros_win, 320  + aros_win->BorderLeft + aros_win->BorderRight,
     			    200  + aros_win->BorderTop  + aros_win->BorderBottom,
			    1024 + aros_win->BorderLeft + aros_win->BorderRight,
			    1024 + aros_win->BorderTop  + aros_win->BorderBottom);
     
     VID_SetPalette(palette);
     
     calc_window_dims();
     vid.width  = aros_win_innerwidth;
     vid.height = aros_win_innerheight;
     reset_framebuffer();
     

    vid.rowbytes = vid.width;
    vid.buffer = aros_framebuffer;
    vid.direct = 0;
    vid.conbuffer = aros_framebuffer;
    vid.conrowbytes = vid.rowbytes;
    vid.conwidth = vid.width;
    vid.conheight = vid.height;
    vid.aspect = ((float)vid.height / (float)vid.width) * (320.0 / 240.0);


}

void VID_ShiftPalette(unsigned char *p)
{
// printf("VID_ShiftPalette\n");
	VID_SetPalette(p);
}


#define NUMCOLORS 256L

void VID_SetPalette(unsigned char *palette)
{
    if (aros_use_cgfx) {
    	int i;
    	/* Just copy the colors to the palette */
	for (i = 0; i < NUMCOLORS; i ++) {
	    cgfx_coltab[i] =  (palette[i*3] << 16)
	    		     |(palette[i*3+1] << 8)
			     | palette[i*3+2];
	}
    
    
    } else {
	ULONG coltab[NUMCOLORS * 3 + 2];
	int i;
    
	ULONG *colcell;
	colcell = &coltab[1];
    
	coltab[0] = (NUMCOLORS << 16) + 0;
    
	for (i = 0; i < NUMCOLORS; i ++)
	{
	    colcell[0] = palette[i*3]   << 24;
	    colcell[1] = palette[i*3+1] << 24;
	    colcell[2] = palette[i*3+2] << 24;
	
	    colcell += 3;
	}
	colcell [0] = 0;
   
	LoadRGB32(&aros_screen->ViewPort, coltab); 
    }

}

// Called at shutdown

void aros_vid_shutdown()
{	
	if (NULL != aros_framebuffer) {
		FreeVec(aros_framebuffer);
	}
	if (NULL != aros_win) {
		CloseWindow(aros_win);
	}
	
	if (!aros_use_wb_screen) {
		if (NULL != aros_screen) {
			CloseScreen(aros_screen);
		}
	}
	
	if (NULL != KeymapBase) {
		CloseLibrary(KeymapBase);
		
	}
	
	if (NULL != GfxBase) {
		CloseLibrary((struct Library *)GfxBase);
	}
	
	if (NULL != IntuitionBase) {
		CloseLibrary((struct Library *)IntuitionBase);
	}
}

void	VID_Shutdown (void)
{
printf("VID_Shutdown\n");

	Con_Printf("VID_Shutdown\n");
	
	aros_vid_shutdown();

}

// flushes the given rectangles from the view buffer to the screen

void	VID_Update (vrect_t *rects)
{


//	vrect_t full;

// if the window changes dimension, skip this frame
	if (aros_size_changed)
	{
printf("config notify\n");
		aros_size_changed = FALSE;
		
		calc_window_dims();
		vid.width = aros_win_innerwidth;
		vid.height = aros_win_innerheight;
		
printf("New dims: %d, %d\n", vid.width, vid.height);		
		
		reset_framebuffer();
		vid.rowbytes = vid.width;
		vid.buffer = aros_framebuffer;
		vid.conbuffer = vid.buffer;
		vid.conwidth = vid.width;
		vid.conheight = vid.height;
		vid.conrowbytes = vid.rowbytes;
		vid.recalc_refdef = 1;				// force a surface cache flush
		Con_CheckResize();
		Con_Clear_f();
		return;
	}
	

	while (rects)
	{
		
#warning Optimize this. Just rerender the part inside the rect
		if (aros_use_cgfx) {
		
		    WriteLUTPixelArray(
		    	aros_framebuffer
		    	, 0, 0
			, aros_win_innerwidth
			, aros_win->RPort
			, cgfx_coltab
			, aros_win_innerleft
			, aros_win_innertop
			, aros_win_innerwidth
			, aros_win_innerheight
			, CTABFMT_XRGB8
		    );
			
		} else {
		
		    /* Just do a full screen update for now */		
		    SetDrMd(aros_win->RPort, JAM1);
		    WritePixelArray8(aros_win->RPort
			, aros_win_innerleft
			, aros_win_innertop
			, aros_win_innerright
			, aros_win_innerbottom
			, aros_framebuffer
			, NULL
		    );
		}
		
	    rects = rects->pnext;
		
	}


}

void GetEvent(void)
{

	struct IntuiMessage *imsg;
	BOOL quit = FALSE;
	
	if ((imsg = (struct IntuiMessage *)GetMsg(aros_win->UserPort))) {
		switch (imsg->Class) {
			
		case IDCMP_NEWSIZE:
			/* The window is resized */
			aros_size_changed = TRUE;
			break;
			
		case IDCMP_RAWKEY: {
			LONG key = -1;
			BOOL down;
			
			
			if (imsg->Code & 0x80) {
// kprintf("UP\n");			
				down = FALSE;
			} else {
// kprintf("DOWN\n");			
				down = TRUE;
			}
			

			switch (imsg->Code & ~0x80) {
				case 0x45: key = K_ESCAPE;	break;
				case 0x44: key = K_ENTER;	break;
				
				case 0x4F: key = K_LEFTARROW;	break;
				case 0x4E: key = K_RIGHTARROW;	break;
				case 0x4C: key = K_UPARROW;	break;
				case 0x4D: key = K_DOWNARROW;	break;
				
				case 0x60:
				case 0x61: key = K_SHIFT;	break;
				
				case 0x63: key = K_CTRL;	break;
				
				case 0x64: key = K_ALT;	break;
				
				default: {
					UBYTE buf[10];
					LONG chars;
					
					struct InputEvent ie;
					memset (&ie, 0, sizeof (ie));
					
					
					ie.ie_NextEvent  = NULL;
					ie.ie_Class = IECLASS_RAWKEY;
					ie.ie_Code  = imsg->Code & ~0x80;
					ie.ie_Qualifier = imsg->Qualifier;
					
					
					chars = MapRawKey(&ie, buf, 10, NULL);
					
// printf("GOT KEY %c, chars=%ld\n", *buf, chars);					
					
					if (1 == chars) {
						key = *buf;
						if (key >= 'A' && key <= 'Z') {
							key = key - 'A' + 'a';
						}
					break; }
					
					
				}
			
			}
			if (-1 != key) {
					
				Key_Event(key, down);
			}
			break; }
			
			case IDCMP_CLOSEWINDOW:
				quit = TRUE;
				break;
			
		}
		
		ReplyMsg((struct Message *)imsg);
	}
	
	if (quit) {
		Sys_Quit();
	}
	
	return;
}
static int dither;

void VID_DitherOn(void)
{
printf("VID_DitherOn\n");

    if (dither == 0)
    {
		vid.recalc_refdef = 1;
        dither = 1;
    }
}

void VID_DitherOff(void)
{
printf("VID_DitherOff\n");
    if (dither)
    {
		vid.recalc_refdef = 1;
        dither = 0;
    }
}

int Sys_OpenWindow(void)
{
	return 0;
}

void Sys_EraseWindow(int window)
{
}

void Sys_DrawCircle(int window, int x, int y, int r)
{
}

void Sys_DisplayWindow(int window)
{
}

void Sys_SendKeyEvents(void)
{
// printf("Sys_SendKeyEvents\n");
	GetEvent();

}


void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
// direct drawing of the "accessing disk" icon isn't supported under Linux
}

void D_EndDirectRect (int x, int y, int width, int height)
{
// direct drawing of the "accessing disk" icon isn't supported under Linux
}

#endif
