
#if defined(ARCH_ia32)

#define DEBUG_MSG_PREFIX "PIIX4"
#include "debug_ext.h"
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <phantom_libc.h>
#include <i386/pio.h>
#include "driver_map.h"

#include "intel_piix4_regs.h"



#if 0
#  define WW() getchar()
#else
#  define WW()
#endif

#define DEV_NAME "PIIX4 PM"


static int intel_piix4_pm_read(struct phantom_device *dev, void *buf, int len);
static int intel_piix4_pm_write(struct phantom_device *dev, const void *buf, int len);

static errno_t check_piix4_pm_sanity(int port);
static errno_t check_piix4_isa_sanity(int port);





static int seq_number = 0;

phantom_device_t * driver_intel_PIIX4_pm_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    int io_port = 0;
    int mem_base = 0;
    size_t mem_size = 0;

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            mem_base = (pci->base[i]);
            mem_size = pci->size[i];
            SHOW_INFO( 1, "mem base 0x%lx, size 0x%lx", mem_base, mem_size);
        } else if( pci->base[i] > 0) {
            io_port = pci->base[i];
            SHOW_INFO( 1, "io_port 0x%x", io_port);
        }
    }

    SHOW_FLOW( 1, "Look for " DEV_NAME " at io %X int %d", io_port, pci->interrupt );

    // Gets port 0. uninited by BIOS? Need explicit PCI io addr assign?
    if( io_port == 0 )
    {
        SHOW_ERROR0( 0, "No io port?" );
        return 0;
    }

    if( check_piix4_pm_sanity(io_port) )
        return 0;


    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = DEV_NAME " power management unit";
    dev->seq_number = seq_number++;
    dev->drv_private = 0;

    dev->dops.read = intel_piix4_pm_read;
    dev->dops.write = intel_piix4_pm_write;

    dev->irq = pci->interrupt;

    return dev;

}








static int intel_piix4_pm_read(struct phantom_device *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    return -1;
}

static int intel_piix4_pm_write(struct phantom_device *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    return -1;
}






static errno_t check_piix4_pm_sanity(int port)
{
    int pmen = inl( port + PII4_PM_PMEN );
    SHOW_INFO( 1, "PMEN %b", pmen, "\020\1TMROF_EN\6GLB_EN\10PWRBTN_EN\13RTC_EN" );

    int pmtmr = inl( port + PII4_PM_PMTMR );
    SHOW_INFO( 1, "PMTMR %d", pmtmr );

    if( pmtmr == 0 )
    {
        SHOW_ERROR0( 0, "PMTMR is zero" );
        return ENXIO;
    }

    return 0;
}










#undef DEV_NAME
#define DEV_NAME "PIIX4 ISA Bridge"


phantom_device_t * driver_intel_1237_bridge_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    int io_port = 0;
    int mem_base = 0;
    size_t mem_size = 0;

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            mem_base = (pci->base[i]);
            mem_size = pci->size[i];
            SHOW_INFO( 1, "mem base 0x%lx, size 0x%lx", mem_base, mem_size);
        } else if( pci->base[i] > 0) {
            io_port = pci->base[i];
            SHOW_INFO( 1, "io_port 0x%x", io_port);
        }
    }

    //SHOW_FLOW( 1, "Look for " DEV_NAME " at io %X int %d", io_port, pci->interrupt );
    SHOW_FLOW( 1, "Look for " DEV_NAME " at io %X", io_port );

    // Gets port 0. uninited by BIOS? Need explicit PCI io addr assign?
    if( io_port == 0 )
    {
        SHOW_ERROR0( 0, "No io port?" );
        return 0;
    }

    if( check_piix4_isa_sanity(io_port) )
        return 0;


    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = DEV_NAME;
    dev->seq_number = seq_number++;
    dev->drv_private = 0;

    //dev->dops.read = intel_piix4_pm_read;
    //dev->dops.write = intel_piix4_pm_write;

    // No
    //dev->irq = pci->interrupt;

    return dev;

}



static errno_t check_piix4_isa_sanity(int port)
{
	(void) port;
    return 0;
}

#endif // defined(ARCH_ia32)
