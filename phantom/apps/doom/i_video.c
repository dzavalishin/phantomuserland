#if 0
// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//      DOOM graphics stuff for SDL library
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>

#include "m_swap.h"
#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#include "kolibri.h"

void BlitDIB();

typedef struct SURFACE
{
        unsigned char *pixels;  
        int w, h;               
        int pitch;              
        int offset;             
} SURFACE;
 
SURFACE screen;

// Fake mouse handling.
boolean         grabMouse;

// Blocky mode,
// replace each 320x200 pixel with multiply*multiply pixels.
// According to Dave Taylor, it still is a bonehead thing
// to use ....
static int      multiply=2;

static int disableVerticalMouse = 0;

static int scr_w =0;
static int scr_h =0;
static int win_x, win_y;
static int win_w, win_h;



//int palette_color[256];


static int      lastmousex = 0;
static int      lastmousey = 0;
boolean         mousemoved = false;


void I_ShutdownGraphics(void)
{

}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?
    
   //printf("start new frame\n\r" );

}

int test_for_event(void)
{ int retval;
  _asm
 {   push ebx
     push ecx
     push edx
     push esi
     push edi
     mov  eax,11
     int  0x40
     mov [retval], eax
     pop edi
     pop esi
     pop edx
     pop ecx
     pop ebx 
 };
 return retval; 
}; 


void I_GetEvent(void)
{
  event_t evnt;
  int val;
  int key;
  //printf("begin get_event()\n\r");
  do
  { val= test_for_event();
    switch(val)
    { case EV_REDRAW:
        BeginDraw();
        DrawWindow(win_x,win_y,win_w,win_h,0x404040,3,0,0,0);
        EndDraw();
        BlitDIB();
        break;

      case EV_KEY:
        if(!get_key(&key))
        { 
          switch(key)
          {  case 0xE0:
             case 0xE1:
               continue;
             default:
               if(key&0x80)
               { //printf("key released\n\r");
                 evnt.type = ev_keyup; 
               }
               else
               { //printf("key pressed %x\n\r",key);
                 evnt.type = ev_keydown; 
               };
               key&=0x7F; 
               evnt.data1=remap_key(key);
               if ( evnt.data1 != 0 )
                 D_PostEvent(&evnt);
               continue;
          };     
        };  
        continue;

      case EV_BUTTON:
        switch(get_button_id())
        {  case 1:
             return;
             
          default:;
        };
    };   
  }while(val);
}



//
// I_StartTic
//
void I_StartTic (void)
{
    I_GetEvent();
    
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// I_SetPalette
//

unsigned int colors[256];

void I_SetPalette (byte* palette)
{
    int i;
    unsigned int r;
    unsigned int g;
    unsigned int b;
    
    for ( i=0; i<256; ++i )
    {
          r = gammatable[usegamma][*palette++];
          g = gammatable[usegamma][*palette++];
          b = gammatable[usegamma][*palette++];
          colors[i]= b|(g<<8)|(r<<16);
    }
}

void BeginDraw()
{ _asm
  { push ebx
    mov   eax,12
    mov   ebx, 1
    int   0x40
    pop ebx
  };  
};

void EndDraw()
{ _asm
  { push ebx
    mov   eax,12
    mov   ebx, 2
    int   0x40
    pop ebx
  };  
};

void I_InitGraphics(void)
{
    static int firsttime=1;

    int frameX, frameY;

    if (!firsttime)
        return;
    firsttime = 0;

    if (M_CheckParm("-2"))
        multiply = 2;

    if (M_CheckParm("-3"))
        multiply = 3;

    if (M_CheckParm("-4"))
        multiply = 4;

    win_w = SCREENWIDTH * multiply;
    win_h = SCREENHEIGHT * multiply;

    // check if the user wants to grab the mouse (quite unnice)
    grabMouse = !!M_CheckParm("-grabmouse");

    /* [Petteri] New: Option to disable mouse vertical movement - useful
       for players used to Quake: */
    disableVerticalMouse = !!M_CheckParm("-novertmouse");

    /* Build and initialize the window: */
    
    frameX = 0;
    frameY = 18;
    
    GetScreenSize(&scr_w, &scr_h);
    win_x = (scr_w-win_w)/2;
    win_y = (scr_h-win_h)/2;
      
 //   hCursor = LoadCursor( 0, IDC_ARROW );
    
    printf("I_InitGraphics: Client area: %ux%u\n\r", win_w, win_h);

    screen.pixels= (byte*) UserAlloc(640*400*4);
    screen.h = win_w;
    screen.w = win_h;
    screen.pitch=640;
    
    BeginDraw();
    DrawWindow(win_x,win_y,win_w,win_h,0x404040,3,0,0,0);
    EndDraw();

}


void conv(char *dst, char *src);

void BlitDIB()
{
  _asm
  {  push ebx
     push ecx
     push edx 
     mov eax, 7
     mov ebx, dword ptr [screen]
     mov ecx, 0x02800190
     xor edx, edx
     int 0x40
     pop edx
     pop ecx
     pop ebx
  } 
};

//
// I_FinishUpdate
// 
void I_FinishUpdate (void)
{
    char *dst0;
    char *dst1;
    char *src;
        
    int x, y;
    
    dst0 = &((char *)screen.pixels)[0];
    dst1 = dst0+640*3;
    src= (char*)screens[0];
    y = SCREENHEIGHT;
    while (y--)
    {
      x = SCREENWIDTH;
      conv(dst0,src);
      src+=320;
      dst0+=640*3*2;
      dst1+=640*3*2;
    }
    BlitDIB();
}

void conv(char *dst, char *src)
{
   _asm
   {
     push ebx
     push ecx
     push esi
     push edi
     
     mov ecx, 320
     mov esi, [src]
     mov edi, [dst]
     cld
l1:     
     lodsb
     movzx eax, al
     mov eax, [colors+eax*4]
     mov ebx, eax
     bswap ebx
     and ebx, 0xFF000000
     or eax, ebx
     mov [edi],eax
     mov [edi+640*3], eax
     shr eax, 8
     mov [edi+4], ax
     mov [edi+640*3+4], ax
     add edi, 6
     dec ecx
     jnz l1 
     
     pop edi
     pop esi
     pop ecx
     pop ebx     
  };  
};   
   
   


#endif
