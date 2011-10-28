// vid_null.c -- Phantom OS video driver

#include "quakedef.h"
#include "d_local.h"
//#include "keys.h"

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/unistd.h>
#include <video/rect.h>
#include <video/point.h>
#include <video/color.h>
#include <event.h>
#include <dev/key_event.h>

static int fb;


#define FB_SET_COLOR(__fd,__c) ioctl(__fd, IOCTL_FB_SETCOLOR, &__c,sizeof(__c) )
#define FB_DRAW_BOX(__fd,__r) ioctl(__fd, IOCTL_FB_DRAWBOX, &__r,sizeof(__r) )
#define FB_FILL_BOX(__fd,__r) ioctl(__fd, IOCTL_FB_FILLBOX, &__r,sizeof(__r) )
#define FB_DRAW_LINE(__fd,__r) ioctl(__fd, IOCTL_FB_DRAWLINE, &__r,sizeof(__r) )
#define FB_DRAW_PIXEL(__fd,__x, __y) \
    do { point_t p; p.x = __x; p.y = __y; \
    ioctl(__fd, IOCTL_FB_DRAWPIXEL, &p,sizeof(p) ); \
    } while(0)




viddef_t	vid;				// global video state

#define	BASEWIDTH	320
#define	BASEHEIGHT	200

byte	vid_buffer[BASEWIDTH*BASEHEIGHT];
short	zbuffer[BASEWIDTH*BASEHEIGHT];
byte	surfcache[256*1024];

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];

void	VID_SetPalette (unsigned char *palette)
{
}

void	VID_ShiftPalette (unsigned char *palette)
{
}

void	VID_Init (unsigned char *palette)
{
    vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
    vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
    vid.aspect = 1.0;
    vid.numpages = 1;
    vid.colormap = host_colormap;
    vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
    vid.buffer = vid.conbuffer = vid_buffer;
    vid.rowbytes = vid.conrowbytes = BASEWIDTH;

    d_pzbuffer = zbuffer;
    D_InitCaches (surfcache, sizeof(surfcache));

    fb = open("/dev/etc/fb.0", O_RDWR );

    if( fb < 0 )
    {
        printf("can't open framebuf\n");
        exit(1);
    }

    char a[2000];
    memset( a, 0x6F, sizeof(a) );
    write( fb, a, sizeof(a) );

    rect_t r;
    r.xsize = BASEWIDTH;
    r.ysize = BASEHEIGHT;
    r.x = 350;
    r.y = 300;

    ioctl( fb, IOCTL_FB_SETBOUNDS, &r, sizeof(r) );


}

void	VID_Shutdown (void)
{
}

void	VID_Update (vrect_t *rects)
{
}

/*
 ================
 D_BeginDirectRect
 ================
 */
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
 ================
 D_EndDirectRect
 ================
 */
void D_EndDirectRect (int x, int y, int width, int height)
{
}


static void process_event( struct ui_event *e )
{
    int key_make;

    //SHOW_FLOW( 2, "e type %x exp %x", e->type, UI_EVENT_TYPE_KEY );

    switch(e->type)
    {
    //case UI_EVENT_TYPE_MOUSE: 	defaultMouseEventProcessor(w, &e); break;


    case UI_EVENT_TYPE_KEY:

        //printf( "key %x/%x mod %x\n", e->k.vk, e->k.ch, e->modifiers );

        // Ignore key up events
        //if(e->modifiers & UI_MODIFIER_KEYUP)            return;

        key_make = !(e->modifiers & UI_MODIFIER_KEYUP);

        //printf( "keydown %x/%x exp %x\n", e->k.vk, e->k.ch, KEY_ARROW_DOWN );

        /*
        switch(e->k.ch)
        {
        case ' ':
            //k_down++;
            return;

        case 0x1B:
            close(fb);
            exit(0);

        }*/

        if(e->k.ch)
        {
            Key_Event ( e->k.ch, key_make );
            return;
        }

        switch(e->k.vk)
        {
        case KEY_ARROW_DOWN:
        case 0xB9:
            //k_down++;
            //SHOW_FLOW( 2, "KEY__DOWN %d", k_down );
            Key_Event ( K_DOWNARROW, key_make );
            break;

        case KEY_ARROW_UP:
        case 0xBF:
            //SHOW_FLOW( 2, "KEY__UP %d", k_down );
            Key_Event ( K_UPARROW, key_make );
            break;

        case KEY_ARROW_LEFT:
        case 0xBB:
            //SHOW_FLOW( 2, "KEY__LEFT %d", k_down );
            Key_Event ( K_LEFTARROW, key_make );
            break;

        case KEY_ARROW_RIGHT:
        case 0xBD:
            //k_right++;
            //SHOW_FLOW( 2, "KEY__RIGHT %d", k_down );
            Key_Event ( K_RIGHTARROW, key_make );
            break;
        }
        return;

    }

}


static int get_event( struct ui_event *e )
{
    int ret = read( fb, e, sizeof(struct ui_event) );
    //printf("-re- %d (exp %d)\n", ret, sizeof(struct ui_event) );
    if( ret < 0 )
        printf("-re- ");

    return ret == sizeof(struct ui_event);
}


static void input(void)
{
    struct ui_event e;

    if(get_event( &e ))
    {
        //printf("pe %x\n", e.type );
        process_event( &e );
    }
}



#if 0
void IN_MouseMove (usercmd_t *cmd)
{
	int		mx, my;

	if (!mouse_avail)
		return;

	regs.x.ax = 11;		// read move
	dos_int86(0x33);
	mx = (short)regs.x.cx;
	my = (short)regs.x.dx;

	if (m_filter.value)
	{
		mouse_x = (mx + old_mouse_x) * 0.5;
		mouse_y = (my + old_mouse_y) * 0.5;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}
	old_mouse_x = mx;
	old_mouse_y = my;

	mouse_x *= sensitivity.value;
	mouse_y *= sensitivity.value;

// add mouse X/Y movement to cmd
	if ( (in_strafe.state & 1) || (lookstrafe.value && (in_mlook.state & 1) ))
		cmd->sidemove += m_side.value * mouse_x;
	else
		cl.viewangles[YAW] -= m_yaw.value * mouse_x;

	if (in_mlook.state & 1)
		V_StopPitchDrift ();

	if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
	{
		cl.viewangles[PITCH] += m_pitch.value * mouse_y;
		if (cl.viewangles[PITCH] > 80)
			cl.viewangles[PITCH] = 80;
		if (cl.viewangles[PITCH] < -70)
			cl.viewangles[PITCH] = -70;
	}
	else
	{
		if ((in_strafe.state & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}
}
#endif



void IN_Init (void)
{
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
#if 0
    int		i;

    regs.x.ax = 3;		// read buttons
    dos_int86(0x33);
    mouse_buttonstate = regs.x.bx;

    // perform button actions
    for (i=0 ; i<mouse_buttons ; i++)
    {
        if ( (mouse_buttonstate & (1<<i)) &&
             !(mouse_oldbuttonstate & (1<<i)) )
        {
            Key_Event (K_MOUSE1 + i, true);
        }
        if ( !(mouse_buttonstate & (1<<i)) &&
             (mouse_oldbuttonstate & (1<<i)) )
        {
            Key_Event (K_MOUSE1 + i, false);
        }
    }

    mouse_oldbuttonstate = mouse_buttonstate;

#endif
}

void IN_Move (usercmd_t *cmd)
{
    //IN_MouseMove (cmd);
}

