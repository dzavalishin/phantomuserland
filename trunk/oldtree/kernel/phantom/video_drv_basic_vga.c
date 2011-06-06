/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Last resort VGA videodriver. System is nearly unusable with this.
 * Sets 320*240 (so called 'X') mode.
 *
**/

#ifdef ARCH_ia32

#include <kernel/vm.h>

#include <phantom_libc.h>
#include <i386/pio.h>


#include "hal.h"
#include "video.h"
#include "video_drv_basic_vga.h"

#include "console.h"

// void DumpVgaMode(void)
#include "misc.h"


struct basic_VGA_Vmode basic_VGA_Mode;

static inline void setpal(int color, char r, char g, char b)
{
    outb( 0x3C8, color );
    outb( 0x3C9, r );
    outb( 0x3C9, g );
    outb( 0x3C9, b );
}

/*
static void setpalette4()
{
    setpal( 0,  0,  0,  0);
    setpal( 1,  0, 42, 42);
    setpal( 2, 42,  0, 42);
    setpal( 3, 63, 63, 63);
}*/

extern char basic_VGA_Pal[];

static void setpalette16()
{
    int j = 0;
    int i;
    for (i = 0; i < 48; i+=3)
    {
        setpal(j, basic_VGA_Pal[i], basic_VGA_Pal[i+1], basic_VGA_Pal[i+2]);
        j++;
    }
}

static void setpalette256()
{
    int j = 0;
    int i;
    for ( i = 0; i < 768; i+=3)
    {
        setpal(j, basic_VGA_Pal[i], basic_VGA_Pal[i+1], basic_VGA_Pal[i+2]);
        j++;
    }
}


static void SetMode(char *regs)
{
    outb( MISC_ADDR, *regs++ );
    outb( STATUS_ADDR, *regs++ );

    int i;
    for( i = 0; i < 5; i++ )
    {
        outb( SEQ_ADDR, i );
        outb( SEQ_ADDR+1, *regs++ );
    }

    outw( CRTC_ADDR, 0x0E11 & 0x7FFF );

    for( i = 0; i < 25; i++ )
    {
        outb( CRTC_ADDR, i );
        outb( CRTC_ADDR+1, *regs++ );
    }


    // Send GRAPHICS regs

    for( i = 0; i < 9; i++ )
    {
        outb( GRACON_ADDR, i );
        outb( GRACON_ADDR+1, *regs++ );
    }

    //int status =
    inb(STATUS_ADDR);

    // Send ATTRCON regs
    for( i = 0; i < 21; i++ )
    {
        //int val =
        inw(ATTRCON_ADDR);
        outb( ATTRCON_ADDR, i );
        outb( ATTRCON_ADDR, *regs++ );
    }

    outb( ATTRCON_ADDR, 0x20 );
}

extern char basic_VGA_mode03h[];


void video_drv_basic_vga_set_text_mode()
{
    SetMode(basic_VGA_mode03h);
    setpalette16();
    //ReadBIOSfont(6,16);

    basic_VGA_Mode.width = 80;
    basic_VGA_Mode.height = 25;
    basic_VGA_Mode.width_bytes = 2000;
    basic_VGA_Mode.colors = 16;
    basic_VGA_Mode.attrib = TVU_TEXT;
}

extern char basic_VGA_mode13h[];

void video_drv_basic_vga_set_basic_gfx_mode()
   {
      SetMode(basic_VGA_mode13h);
      setpalette256();

      basic_VGA_Mode.width = 320;
      basic_VGA_Mode.height = 200;
      basic_VGA_Mode.width_bytes = 64000u;
      basic_VGA_Mode.colors = 256;
      basic_VGA_Mode.attrib = TVU_GRAPHICS;
   }

extern char basic_VGA_modeC4[];

#if 0
static void video_drv_basic_vga_set_C4_mode()
{
    SetMode(basic_VGA_modeC4);
    setpalette256();

    basic_VGA_Mode.width = 320;
    basic_VGA_Mode.height = 200;
    basic_VGA_Mode.width_bytes = 16000;
    basic_VGA_Mode.colors = 256;
    basic_VGA_Mode.attrib = TVU_GRAPHICS | TVU_UNCHAINED;
}
#endif

// unchained 320 x 240 x 256
void video_drv_basic_vga_set_X_mode()
{
    SetMode(basic_VGA_modeC4);

    outb(MISC_ADDR,0xE3);
    // turn off write protect
    outw(CRTC_ADDR,0x2C11);
    // vertical total
    outw(CRTC_ADDR,0x0D06);
    // overflow register
    outw(CRTC_ADDR,0x3E07);
    // vertical retrace start
    outw(CRTC_ADDR,0xEA10);
    // vertical retrace end AND wr.prot
    outw(CRTC_ADDR,0xAC11);
    // vertical display enable end
    outw(CRTC_ADDR,0xDF12);
    // start vertical blanking
    outw(CRTC_ADDR,0xE715);
    // end vertical blanking
    outw(CRTC_ADDR,0x0616);

    setpalette256();
    basic_VGA_Mode.width = 320;
    basic_VGA_Mode.height = 240;
    basic_VGA_Mode.width_bytes = 19200;
    basic_VGA_Mode.colors = 256;
    basic_VGA_Mode.attrib = TVU_GRAPHICS | TVU_UNCHAINED;
}



// ----------------------------------------------------------------


static inline unsigned get_fb_seg(void)
{

    const int VGA_GC_INDEX 		= 0x3CE;
    const int VGA_GC_DATA 		= 0x3CF;

    outb(VGA_GC_INDEX, 6);
    unsigned seg = inb(VGA_GC_DATA);

    seg >>= 2;
    seg &= 3;

    switch(seg)
    {
    case 0:
    case 1:        return 0xA000;
    case 2:        return 0xB000;
    case 3:        return 0xB800;
    }

    // can't get here. but...
    return 0xA000;
}


static inline void setPlane( int plane )
{
    outb(0x3c4,0x02);
    outb(0x3c5,(0x01<<plane));
}

/*
 * The idea here is that 8-bit indexed color palette is made so that
 * bits are devoted to R, G and B: 3, 3 and 2 corresponding.
 *
 *
 */

static inline unsigned char rgba2palette( const struct rgba_t src )
{
    return
        (((src.r >> 5) & 0x7u) << 5) |
        (((src.g >> 5) & 0x7u) << 3) |
         ((src.b >> 6) & 0x3u);
}

#define DBG_BLT 0

// nelem supposed to have 2 lower bits zeroed
void basic_vga_unchained_rgba2palette_move( char *dest, const struct rgba_t *src, int nelem )
{
    nelem /= 4;

#if DBG_BLT
    int total = 0;
#endif
    int plane;
    for( plane = 0; plane < 4; plane++ )
    {
#if DBG_BLT
        printf("plane %d: ", plane);
#else
        setPlane( plane );
#endif

        int pos;
        char *out = dest;
        const struct rgba_t *in = src+plane;

        for( pos = 0; pos < nelem; pos++)
        {
#if DBG_BLT
            printf(" %x = %02x%02x%02x", rgba2palette( *in ), in->r, in->g, in->b);
            total++;
#else
            *out++ = rgba2palette( *in );
#endif
            in += 4;
        }
    }
#if DBG_BLT
    printf("total = %d\n", total );
#endif
}


static void set_rgb332_palette()
{
    int color;
    // VGA has 6-bit palette
    for( color = 0; color <= 0xFF; color++ )
        setpal( color,
                ((color >> 5) & 0x7) << 3, // top 3 bits
                ((color >> 2) & 0x7) << 3, // middle 3 bits
                (color & 0x3) << 4
              );
}

// ----------------------------------------------------------------


static int basic_vga_probe() { return 1; } // TODO maybe some real probe??
static int basic_vga_start();
static int basic_vga_stop();


#define SHADOW 1

static struct rgba_t* shadow_copy;

#define SHADOW_REVERSE_START(xpos,ypos) (  ( video_driver_basic_vga.xsize * ((video_driver_basic_vga.ysize -1) - ypos) + xpos ) + shadow_copy)
#define SHADOW_FORWARD_START(xpos,ypos) (  ( video_driver_basic_vga.xsize * (ypos) + xpos) + shadow_copy)


#define DRV_VIDEO_REVERSE_LINESTART(ypos) (  ( video_drv->xsize * ((video_drv->ysize -1) - ypos) )/4 + video_drv->screen)
#define DRV_VIDEO_FORWARD_LINESTART(ypos) ((video_drv->xsize * ypos)/4 + video_drv->screen)


void 	basic_vga_bitblt(const struct rgba_t *from, int xpos, int ypos, int xsize, int ysize)
{
    //printf("bit blt pos (%d,%d) size (%d,%d)\n", xpos, ypos, xsize, ysize);
    assert(video_drv->screen != 0);

    int xafter = xpos+xsize;
    int yafter = ypos+ysize;

    if( xafter <= 0 || yafter <= 0
        || xpos >= video_drv->xsize || ypos >= video_drv->ysize )
        return; // Totally clipped off

    // Where to start in source line
    int xshift = (xpos < 0) ? -xpos : 0;

    // Which source line to start from
    int yshift = (ypos < 0) ? -ypos : 0;

    if( yshift > 0 )
    {
        // This one is easy candy

        //printf("yshift = %d\n", yshift );
        from += xsize*yshift; // Just skip some lines;
        ysize -= yshift; // Less lines to go
        ypos += yshift;
        assert(ypos == 0);
        yshift = 0;
    }
    assert(yshift == 0);
    assert(xshift >= 0);


    //printf("xshift = %d\n", xshift );

    // xlen is how many pixels to move for each line
    int xlen = xsize;
    if( xafter > video_drv->xsize )
    {
        xlen -= (xafter - video_drv->xsize);
    }
    xlen -= xshift;
    assert(xlen > 0);
    assert(xlen <= xsize);
    assert(xlen <= video_drv->xsize);

    if( yafter > video_drv->ysize )
    {
        yafter = video_drv->ysize;
    }

    //char *lowest_line = ((drv_video_screen.ysize -1) - ypos) + drv_video_screen.screen;

    int sline = ypos;
    int wline = 0;


    //printf("xlen = %d, sline = %d yafter=%d \n", xlen, sline, yafter );
#define reverse 1

    if(reverse)
    {

        for( ; sline < yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_REVERSE_LINESTART(sline) + xpos*1;
            // Window start pos in line
            const struct rgba_t *w_start = from + ((wline*xsize) + xshift);

            basic_vga_unchained_rgba2palette_move( (void *)s_start, w_start, xlen );
#if SHADOW
            void *sh = SHADOW_REVERSE_START(xpos,sline);
            memmove( sh, w_start, xlen * sizeof(struct rgba_t) );
#endif
        }
    }
    else
    {
        for( ; sline < yafter; sline++, wline++ )
        {
            // Screen start pos in line
            char *s_start = DRV_VIDEO_FORWARD_LINESTART(sline) + xpos*1;
            // Window start pos in line
            const struct rgba_t *w_start = from + ((wline*xsize) + xshift);

            basic_vga_unchained_rgba2palette_move( (void *)s_start, w_start, xlen );

#if SHADOW
            void *sh = SHADOW_FORWARD_START(xpos,sline);
            memmove( sh, w_start, xlen * sizeof(struct rgba_t) );
#endif
        }
    }

    drv_video_update();

}

static void 	basic_vga_winblt(const drv_video_window_t *from, int xpos, int ypos)
{
    basic_vga_bitblt( from->pixel, xpos, ypos, from->xsize, from->ysize );
}


static void basic_vga_readblt( rgba_t *to, int xpos, int ypos, int xsize, int ysize)
{
    int xafter = xpos+xsize;
    int yafter = ypos+ysize;

    if( xafter <= 0 || yafter <= 0
        || xpos >= video_drv->xsize || ypos >= video_drv->ysize )
        return; // Totally clipped off

    // Where to start in source line
    int xshift = (xpos < 0) ? -xpos : 0;

    // Which source line to start from
    int yshift = (ypos < 0) ? -ypos : 0;

    if( yshift > 0 )
    {
        // This one is easy candy

        //printf("yshift = %d\n", yshift );
        to += xsize*yshift; // Just skip some lines;
        ysize -= yshift; // Less lines to go
        ypos += yshift;
        assert(ypos == 0);
        yshift = 0;
    }
    assert(yshift == 0);
    assert(xshift >= 0);


    //printf("xshift = %d\n", xshift );

    // xlen is how many pixels to move for each line
    int xlen = xsize;
    if( xafter > video_drv->xsize )
    {
        xlen -= (xafter - video_drv->xsize);
    }
    xlen -= xshift;
    assert(xlen > 0);
    assert(xlen <= xsize);
    assert(xlen <= video_drv->xsize);

    if( yafter > video_drv->ysize )
    {
        yafter = video_drv->ysize;
    }

    //char *lowest_line = ((drv_video_screen.ysize -1) - ypos) + drv_video_screen.screen;

    int sline = ypos;
    int wline = 0;


    //printf("xlen = %d, sline = %d yafter=%d \n", xlen, sline, yafter );
#define reverse 1

#if SHADOW
    if(reverse)
    {

        for( ; sline < yafter; sline++, wline++ )
        {
            // Window start pos in line
            struct rgba_t *w_start = to + ((wline*xsize) + xshift);

            void *sh = SHADOW_REVERSE_START(xpos,sline);
            memmove( w_start, sh, xlen * 4 );
        }
    }
    else
    {
        for( ; sline < yafter; sline++, wline++ )
        {
            // Window start pos in line
            struct rgba_t *w_start = to + ((wline*xsize) + xshift);

            void *sh = SHADOW_FORWARD_START(xpos,sline);
            memmove( w_start, sh, xlen * 4 );
        }
    }
#endif
}





static void basic_vga_map_video(int on_off)
{
    const int cnt = (64*1024)/4096;

    int baseaddr = get_fb_seg() << 4;

    printf("VGA video mem at 0x%x\n", baseaddr );

    int i;
    // need 64K aperture
    for( i = 0; i < cnt; i++ )
    {
        int pa = baseaddr + (i << 12);
        hal_page_control( pa, (void *)phystokv( pa ), on_off ? page_map : page_unmap, page_rw );
    }

    video_driver_basic_vga.screen = (void *)phystokv( baseaddr );

}


// TODO VGA 400*300
struct drv_video_screen_t        video_driver_basic_vga =
{
    "Basic VGA",
    // size
    320, 240,
    // mouse x y flags
    0, 0, 0,

    // screen
screen:	0,

probe: 			basic_vga_probe,
start: 			basic_vga_start,
stop:   		basic_vga_stop,

update:    		drv_video_null,
bitblt:    		(void *)basic_vga_bitblt,
winblt: 		(void *)basic_vga_winblt,
readblt: 		basic_vga_readblt,

mouse:    		drv_video_null,

    // mouse has to be implememnted separately for this card :(
#if 0
redraw_mouse_cursor:    drv_video_draw_mouse_deflt,
set_mouse_cursor:       drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:           drv_video_mouse_on_deflt,
#else
redraw_mouse_cursor:    drv_video_null,
set_mouse_cursor:       (void *)drv_video_null,
mouse_disable:          drv_video_null,
mouse_enable:           drv_video_null,
#endif

};


static void clearPlane(int plane, char clc)
{
    setPlane( plane );
    char *p = video_driver_basic_vga.screen;
    int sz = basic_VGA_Mode.width_bytes;
        //video_driver_basic_vga.xsize * video_driver_basic_vga.ysize;
    while( sz-- )
        *p++ = clc;
}

static void clrscr()
{
    // TODO? 4 planes at once
    if( basic_VGA_Mode.attrib & TVU_UNCHAINED )
    {
        int plane;
        for( plane = 0; plane <4; plane ++ )
        {
            //clearPlane(plane, plane << 5);
            clearPlane( plane, 0 );
        }
    }
    else
    {
        clearPlane(0, 0);
    }
}

//static void test_vga_drv();



static int basic_vga_start()
{
    int sh_size = sizeof(struct rgba_t) *
                             video_driver_basic_vga.xsize *
                             video_driver_basic_vga.ysize;
    if( shadow_copy == 0 )
        shadow_copy = malloc( sh_size );

    basic_vga_map_video(1);
    //video_drv_basic_vga_set_basic_gfx_mode();
    video_drv_basic_vga_set_X_mode();
    basic_vga_map_video(1);

    set_rgb332_palette();

    clrscr();

    //test_vga_drv();

    return 0;
}

static int basic_vga_stop()
{
    video_drv_basic_vga_set_text_mode();
    basic_vga_map_video(1);
    clrscr();
    // Don't unmap for it will kill text mode too!
    // basic_vga_map_video(0);
    return 0;
}



#if 0
static void test_vga_drv()
{
    printf("VGA test data\n");
    //getchar();

#define sz 19200*4
    static struct rgba_t test[sz];

    memset( test, sizeof(test), 0 );

    int i;
    for( i = 0; i < sz; i += 2 )
    {
        test[i].a = 0xFF;

        test[i].r = ((i & 0x6) == 0x2) ? 0xFF : 0;
        test[i].g = ((i & 0x6) == 0x4) ? 0xFF : 0;
        test[i].b = ((i & 0x6) == 0x6) ? 0xFF : 0;

        test[i+1] = test[i];
    }

#if DBG_BLT
    hexdumpb(0, test, sz*4 );
#endif

    printf("test VGA driver mode switch\n");
    //getchar();
#if !DBG_BLT
    //basic_vga_start();
#endif

#if 0
    basic_vga_unchained_rgba2palette_move(
                                          video_driver_basic_vga.screen,
                                          test, sz );
#else
    basic_vga_bitblt(test, 0, 0,
                     video_driver_basic_vga.xsize,
                     video_driver_basic_vga.ysize
                    );
#endif

    //getchar();
    //basic_vga_stop();
}
#endif


void write_vga_register(int port, int index, int v) 
{
    if (port==0x3C0)
    {
        inb(0x3DA);
        outb(port, index);
        outb(port, v);
    }
    else
    {
        outb(port, index);
        outb(port+1, v);
    }
}

int read_vga_register(int port, int index)
{
   if (port==0x3C0)
      inb(0x3DA);

   outb(port, index);
   return inb(port+1);
}







// indexed = 0 for single reg
// indexed = N for N indirect registers
static void dreg( const char *name, int regbase, int indexed )
{
    printf(".%s = { ", name);

    if( !indexed )
        printf("0x%x ", inb(regbase));
    else
    {
        int port = 0;
        for(port = 0; port < indexed; port++)
        {
            outb( regbase, port );
            int v = inb( regbase+1 );

            printf("0x%x, ", v );
        }
    }

    printf("}, // %s\n", name);
}




void DumpVgaMode(void)
{
    dreg( "CRTC", 0x03D4, 256 );
    dreg( "ATTR", 0x03C0, 256 );
    dreg( "SEQR", 0x03C4, 256 );
    dreg( "GRAC", 0x03CE, 256 );
    dreg( "MSR",  0x03CC, 0 ); // Wr = 0x03C2
    dreg( "FCR",  0x03CA, 0 ); // Wr = 0x03DA
    dreg( "DACW", 0x03C8, 256 );
    dreg( "GF2D", 0x03D6, 256 );

    inb(STATUS_ADDR);
}







#endif // ARCH_ia32


