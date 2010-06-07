#include <phantom_libc.h>
#include <i386/vesa.h>
#include <hal.h>
#include <video.h>
#include "vm86.h"

#include "../misc.h"

#define VBE_RET ( (tss_vm86.tss.eax & 0xFFFFu) != 0x004F ? 0 : (tss_vm86.tss.eax & 0xFFFFu) )


void setTextVideoMode()
{
    tss_vm86.tss.eax = 0x0003;
    phantom_bios_int_10();
}





int getVesaControllerInfo( struct VBEInfoBlock *info )
{
    memset( vm86_setup.data, 0, sizeof( struct VBEInfoBlock ) );
    strcpy( vm86_setup.data, "VBE2" ); // Ask for more

    // es:di -> result
    tss_vm86.tss.es = ((int)vm86_setup.data) >> 4;
    tss_vm86.tss.edi = 0; // start of ds

    tss_vm86.tss.eax =0x4F00;

    phantom_bios_int_10();

    memmove( info, vm86_setup.data, sizeof( struct VBEInfoBlock ) );
    return VBE_RET;
}




int getVesaModeInfo( u_int16_t mode, struct VBEModeInfoBlock *info )
{
    memset( vm86_setup.data, 0, sizeof( struct VBEModeInfoBlock ) );

    tss_vm86.tss.ecx = mode & 0x01ff;

    // es:di -> result
    tss_vm86.tss.es = ((int)vm86_setup.data) >> 4;
    tss_vm86.tss.edi = 0; // start of ds

    tss_vm86.tss.eax =0x4f01;

    phantom_bios_int_10();

    memmove( info, vm86_setup.data, sizeof( struct VBEModeInfoBlock ) );
    return VBE_RET;
}

int setVesaMode( u_int16_t mode )
{
    tss_vm86.tss.ebx = mode; // | 0x4000;
    tss_vm86.tss.eax = 0x4f02;

    phantom_bios_int_10();
    return VBE_RET;
}



#define MAX_W 1024


#define farTo32(fp) ( (void *) (((fp & 0xFFFF0000u) >> 12) | (fp & 0xFFFFu) ) )


void phantom_init_vesa(void)
{

    struct VBEInfoBlock * vi;
    int rc;

    struct VBEInfoBlock vi_buf;
    printf("\nLooking for VESA VBE... " );
    rc = getVesaControllerInfo( &vi_buf );
    if( rc )
    {
        printf("failed, rc = %X\n", rc );
        return;
    }
    vi = &vi_buf;
// for some reason QEMU switches videomode in this point
//video_drv_basic_vga_set_text_mode();
//setTextVideoMode();

    char *oem_name = farTo32(vi->oem_ptr);
    printf("VESA ver %X ptr %X, OEM '%s'\n", vi->version, vi->video_ptr, oem_name );

    printf("Total %dK mem, cap: %b\n", 64*vi->total_memory/1024, vi->capabilities, "\020\01DAC 8bit\02not VGA compatible\03RAMDAC blank\04STEREO\05STEREO EVC" );
//getchar();
    //struct VBEModeInfoBlock *vib = farTo32(vi->video_ptr);

    u_int16_t best_mode = -1;
    int best_width = -1;
    struct VBEModeInfoBlock best_info;

    u_int16_t modes_buf[256];
    // get them out of VM86 mem
    memmove( modes_buf, farTo32(vi->video_ptr), sizeof(modes_buf) );
    u_int16_t *modes = modes_buf;


    printf("Lookup VESA modes:\n");
    for(; *modes != 0xFFFF; modes++ )
    {
        u_int16_t mode = *modes;

        struct VBEModeInfoBlock info;

        getVesaModeInfo( mode, &info );

        // is off
        if(! (info.attributes & 0x01 ))
            continue;

        {
            const char *map = "\020\1OK\2RSV\3TTY\4COLR\5GFX\6~VGA\7~WIN\10LINBUF\11DSCAN\012INTRLACE\013TRIBUF\014STEREO\015DUALSTART";
            printf("%02X, %4d*%4d/%2d, mem %X attr=%b: ",
                   mode, 
                   info.x_resolution, info.y_resolution, info.bits_per_pixel,
                   info.phys_base_ptr,
                   info.attributes, map
                  );
        }
        if(! (info.attributes & 0x08 ))
        {
            printf("not color\n");
            continue;
        }

        if(! (info.attributes & 0x80 ))
        {
            printf("not linear\n");
            continue;
        }

        if(! (info.attributes & 0x10 ))
        {
            printf("not graphics\n");
            continue;
        }

        if( info.bits_per_pixel != 24 )
        {
            printf("not 24bpp\n");
            continue;
        }

        if( info.memory_model != 6 )
        {
            printf("not direct color mem model (%d)\n", info.memory_model );
            continue;
        }

        if( info.num_planes > 1 )
        {
            printf("too much planes\n");
            continue;
        }

        if( info.x_resolution > best_width && info.x_resolution <= MAX_W )
        {
            best_width = info.x_resolution;
            best_mode = mode;
            best_info = info;
            printf("better\n");
        }
        else
            printf("worse\n");

    }

    if( best_width < 0 )
    {
        printf("No suitable VESA mode found\n");
        return;
    }

    //u_int32_t physVidmemPtr = best_info.phys_base_ptr;

    printf("VESA mode %d %d*%d %dbpp selected, mem at %X\n",
           best_mode,
           best_info.x_resolution, best_info.y_resolution,
           best_info.bits_per_pixel, best_info.phys_base_ptr
          );


    video_driver_bios_vesa.xsize = best_info.x_resolution;
    video_driver_bios_vesa.ysize = best_info.y_resolution;

    int v_memsize = 64*1024*vi->total_memory;

    set_video_driver_bios_vesa_pa( best_info.phys_base_ptr, v_memsize );
    set_video_driver_bios_vesa_mode( best_mode | VBE_MODE_LINEAR );


#if VESA_ENFORCE
    printf("Setting VESA video mode %d\n", best_mode );
    if( setVesaMode( best_mode | VBE_MODE_LINEAR ) )
    {
        // TODO reset VGA text mode here!
        printf("Failed to set VESA video mode %d, VESA is assumed to be dead\n", best_mode );
        return;
    }

    phantom_enforce_video_driver(&video_driver_bios_vesa);
#endif
}


