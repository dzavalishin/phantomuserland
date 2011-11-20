#ifdef ARCH_ia32
/*
 ** Copyright 2002-2005, Michael Noisternig. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include <newos/compat.h>


#include "ia32/vesa3/vesa3.h"
#include "gtf.h"
#include "ia32/vesa3/video_param_table.h"
#include <ia32/selector.h>

#include <ia32/seg.h>
#include <ia32/private.h>

#include <kernel/vm.h>
#include <kernel/init.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <hal.h>


#define PH_LDT_SEL 1

#define IO_SEL_SIZE (64*1024)
static void *      bios_io_mem;
static selector_id bios_io_sel;

//-----------------------------------------------------------------------------

// set for reading the BIOS from array biosdata - useful for debugging
//#define FAKE_VESA

// NOTE: many VESA VBE 3.0 implementations are broken
// I try to address these problems by patching the copy of the BIOS in RAM

//----------------------------------------------------------------------------

#ifdef FAKE_VESA
//#include "sapphire9200.h"
#include "nv4193e.h"
//#include "vivid!81.h"
//#include "prophet3.h"
#endif

#define STRING2(s) #s
#define STRING(s) STRING2(s)

struct s_regs {
    uint16	ax;
    uint16	bx;
    uint16	cx;
    uint16	dx;
    uint16	si;
    uint16	di;
    uint16	es;
} _PACKED;

//static devfs_framebuffer_info fb_info;

static vesa3_infoblock infoblock;
static vesa3_mode_infoblock mode_infoblock;

static selector_id sel_infoblock;

static unsigned char *video_bios;
static unsigned char *bios_data;
static unsigned char *bios_stack;

static selector_id sel_code;
static selector_id sel_stack;

static int vesa3_retn;  // for 16-bit far return or 32-bit near return

extern void vesa3_dummy_retf();
extern void vesa3_call_retf();
asm(
    ".global _vesa3_call_retf;"
    "_vesa3_call_retf:;"
    //"pushl %ds;"
    //"popl %gs;"
    //"pushl $0x40;"
    //"popl %ds;"
    "	addr16;"
    "	lcall	*_vesa3_retn;"
    //"	lcall	%gs:*vesa3_retn;"
    "	data16;"
    ".global _vesa3_dummy_retf;"
    "_vesa3_dummy_retf:;"
    "	lret;"
   );

static struct s_farcall {
    uint32	offset;
    uint32	seg;
} farcall = { (uint32)vesa3_dummy_retf, KERNEL_CS };

static void vesa_call( struct s_regs *regs )
{
    int ie = hal_save_cli();

    asm(
        "pushl	%%es;"
        "pushal;"
        "movl	%%esp,%%esi;"
        "pushl	_sel_stack;"
        "pushl	$("STRING(VESA_STACK_SIZE)"-4);"
        "call	_vesa3_stack_switch;"
        "pushl	$"STRING(KERNEL_DS)";"
        "pushl	%%esi;"
        "pushl	%%edi;"
        "pushl	%%ds;"
        "movw	12(%%edi),%%dx;"
        "movw	%%dx,%%es;"
        "movw	(%%edi),%%ax;"
        "movw	2(%%edi),%%bx;"
        "movw	4(%%edi),%%cx;"
        "movw	6(%%edi),%%dx;"
        "movw	8(%%edi),%%si;"
        "movw	10(%%edi),%%di;"
        "lcall	*_farcall;"
        "popl	%%ds;"
        "popl	%%ebp;"
        "movw	%%ax,%%ds:(%%ebp);"
        "movl	%%ebp,%%eax;"
        "movw	%%bx,2(%%eax);"
        "movw	%%cx,4(%%eax);"
        "movw	%%dx,6(%%eax);"
        "movw	%%si,8(%%eax);"
        "movw	%%di,10(%%eax);"
        "movw	%%es,%%bx;"
        "movw	%%bx,12(%%eax);"
        "call	_vesa3_stack_switch;"
        "popal;"
        "popl	%%es;"
        : : "D" (regs) );

    if( ie ) hal_sti();
}

//----------------------------------------------------------------------------

#if 0
static void *linear( void *ptr )
{
    return (void *)( *(unsigned short *)ptr + ((int)(((unsigned short *)ptr)[1])<<4) );
}
#endif

static void *memmem( const void *mem, size_t len, const void *submem, size_t sublen )
{
    const char *m = mem;
    const char *max = &m[len-sublen];

    for ( ; m < max; m++ ) {
        m = memchr( m, *(char *)submem, max-m );
        if ( m == NULL )
            break;
        if ( memcmp(m, submem, sublen) == 0 )
            return (void *)m;
    }

    return NULL;
}

static void resolve_farptr( uint32 *string_farptr )
{
    if ( (*string_farptr >> 16) == sel_infoblock && (*string_farptr &= 0xffff) < sizeof(vesa3_infoblock) )
        *string_farptr += (uint32)&infoblock;
    else
        *string_farptr = 0;
}

//----------------------------------------------------------------------------

static uint16 vesa3_get_mode_infoblock( uint16 mode )
{
    struct s_regs regs;

    regs.ax = 0x4f01;
    regs.cx = mode;
    //regs.es = i386_selector_add( SELECTOR(&mode_infoblock,sizeof(mode_infoblock)-1,DATA_w,false) );
    regs.es = bios_io_sel;
    regs.di = 0;
    vesa_call( &regs );
    //i386_selector_remove( regs.es );

    memcpy( &mode_infoblock, bios_io_mem, sizeof(mode_infoblock) );

    return regs.ax;
}

static uint16 vesa3_set_mode_CRTC_generic( uint16 mode, vesa3_CRTC_infoblock *crtc )
{
    /*	vesa3_CRTC_infoblock CRTC_info_block = {
     640+16+64+80, 640+16, 640+16+64,
     480+3+4+13, 480+3, 480+3+4,
     VESA3_CRTC_HSYNC_NEGATIVE, 23750000, 5938, {} };*/
    struct s_regs regs;

    regs.ax = 0x4f02;
    regs.bx = mode | 0xc000; //| 0xc800;
    regs.es = 0;
    /*	regs.es = i386_selector_add( SELECTOR(&CRTC_info_block,sizeof(CRTC_info_block)-1,DATA_w,false) );
     regs.di = 0;*/
    vesa_call( &regs );

    return regs.ax;
}

struct s_edid {
    uint16		dot_clock;			// pixel clock / 10000
    uint8		xres_lo;
    uint16		hblank : 12;		// blank = front porch + sync width + back porch
    uint8		xres_hi : 4;
    uint8		yres_lo;
    uint16		vblank : 12;
    uint8		yres_hi : 4;
    uint8		hoverplus_lo;		// overplus = front porch
    uint8		hsyncwidth_lo;
    uint8		vsyncwidth_lo : 4;
    uint8		voverplus_lo : 4;
    uint8		vsyncwidth_hi : 2;
    uint8		voverplus_hi : 2;
    uint8		hsyncwidth_hi : 2;
    uint8		hoverplus_hi : 2;
    uint8		_reserved[6];
} _PACKED;

static struct s_edid *ati_edid_dest;

static uint16 vesa3_set_mode_CRTC_ATI( uint16 mode, vesa3_CRTC_infoblock *crtc )
{
    /*	static const struct s_edid default_edid = {
     2375, 640&0xff, 160, 640>>8, 480&0xff, 20, 480>>8,
     16&0xff, 64&0xff, 4, 3&0xff, 4>>4, 3>>4, 64>>8, 16>>8, {}
     };*/
    struct s_regs regs;
    unsigned hoverplus = crtc->horizontal_sync_start - mode_infoblock.x_resolution;
    unsigned voverplus = crtc->vertical_sync_start - mode_infoblock.y_resolution;
    unsigned hsyncwidth = crtc->horizontal_sync_end - crtc->horizontal_sync_start;
    unsigned vsyncwidth = crtc->vertical_sync_end - crtc->vertical_sync_start;

    dprintf( "vesa3: ATI: writing EDID data\n" );

    ati_edid_dest->dot_clock = crtc->pixel_clock / 10000;
    ati_edid_dest->xres_lo = mode_infoblock.x_resolution & 0xff;
    ati_edid_dest->hblank = crtc->horizontal_total - mode_infoblock.x_resolution;
    ati_edid_dest->xres_hi = mode_infoblock.x_resolution >> 8;
    ati_edid_dest->yres_lo = mode_infoblock.y_resolution & 0xff;
    ati_edid_dest->vblank = crtc->vertical_total - mode_infoblock.y_resolution;
    ati_edid_dest->yres_hi = mode_infoblock.y_resolution >> 8;
    ati_edid_dest->hoverplus_lo = hoverplus & 0xff;
    ati_edid_dest->hsyncwidth_lo = hsyncwidth & 0xff;
    ati_edid_dest->vsyncwidth_lo = vsyncwidth & 0xf;
    ati_edid_dest->voverplus_lo = voverplus & 0xf;
    ati_edid_dest->vsyncwidth_hi = vsyncwidth >> 4;
    ati_edid_dest->voverplus_hi = voverplus >> 4;
    ati_edid_dest->hsyncwidth_hi = hsyncwidth >> 8;
    ati_edid_dest->hoverplus_hi = hoverplus >> 8;

    //	memcpy( ati_edid_dest, &default_edid, sizeof(struct s_edid) );
    //	memcpy( &video_bios[0x9b8e], &video_bios[0x9f26+9*0x12], 0x12 );

    dprintf( "vesa3: ATI: EDID: dot_clock %u, xres %u, yres %u, hblank %u, vblank %u, hsyncwidth %u, vsyncwidth %u, hoverplus %u, voverplus %u\n",
             ati_edid_dest->dot_clock,
             ati_edid_dest->xres_lo+(ati_edid_dest->xres_hi<<8),
             ati_edid_dest->yres_lo+(ati_edid_dest->yres_hi<<8),
             ati_edid_dest->hblank,
             ati_edid_dest->vblank,
             ati_edid_dest->hsyncwidth_lo+(ati_edid_dest->hsyncwidth_hi<<8),
             ati_edid_dest->vsyncwidth_lo+(ati_edid_dest->vsyncwidth_hi<<4),
             ati_edid_dest->hoverplus_lo+(ati_edid_dest->hoverplus_hi<<8),
             ati_edid_dest->voverplus_lo+(ati_edid_dest->voverplus_hi<<4)
           );

    regs.ax = 0x4f02;
    regs.bx = mode | 0xc000;
    regs.es = 0;
    vesa_call( &regs );

    return regs.ax;
}

static uint16 vesa3_set_mode_CRTC_nVidia( uint16 mode, vesa3_CRTC_infoblock *crtc )
{
    struct s_regs regs;

    regs.ax = 0x4f02;
    regs.bx = mode | 0xc000;
    regs.di = 0;
    regs.es = 0;
    vesa_call( &regs );

    return regs.ax;
}

static uint16 (*vesa3_set_mode_CRTC)(uint16, vesa3_CRTC_infoblock *) = vesa3_set_mode_CRTC_generic;

static uint16 vesa3_set_mode( unsigned int xres, unsigned int yres, unsigned int bpp, unsigned int refresh_rate )
{
    uint16 err = 0xffff;
    uint16 *mode = (uint16 *)infoblock.video_mode_ptr;

    if ( mode ) {
        dprintf( "vesa3: searching mode %ux%ux%u...\n", xres, yres, bpp );
        while ( *mode != 0xffff ) {
            err = vesa3_get_mode_infoblock( *mode );
            if ( err == 0x004f
                 && mode_infoblock.x_resolution == xres
                 && mode_infoblock.y_resolution == yres
                 && mode_infoblock.bits_per_pixel == bpp ) {
                struct vesa3_CRTC_infoblock crtc;
//#warning bring me back
                gtf_compute_CRTC_data( &crtc, xres, yres, refresh_rate<<10, false, false );

                if ( crtc.pixel_clock > mode_infoblock.max_pixel_clock && mode_infoblock.max_pixel_clock > 0 ) {
                    dprintf( "vesa3: pixel clock too high (max. is %u Hz)!\n", mode_infoblock.max_pixel_clock );
                    return -1;
                }
                dprintf( "vesa3: switching to video mode 0x%x...\n", *mode );
                err = vesa3_set_mode_CRTC( *mode, &crtc );
                break;
            }
            mode++;
        }
    }

    return err;
}

//----------------------------------------------------------------------------
/* For compatibility with the vesa (2.0) driver and demonstration purposes
 this driver goes to VESA VBE mode 640x480x32/15 when opened.
 Note that for going back to text mode you have to utilize a standard VGA
 driver as VESA VBE 3.0 does NOT support text modes.
 */
static int vesa3_open( void )
{
    int err = vesa3_set_mode( 640, 480, 32, 60 );

    dprintf( "vesa3: open(): vesa3_set_mode() returned 0x%x\n", err );

    if ( err != 0x004f ) {
        err = vesa3_set_mode( 640, 480, 15, 60 );  // let's try 15 bit mode instead
        dprintf( "vesa3: open(): vesa3_set_mode() returned 0x%x\n", err );
        if ( err != 0x004f )
            return ERR_GENERAL;
    }

    dprintf( "vesa3: physical video memory at 0x%x\n", mode_infoblock.phys_base_ptr );

#if 0
    fb_info.width = mode_infoblock.x_resolution;
    fb_info.height = mode_infoblock.y_resolution;
    fb_info.bit_depth = mode_infoblock.bits_per_pixel;

    switch ( fb_info.bit_depth ) {
    case  8: fb_info.color_space = COLOR_SPACE_8BIT; break;
    case 15: fb_info.color_space = COLOR_SPACE_RGB555; break;
    case 24: fb_info.color_space = COLOR_SPACE_RGB888; break;
    case 32: fb_info.color_space = COLOR_SPACE_RGB888+1; // COLOR_SPACE_RGB8888
    }
#endif

#if 0
    {
        unsigned i, x, y, rgb1, rgb2;
        void *video_mem;

        //draw a color-fading rectangle on screen
        vm_map_physical_memory( vm_get_kernel_aspace_id(), "linear_video_mem", (void**)&video_mem, REGION_ADDR_ANY_ADDRESS,
                                mode_infoblock.x_resolution * mode_infoblock.y_resolution * ((mode_infoblock.bits_per_pixel+7)/8),
                                LOCK_RW | LOCK_KERNEL, (addr_t)mode_infoblock.phys_base_ptr );

#define PIXEL8888(x,y,c) \
    *(uint32*)&((char*)video_mem)[((y)*640+(x))*4] = (c);

#define PIXEL888(x,y,c) \
    *(uint16*)&((char*)video_mem)[((y)*640+(x))*3] = (c)&0xffff; \
    *(uint8*)&((char*)video_mem)[((y)*640+(x))*3+2] = (c)>>16;

#define PIXEL555(x,y,c) \
    ((uint16*)video_mem)[((y)*640+(x))] = (((c)>>3)&0x1f) | (((c)>>6)&0x3e0) | (((c)>>9)&0x7c00);

#define PIXEL PIXEL8888

#if 1
        for ( i = 0; i < 20; i++ ) {
            for ( x = 0; x < 640*480; x++ ) {
                if ( (x/640)%30 < 10 ) {
                    PIXEL( x, 0, 0xff*(640-(x+i))/640 + ((0xff*(x+i)/640) << 8) );
                }
                else if ( (x/640)%30 < 20 ) {
                    PIXEL( x, 0, ((0xff*(640-(x+i))/640) << 8) + ((0xff*(x+i)/640) << 16) );
                }
                else {
                    PIXEL( x, 0, ((0xff*(640-(x+i))/640) << 16) + 0xff*(x+i)/640 );
                }
            }
            /*  	      for ( y = 0; y < 256; y++ ) {
             rgb1 = ((255-y)<<8)+(y<<16);
             rgb2 = ((y*(i&255)>>8)<<16)+((y*(i&255)>>8)<<8)+(255-y)+(y*(i&255)>>8);
             for ( x = 0; x < 256; x++ )
             ((unsigned*)video_mem)[y*640+x] =
             (((rgb1&0xff)*(255-x)>>8)+((rgb2&0xff)*x)>>8) |
             ((((((rgb1&0xff00)>>8)*(255-x)>>8)+(((rgb2&0xff00)>>8)*x>>8)))<<8) |
             (((((rgb1>>16)*(255-x)>>8)+((rgb2>>16)*x>>8)))<<16);
             }*/
            for ( x = 0; x < 1<<20; x++ ) y++;  // dummy loop
        }
#endif
        for ( y = 0; y < 20; y++ )
            for ( x = 0; x < 20; x++ )
                PIXEL( x, y, 0x1f );
    }
#endif

    return NO_ERROR;
}

#if 0

//----------------------------------------------------------------------------
static int vesa3_close( dev_cookie cookie )
{
    return NO_ERROR;
}

//----------------------------------------------------------------------------
static int vesa3_free_cookie( dev_cookie cookie )
{
    return NO_ERROR;
}

//----------------------------------------------------------------------------
static int vesa3_seek( dev_cookie cookie, off_t pos, seek_type st)
{
    return ERR_NOT_ALLOWED;
}

//----------------------------------------------------------------------------
static int vesa3_ioctl( dev_cookie cookie, int op, void *buf, size_t len )
{
    int err = ERR_INVALID_ARGS;

    switch ( op ) {

    case 0xfb:
        err = user_memcpy( buf, &mode_infoblock.phys_base_ptr, 4 );
        break;

    case IOCTL_DEVFS_GET_FRAMEBUFFER_INFO:
        dprintf( "vesa3: IOCTL_DEVFS_GET_FRAMEBUFFER_INFO\n" );

        if ( is_kernel_address(buf) )
            return ERR_VM_BAD_USER_MEMORY;

        if ( len == sizeof(fb_info) )
            err = user_memcpy( buf, &fb_info, sizeof(fb_info) );
        break;

    case IOCTL_DEVFS_MAP_FRAMEBUFFER:
        {
            aspace_id aid = vm_get_current_user_aspace_id();
            region_id rid;
            void *linear_video_mem;

            dprintf( "vesa3: IOCTL_DEVFS_MAP_FRAMEBUFFER\n" );

            if ( is_kernel_address(buf) )
                return ERR_VM_BAD_USER_MEMORY;

            if ( len != sizeof(linear_video_mem) )
                break;

            err = rid = vm_map_physical_memory( aid, "linear_video_mem", &linear_video_mem, REGION_ADDR_ANY_ADDRESS,
                                                mode_infoblock.x_resolution * mode_infoblock.y_resolution * ((mode_infoblock.bits_per_pixel+7)/8),
                                                LOCK_RW, (addr_t)mode_infoblock.phys_base_ptr );
            if ( err < 0 )
                break;

            err = user_memcpy( buf, &linear_video_mem, sizeof(linear_video_mem) );
            if ( err < 0 ) {
                vm_delete_region( aid, rid );
                break;
            }

            err = rid;  // return region id
        }
    }

    dprintf( "vesa3: ioctl returns 0x%x\n", err );

    return err;
}

//----------------------------------------------------------------------------
static ssize_t vesa3_read( dev_cookie cookie, void *buf, off_t pos, ssize_t len )
{
    return ERR_UNIMPLEMENTED;
}

//----------------------------------------------------------------------------
static ssize_t vesa3_write( dev_cookie cookie, const void *buf, off_t pos, ssize_t len )
{
    return ERR_UNIMPLEMENTED;
}

//----------------------------------------------------------------------------
static struct dev_calls vesa3_hooks = {
    vesa3_open,
    vesa3_close,
    vesa3_free_cookie,
    vesa3_seek,
    vesa3_ioctl,
    vesa3_read,
    vesa3_write,
    /* no paging here */
    NULL,
    NULL,
    NULL
};

//----------------------------------------------------------------------------
int dev_bootstrap(void);

#endif

errno_t vesa3_bootstrap(void)
{
    struct s_regs regs;
    struct vesa3_pm_infoblock *pm_info;
    unsigned char *ptr;
    unsigned char checksum = 0;
    unsigned int i;
    enum { BIOS_ANY, BIOS_NVIDIA, BIOS_ATI } bios_vendor = BIOS_ANY;
    int vesa_data_size = VESA_DATA_SIZE;
    //region_id id;
    //aspace_id sp = vm_get_kernel_aspace_id();
    unsigned char *video_mem;
    int ret_val = 0;

    dprintf( "vesa3: setting up VBE 3.0...\n" );

    // copy video BIOS to RAM
    video_bios = (unsigned char *)kmalloc( VESA_CODE_SIZE );
    if ( video_bios == NULL )
        return ENOMEM;

#ifdef FAKE_VESA
    memcpy( video_bios, biosdata, sizeof(biosdata) );
#else
    memcpy_p2v( video_bios, (physaddr_t)0xc0000, VESA_CODE_SIZE );

#if 0
    int n_pages = BYTES_TO_PAGES(VESA_CODE_SIZE);

    if(hal_alloc_vaddress((void**)&ptr, n_pages ))
        goto no_vesa1;

    hal_pages_control( (physaddr_t)0xc0000, ptr, n_pages, page_map_io, page_rw );

    /*
    ret_val = vm_map_physical_memory( sp, "video_bios", (void**)&ptr,
                                      REGION_ADDR_ANY_ADDRESS, VESA_CODE_SIZE, LOCK_RO | LOCK_KERNEL, (addr_t)0xc0000 );
    if ( ret_val < 0 )
        goto no_vesa1;
    */

    memcpy( video_bios, ptr, VESA_CODE_SIZE );

    //vm_delete_region( sp, ret_val );
    hal_pages_control( (physaddr_t)0xc0000, ptr, n_pages, page_unmap, page_rw );
    hal_free_vaddress( ptr, n_pages );

#endif

#endif

    //hexdump( video_bios, VESA_CODE_SIZE, 0, 0 );

    // find VESA VBE protected mode info block signature
    ptr = video_bios;
    while ( ptr <= video_bios+VESA_CODE_SIZE-sizeof(struct vesa3_pm_infoblock) && ((struct vesa3_pm_infoblock*)ptr)->signature != VESA_MAGIC )
        ptr++;

    pm_info = (struct vesa3_pm_infoblock*)ptr;

    ret_val = ENXIO;
    if ( pm_info->signature != VESA_MAGIC ) {
        dprintf( "vesa3: VBE 3.0 not available!\n" );
        goto no_vesa1;
    }

    dprintf( "vesa3: VBE 3.0 protected mode info block found...\n" );

    // calculate checksum
    for ( i = 0; i < sizeof(struct vesa3_pm_infoblock); i++ )
        checksum += *ptr++;

    if ( checksum == 0 )
        dprintf( "vesa3: checksum ok...\n" );
    else {
        // find out if we have a broken ATI BIOS
        static const char *const ati_vendor = "ATI Technologies Inc.";
        /*		static const char edid_640x480x60[18] = {
         0xd6,0x09,0x80,0xa0,0x20,0xe0,0x2d,0x10,0x10,0x60,0xa2,0x00,0x00,0x00,0x00,0x08,0x08,0x18
         };*/

        ptr = memmem( video_bios, VESA_CODE_SIZE, ati_vendor, strlen(ati_vendor) );
        if ( ptr != NULL ) {
            dprintf( "vesa3: ATI: checksum wrong, but broken ATI BIOS detected... trying to fix it...\n" );
            bios_vendor = BIOS_ATI;
            vesa3_set_mode_CRTC = vesa3_set_mode_CRTC_ATI;

            /*			// find EDID source data
             ptr = memmem( video_bios, VESA_CODE_SIZE, edid_640x480x60, sizeof(edid_640x480x60) );
             if ( ptr == NULL ) {
             dprintf( "vesa3: ATI: could not find EDID data in BIOS!\n" );
             goto no_vesa1;
             }
             dprintf( "vesa3: ATI: EDID data found at 0x%x\n", ptr-video_bios );*/

            // find EDID destination block
            i = *(uint16*)&video_bios[*(uint16*)&video_bios[*(uint16*)&video_bios[0x48]+0x34]+0x2];
            dprintf( "vesa3: ATI: EDID destination at 0x%x\n", i );
            ati_edid_dest = (struct s_edid *)&video_bios[i];

            //*(uint16*)(&video_bios[6]) = pm_info->sel_code;
            *(uint32*)(&video_bios[4]) = 0;
        }
        else {
            dprintf( "vesa3: checksum wrong!\n" );
            goto no_vesa1;
        }
    }

    // VBE 3.0 pm block found!

    // we start with the init function
    vesa3_retn = pm_info->init_offs;

#if 0
    // now analyze the bios to find out if it is a nVidia derived broken one
    if ( video_bios[vesa3_retn] == 0xe9 ) {  // jmp
        ptr = (unsigned char*)(video_bios+vesa3_retn+3) + *(uint16*)&(video_bios[vesa3_retn+1]);
        // look for "mov ds,[cs:0x6e]; call ?; mov ax,[cs:0x66]"
        if ( *(uint32*)ptr == 0x6e1e8e2e && *(uint16*)(ptr+4) == 0xe800 && *(uint32*)(ptr+8) == 0x0066a12e ) {
            // look for "o32 retn" (broken bios) or "retf" (repaired bios)
            ptr += 12;
            while ( ptr <= video_bios+0x8000-2 && *ptr != 0xcb && *(uint16*)ptr != 0xc366 )
                ptr++;
            // ok, this bios is quite certainly broken, so we patch it
            if ( *(uint16*)ptr == 0xc366 ) {
                dprintf( "vesa3: nVidia: detected broken nVidia BIOS... trying to fix it...\n" );
                bios_vendor = BIOS_NVIDIA;
                vesa3_set_mode_CRTC = vesa3_set_mode_CRTC_nVidia;
                vesa_data_size += 0x800;  // account for copy of bios video tables
                // we can't call directly so we write a stub code
                // at the end of the bios code
                ptr = video_bios+0xf000;
                dprintf( "vesa3: nVidia: creating stub code at %x...\n", (int)ptr );
                *((uint16*)ptr)++ = 0xbe66;  // mov esi,retn
                *((int32_t*)ptr)++ = (int32_t)&vesa3_retn;
                *((uint32*)ptr)++ = 0x68368b67;  // mov si,[esi]
                *((uint16*)ptr)++ = 0x0000;  // push 0
                *((uint32*)ptr)++ = 0xcb66d6ff;  // call si; o32 retf
                // the other places we need to fix is where they try to load
                // the video parameter tables at address 0x4a8
                for ( ptr = video_bios+0x5b; ptr <= video_bios+0x8000-10; ptr++ )
                    // these are either "les ?,[es:?...]"
                    if ( *((uint16*)ptr) == 0xc426 ) {
                        // sub bx,bx; mov es,bx; les bx,[es:0x4a8]; les bx,[es:bx+0x10]; les bx,[es:bx+0x16]
                        // -> sub bx,bx; mov es,bx; 13 x nop;
                        if ( *(uint32*)(ptr-4) == 0xc38edb2b
                             && *(uint32*)(ptr+1) == 0x04a81ec4
                             && *(uint32*)(ptr+5) == 0x105fc426
                             && *(uint32*)(ptr+9) == 0x165fc426 ) {
                            dprintf( "vesa3: patching %x...\n", (int)ptr );
                            for ( i = 0; i < 13; i++ ) *ptr++ = 0x90;
                        }
                        /*						else if ( *(uint32*)(ptr-4) == 0x04a83ec4 ) {
                         // les di,[0x4a8]; les di,[es:di]
                         // -> push ds; pop es; mov di,0x600; nop; nop
                         if ( *(ptr+2) == 0x3d ) {
                         dprintf( "vesa3: patching %x...\n", (int)(ptr-4) );
                         *(uint32*)(ptr-4) = 0x00bf071e;
                         *(uint16*)ptr = 0x9006;
                         *(ptr+2) = 0x90;
                         }
                         // les di,[0x4a8]; les di,[es:di+0x10] || les di,[es:di+0x4]
                         // -> xor di,di; mov es,di; 4 x nop;
                         else if ( *(uint16*)(ptr+2) == 0x107d || *(uint16*)(ptr+2) == 0x047d ) {
                         dprintf( "vesa3: patching %x...\n", (int)(ptr-4) );
                         *(uint32*)(ptr-4) = 0xc78eff31;
                         *(uint32*)ptr = 0x90909090;
                         }
                         }*/
                    }
                /*					// or "lds si,[0x4a8]; lds si,[si]"
                 // -> mov esi,0x600
                 else if ( *(uint32*)ptr == 0x04a836c5 && *(uint16*)(ptr+4) == 0x34c5 ) {
                 dprintf( "vesa3: patching %x...\n", (int)ptr );
                 *(uint32*)ptr = 0x0600be66;
                 *(uint16*)(ptr+4) = 0x0000;
                 }*/
            }
        }
    }
#endif

    // setup structure (provide selectors, map video mem, ...)
    ret_val = ENOMEM;
    bios_data = (unsigned char *)kmalloc( vesa_data_size );
    if ( bios_data == NULL )
        goto no_vesa1;
    bios_stack = (unsigned char *)kmalloc( VESA_STACK_SIZE );
    if ( bios_stack == NULL )
        goto no_vesa2;
    memset( bios_data, 0, VESA_DATA_SIZE );

#if !PH_LDT_SEL
    pm_info->sel_bios_data = i386_selector_add( SELECTOR(bios_data,vesa_data_size-1,DATA_w,false) );
    pm_info->sel_code = i386_selector_add( SELECTOR(video_bios,VESA_CODE_SIZE-1,DATA_w,false) );

    sel_code = i386_selector_add( SELECTOR(video_bios,VESA_CODE_SIZE-1,CODE_r,false) );
    sel_stack = i386_selector_add( SELECTOR(bios_stack,VESA_STACK_SIZE-1,DATA_w,false) );
#else
    assert( !get_uldt_sel( &pm_info->sel_bios_data, (linaddr_t)bios_data, vesa_data_size, 0, 0 ) );
    assert( !get_uldt_sel( &pm_info->sel_code,      (linaddr_t)video_bios, VESA_CODE_SIZE, 0, 0 ) );

    assert( !get_uldt_sel( &sel_code,  (linaddr_t)video_bios, VESA_CODE_SIZE, 1, 0 ) );
    assert( !get_uldt_sel( &sel_stack, (linaddr_t)bios_stack, VESA_STACK_SIZE, 0, 0 ) );

#endif

    if ( bios_vendor == BIOS_NVIDIA ) {
        farcall.offset = 0xf000;
        farcall.seg = sel_code;
#if 0
        // copy video tables to local memory
        ret_val = vm_map_physical_memory( sp, "video_tables", (void**)&ptr,
                                          REGION_ADDR_ANY_ADDRESS, 0x100000, LOCK_RW | LOCK_KERNEL, (addr_t)0x0 );
        if ( ret_val < 0 )
            goto no_vesa3;
        dprintf("vesa3: nVidia: (int)linear(ptr+0x4a8) = 0x%x\n", (int)linear(ptr+0x4a8) );
        memcpy( bios_data+VESA_DATA_SIZE, ptr+(int)linear(ptr+(int)linear(ptr+0x4a8)), 29*64 );  // video parameter table
        vm_delete_region( sp, ret_val );
#else
        dprintf( "vesa3: nVidia: setting up fake video parameter table...\n" );
        memcpy( bios_data+VESA_DATA_SIZE, video_param_table, 29*64 );  // VGA video parameter table
        ptr = bios_data+VESA_DATA_SIZE+29*64;  // video save pointer table
        memset( ptr, 0, 0x1c );  // make sure it is zeroed out
        *(uint16 *)ptr = VESA_DATA_SIZE;  // first entry is pointer to video parameter table
        *(uint16 *)(ptr+2) = pm_info->sel_bios_data;
        *(uint16*)(bios_data+0x4a8) = VESA_DATA_SIZE+29*64;  // 0x4a8 is pointer to video save pointer table
        *(uint16*)(bios_data+0x4a8+2) = pm_info->sel_bios_data;
        /**ptr1 = 0x68;
         *(uint16*)(ptr1+1) = pm_info->sel_code;
         *(uint32*)(ptr1+3) = 0x0798bf07;
         *(uint32*)ptr2 = 0x0798be66;
         *(uint16*)(ptr2+4) = 0x0000;*/
        /*memcpy( bios_data+0x449, video_table1, sizeof(video_table1) );
         memcpy( bios_data+0x484, video_table2, sizeof(video_table2) );
         *(uint16*)(bios_data+0x4a8) = 0x2878;
         *(uint16*)(bios_data+0x4a8+2) = pm_info->sel_code;*/
#endif
        dprintf( "vesa3: nVidia: patching done.\n" );
    }
    else {
        farcall.offset = 0;
#if PH_LDT_SEL
        selector_id tmpsel;
        assert( !get_uldt_sel( &tmpsel, (linaddr_t)vesa3_call_retf, 0x20, 1, 0 ) );
        farcall.seg = tmpsel;
#else
        farcall.seg = i386_selector_add( SELECTOR(vesa3_call_retf,0x20-1,CODE_r,false) );
#endif
        vesa3_retn |= sel_code<<16;
    }

    //ret_val = id = vm_map_physical_memory( sp, "video_mem", (void**)&video_mem, REGION_ADDR_ANY_ADDRESS, 0x20000, LOCK_RW | LOCK_KERNEL, (addr_t)0xa0000 );
    //if ( ret_val < 0 )        goto no_vesa3;

    int n_pages = BYTES_TO_PAGES(0x20000);

    if(hal_alloc_vaddress( (void**)&video_mem, n_pages ))
        goto no_vesa1;

    hal_pages_control( (physaddr_t)0xa0000, video_mem, n_pages, page_map_io, page_rw );


#if PH_LDT_SEL
    assert( !get_uldt_sel( &pm_info->sel_a0000, (linaddr_t)video_mem,         0x10000, 0, 0 ) );
    assert( !get_uldt_sel( &pm_info->sel_b0000, (linaddr_t)video_mem+0x10000,  0x8000, 0, 0 ) );
    assert( !get_uldt_sel( &pm_info->sel_b8000, (linaddr_t)video_mem+0x18000,  0x8000, 0, 0 ) );
#else
    pm_info->sel_a0000 = i386_selector_add( SELECTOR(video_mem,0x10000-1,DATA_w,false) );
    pm_info->sel_b0000 = i386_selector_add( SELECTOR(video_mem+0x10000,0x10000-1,DATA_w,false) );
    pm_info->sel_b8000 = i386_selector_add( SELECTOR(video_mem+0x18000,0x8000-1,DATA_w,false) );
#endif
    /*
     pm_info->sel_a0000 = i386_selector_add( SELECTOR(0xa0000,0x10000-1,DATA_w,false) );
     pm_info->sel_b0000 = i386_selector_add( SELECTOR(0xb0000,0x8000-1,DATA_w,false) );
     pm_info->sel_b8000 = i386_selector_add( SELECTOR(0xb8000,0x8000-1,DATA_w,false) );*/

    pm_info->in_pm = 1;

    /*	// print some debug info
     dprintf( "vesa3: video_bios: 0x%x\n", (int)video_bios );
     dprintf( "vesa3: init_offs: 0x%x\n", (int)video_bios+(int)pm_info->init_offs );
     dprintf( "vesa3: call_offs: 0x%x\n", (int)video_bios+(int)pm_info->call_offs );
     dprintf( "vesa3: vesa_call: 0x%x\n", (int)vesa_call );
     dprintf( "vesa3: vesa3_call_retf: 0x%x\n", (int)vesa3_call_retf );*/

    /*#ifdef DIAMOND_VIPER_V550_1_93E
     // patch
     ptr = (char *)((int)video_bios+0x4885);
     *ptr++ = 0xeb;  // jmp
     *ptr++ = 0x17;  // +0x17
     ptr = (char *)((int)video_bios+0x2f9d);
     *ptr++ = 0x1e;  // push ds
     *ptr++ = 0x07;  // pop es
     *ptr++ = 0xbf;  // mov di,0x600
     *ptr++ = 0x00;
     *ptr++ = 0x06;
     *ptr++ = 0x90;  // nop
     *ptr++ = 0x90;  // nop
     ptr = (char *)((int)video_bios+0x3388);
     *ptr++ = 0x1f;  // pop ds
     *ptr++ = 0xeb;  // jmp
     *ptr++ = 0x15;  // +0x15
     ptr = (char *)((int)video_bios+0x3085);
     *ptr++ = 0xeb;  // jmp
     *ptr++ = 0x41;  // +0x41
     ptr = video_bios+0x69ac;
     *(uint32*)ptr = 0x0600be66;  // mov esi,0x600
     *(uint16*)(ptr+4) = 0x0000;

     ptr = video_bios+0xf000;
     *ptr++ = 0x66;  // mov esi,vesa3_retn
     *ptr++ = 0xbe;
     *((int32*)ptr)++ = &vesa3_retn;
     *ptr++ = 0x67;  // mov si,[esi]
     *ptr++ = 0x8b;
     *ptr++ = 0x36;
     *ptr++ = 0x68;	// push 0
     *ptr++ = 0x00;
     *ptr++ = 0x00;
     *ptr++ = 0xff;  // call si
     *ptr++ = 0xd6;
     *ptr++ = 0x66;  // o32
     *ptr++ = 0xcb;  // retf
     #else
     #endif*/

    dprintf( "vesa3: calling init...\n" );

    regs.es = 0;
    vesa_call( &regs );  // init
    vesa3_retn = (vesa3_retn & 0xffff0000) | pm_info->call_offs;

    dprintf( "vesa3: reading controller information...\n" );

    bios_io_mem = calloc( 1, IO_SEL_SIZE );
    assert(bios_io_mem);
    assert( !get_uldt_sel( &bios_io_sel, kvtophys(bios_io_mem), IO_SEL_SIZE, 0, 0 ) );


    regs.ax = 0x4f00;
    regs.es = bios_io_sel;// sel_infoblock = i386_selector_add( SELECTOR(&infoblock,sizeof(infoblock)-1,DATA_w,false) );
    regs.di = 0;
    infoblock.signature = 'V'|('B'<<8)|('E'<<16)|('2'<<24);
    vesa_call( &regs );  // get VBE controller information
    //i386_selector_remove( sel_infoblock );
    memcpy( &infoblock, bios_io_mem, sizeof(infoblock) );

    dprintf( "vesa3: signature: %4s, version: 0x%x\n", (char*)&infoblock.signature, infoblock.version );

    resolve_farptr( &infoblock.oem_string_ptr );
    if ( infoblock.oem_string_ptr )
        dprintf( "vesa3: OEM string: %s\n", (char*)infoblock.oem_string_ptr );

    resolve_farptr( &infoblock.oem_vendor_name_ptr );
    if ( infoblock.oem_vendor_name_ptr )
        dprintf( "vesa3: OEM vendor name: %s\n", (char*)infoblock.oem_vendor_name_ptr );

    resolve_farptr( &infoblock.oem_product_name_ptr );
    if ( infoblock.oem_product_name_ptr )
        dprintf( "vesa3: OEM product name: %s\n", (char*)infoblock.oem_product_name_ptr );

    resolve_farptr( &infoblock.video_mode_ptr );
#if 0  // broken ATI bios
    {
        struct s_farcall farcall_bak = farcall;
        int retn_bak = vesa3_retn;

        farcall.offset = 0xf000;
        farcall.seg = sel_code;
        vesa3_retn = 0x9ba0;

        video_bios[0x9bd7] = 0x00; //0x21;

        ptr = video_bios+0xf000;
        dprintf( "vesa3: creating stub code at %x...\n", (int)ptr );
        *((uint16*)ptr)++ = 0xbe66;  // mov esi,retn
        *((int32*)ptr)++ = (int32)&vesa3_retn;
        *((uint32*)ptr)++ = 0xff368b67;  // mov si,[esi]
        *((uint32*)ptr)++ = 0xcb66d6;  // call si; o32 retf

        for ( i = 0; ; i++ ) {
            regs.bx = i<<8;
            regs.es = pm_info->sel_code;
            regs.di = 0x9b8e; //0x1f32
            vesa_call( &regs );  // init
        }

        farcall = farcall_bak;
        vesa3_retn = retn_bak;
    }
#endif

    dprintf( "vesa3: successfully initialized!\n" );

    return 0;

    //hal_pages_control( (physaddr_t)0xa0000, video_mem, n_pages, page_unmap, page_rw );
    //hal_free_vaddress( video_mem, n_pages );



//no_vesa3:
    kfree( bios_stack );
no_vesa2:
    kfree( bios_data );
no_vesa1:
    kfree( video_bios );
    dprintf( "vesa3: failed.\n" );

    return ret_val;
}
#else // ARCH_ia32
#include <errno.h>
errno_t vesa3_bootstrap(void)
{
    return ENXIO;
}
#endif // ARCH_ia32

