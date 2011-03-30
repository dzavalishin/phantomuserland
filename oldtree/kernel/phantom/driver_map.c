/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Drivers probe/map.
 *
**/

#define DEBUG_MSG_PREFIX "driver"
#include "debug_ext.h"
#define debug_level_flow 1
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/config.h>
#include <kernel/init.h>
#include <kernel/board.h>
#include <kernel/drivers.h>

#include <phantom_libc.h>

#include <i386/pci.h>

#include <device.h>

#include "misc.h"

#include <virtio_blk.h>
#include <virtio_net.h>
#include <virtio_rng.h>

#include <kernel/debug.h>


static void add_alldevs( phantom_device_t *dev );
static void dump_alldevs( int ac, char **av );




#define SPECIAL_VIRTIO_MAP 0

// very simple drivers to devices mapping structure

typedef struct
{
    const char *name;
    phantom_device_t * (*probe_f)( pci_cfg_t *pci, int stage );        // Driver probe func
    int minstage; // How early we can be called
    int vendor; // PCI Vendor or 0 to ignore (must have device == 0 too)
    int device; // PCI device ID or 0 to ignore
    int dclass; // PCI device class or 0 to ignore (must have dev == 0)

} pci_probe_t;

#define VIRTIO_VENDOR 0x1AF4


#if HAVE_PCI

// NB! No network drivers on stage 0!
static pci_probe_t pci_drivers[] =
{
#if 1
    { "VirtIO Disk", 	driver_virtio_disk_probe, 	2, VIRTIO_VENDOR, 0, 1 },
    { "VirtIO Baloon", 	driver_virtio_baloon_probe, 	2, VIRTIO_VENDOR, 0, 5 },
    //{ "VirtIO Random",  driver_virtio_random_probe, 	2, VIRTIO_VENDOR, 0, 1 }, // TODO dev/dclass?
#endif

#if 0 && HAVE_NET
    { "VirtIO Net",  	driver_virtio_net_probe, 	2, VIRTIO_VENDOR, 0x1000, 0 }, // TODO dev/dclass?
    { "AMD PcNet",   	driver_pcnet_pchome_probe, 	1, AMD_VENDORID, PCNET_DEVICEID, 0 },
//    { "AMD PcHome",  	driver_pcnet_pchome_probe, 	1, AMD_VENDORID, PCHOME_DEVICEID, 0 },
//    { "RTL 8139", 	driver_rtl_8139_probe, 		1, RTL8139_VENDORID, RTL8139_DEVICEID, 0 },
//    { "Intel i82559er", driver_intel_82559_probe, 	3, INTEL_VENDORID, 0x1209, 0 },
    { "Ne2000 PCI", 	driver_pci_ne2000_probe, 	1, 0x10ec, 0x8029, 0 },

    { "RTL 8169", 	driver_rtl_8169_probe, 		1, RTL8139_VENDORID, 0x8169, 0 },
    { "RTL 8129", 	driver_rtl_8169_probe, 		1, RTL8139_VENDORID, 0x8129, 0 },
    { "RTL 8168", 	driver_rtl_8169_probe, 		1, RTL8139_VENDORID, 0x8168, 0 }, // TODO problems possible

#endif // HAVE_NET

#if COMPILE_OHCI
    { "USB OHCI",       driver_ohci_probe, 		3, 0, 0, OHCI_BASE_CLASS },
#endif

#if COMPILE_UHCI
    { "USB UHCI",       driver_uhci_probe, 		3, 0, 0, UHCI_BASE_CLASS },
#endif

    // Chipset drivers, etc

    // Do not work, seem to be uninited by BIOS - no IO port defined
    //{ "PIC/Mem 1237", 	driver_intel_1237_bridge_probe,	3, INTEL_VENDORID, 0x1237, 0 },
    //{ "PIIX4 PM", 	driver_intel_PIIX4_pm_probe,	3, INTEL_VENDORID, 0x7113, 0 },

//    { "es1370", 	driver_es1370_probe,		2, ENSONIQ_VENDORID, 0x5000, 0 },
    //{ "es1370", 	driver_es1370_probe,		2, 0x1274, 0x5000, 0 },
    
};



#endif // HAVE_PCI



static isa_probe_t *board_drivers = 0;

// NB! No network drivers on stage 0!
static isa_probe_t isa_drivers[] =
{

    // End of list marker
    { 0, 0, 0, 0, 0 },
};



typedef struct
{
    const char *name;
    phantom_device_t * (*probe_f)( const char *name, int stage );        // Driver probe func
    int minstage; // How early we can be called

} etc_probe_t;

// NB! No network drivers on stage 0!
static etc_probe_t etc_drivers[] =
{
    { "console",                driver_console_probe,    	0 },
    { "null",                   driver_null_probe,      	0 },
    { "fb",                     driver_framebuf_probe,    	0 },

#if defined(ARCH_ia32)
    { "SMBios", 		driver_etc_smbios_probe, 	0 },
//    { "ACPI", 			driver_etc_acpi_probe, 		0 },
#endif
};




#if HAVE_PCI

typedef struct
{
    pci_cfg_t   pci;
    int 	filled; // not empty rec
    int 	used; // already found a driver
} pci_record_t;




static void show_pci( pci_cfg_t* pci )
{
    printf("PCI %d.%d: %s (%s) %X:%X class %X:%X (%s) if %X\n",
           //pci->bus,
           pci->dev, pci->func,
           get_pci_device( pci->vendor_id, pci->device_id ),
           get_pci_vendor(pci->vendor_id), 
           pci->vendor_id, pci->device_id,
           pci->base_class, pci->sub_class, get_pci_class(pci->base_class, pci->sub_class),
           pci->interface
          );

}



#define MAXPCI 512
static pci_record_t allpci[MAXPCI];
static int load_once = 0;

static int loadpci()
{
    int bus,dev,func;
    int pci_slot = 0;

    if(load_once) return 0;
    load_once = 1;




    SHOW_FLOW0( 1, "Loading PCI config:");

    // TODO we need to count buses ourself and
    // not to check all the world each time
    for(bus=0;bus<255;bus++){
        for(dev=0;dev<32;dev++) {

            if(pci_slot >= MAXPCI)
            {
                SHOW_ERROR0( 1, "Warning: too much PCI devs, some are missed");
                return 0;
            }

            if(phantom_pci_probe(bus,dev,0, &allpci[pci_slot].pci ))
                continue;

            show_pci(&allpci[pci_slot].pci);

            allpci[pci_slot].used = 0;
            allpci[pci_slot].filled = 1;

            int is_multifunc = allpci[pci_slot].pci.header_type & 0x80;

            pci_slot++;

            if(is_multifunc)
            {
                for( func = 1; func < 8; func++ )
                {
                    if(!phantom_pci_probe(bus,dev,func, &allpci[pci_slot].pci ) )
                    {
                        show_pci(&allpci[pci_slot].pci);

                        allpci[pci_slot].used = 0;
                        allpci[pci_slot].filled = 1;
                        pci_slot++;

                    }
                }
            }
        }
    }

    return 0;
}


static int probe_pci( int stage, pci_cfg_t *pci )
{

    SHOW_FLOW( 2, "%d PCI check vend %X dev %X cl %X", stage, pci->vendor_id, pci->device_id, pci->base_class );

    unsigned int i;
    for( i = 0; i < sizeof(pci_drivers)/sizeof(pci_probe_t); i++ )
    {
        pci_probe_t *dp = &pci_drivers[i];
        SHOW_FLOW( 3, "-- against %X : %X cl %X stg %d", dp->vendor, dp->device, dp->dclass, dp->minstage );
        if( stage < dp->minstage )
            continue;
        if( dp->device && pci->device_id != dp->device )
            continue;
        if( dp->vendor && pci->vendor_id != dp->vendor )
            continue;
        if( dp->dclass && pci->base_class != dp->dclass )
            continue;

        SHOW_FLOW( 1, "Probing driver '%s' for PCI dev %d:%d (%s:%s %X:%X class %X:%X)",
                   dp->name, pci->dev, pci->func,
                   get_pci_vendor(pci->vendor_id), get_pci_device( pci->vendor_id, pci->device_id ),
                   pci->vendor_id, pci->device_id,
                   pci->base_class, pci->sub_class
                  );
        phantom_device_t *dev = dp->probe_f( pci, stage );

        if( dev != 0 )
        {
            SHOW_INFO( 0, "Driver '%s' attached to PCI dev %d:%d (%s:%s)",
                   dp->name, pci->dev, pci->func,
                   get_pci_vendor(pci->vendor_id), get_pci_device( pci->vendor_id, pci->device_id )
                  );

            phantom_bus_add_dev( &pci_bus, dev );
            add_alldevs( dev );

            return 1;
        }
        else
            SHOW_ERROR0( 1, "Failed");
    }

    return 0;
}


#endif // HAVE_PCI




static int probe_isa( int stage, isa_probe_t *idrivers )
{
    unsigned int i;
    for( i = 0; idrivers[i].probe_f != 0; i++ )
    {
        isa_probe_t *dp = idrivers+i;

        SHOW_FLOW( 2, "check %s stage %d minstage %d", dp->name, stage, dp->minstage );
        if( stage < dp->minstage )
            continue;

        SHOW_FLOW( 2, "probe %s @ 0x%x", dp->name, dp->port );

        phantom_device_t *dev = dp->probe_f( dp->port, dp->irq, stage );

        if( dev != 0 )
        {
            SHOW_INFO( 0, "Driver '%s' attached to ISA at 0x%X (IRQ %d)",
                       dp->name, dp->port, dp->irq );
            phantom_bus_add_dev( &isa_bus, dev );
            add_alldevs( dev );
        }
        dp->minstage = 1000; // prevent it from being processed next time
    }

    return 0;
}



static int probe_etc( int stage )
{
    unsigned int i;
    for( i = 0; i < sizeof(etc_drivers)/sizeof(etc_probe_t); i++ )
    {
        etc_probe_t *dp = &etc_drivers[i];

        if( stage < dp->minstage )
            continue;

        phantom_device_t *dev = dp->probe_f( dp->name, stage );

        if( dev != 0 )
        {
            SHOW_INFO( 0, "Driver '%s' attached", dp->name );
            phantom_bus_add_dev( &etc_bus, dev );
            add_alldevs( dev );
            dp->minstage = ~0; // prevent it from being processed next time
        }
    }

    return 0;
}


// Stage is:
//   0 - very early in the boot - interrupts can be used only
//   1 - boot, most of kernel infrastructure is there
//   2 - disks which Phantom will live in must be found here
//   3 - late and optional and slow junk

int phantom_find_drivers( int stage )
{
    if( stage == 0 )
    {
#if HAVE_PCI
        if(loadpci())
            SHOW_ERROR0( 0, "Can not load PCI devices table" );
#endif
        // Get driver mappoings from board specific code
        board_make_driver_map();
    }

    if(stage == 3)
    {
        // add the debug command
        dbg_add_command(&dump_alldevs, "devs", "List all of the device drivers");
    }

#if HAVE_PCI
    int i;

    SHOW_FLOW( 0, "Look for PCI devices, stage %d", stage );
    for(i = 0; i <= MAXPCI; i++ )
    {
        if( (!allpci[i].filled) || allpci[i].used )
            continue;

        if( probe_pci(stage, &allpci[i].pci ) )
            allpci[i].used = 1;
    }
    SHOW_FLOW( 2, "Finished looking for PCI devices, stage %d", stage );


#if SPECIAL_VIRTIO_MAP
    printf("Look for VirtIO PCI devices, stage %d\n", stage );
    for(i = 0; i <= MAXPCI; i++ )
    {
        if( !(virtio_pci[i].filled) || virtio_pci[i].used )
            continue;

        if( probe_virtio(stage, &allpci[i].pci ) )
            virtio_pci[i].used = 1;
    }
//getchar();
    printf("Finished looking for VirtIO PCI devices, stage %d", stage );
#endif
#endif // HAVE_PCI

    SHOW_FLOW( 2, "Start looking for ISA devices, stage %d", stage );
    probe_isa( stage, isa_drivers );
    SHOW_FLOW( 2, "Finished looking for ISA devices, stage %d", stage );

    if(board_drivers)
    {
        SHOW_FLOW( 2, "Start looking for board devices, stage %d", stage );
        probe_isa( stage, board_drivers );
        SHOW_FLOW( 2, "Finished looking for board devices, stage %d", stage );
    }
    //getchar();
    probe_etc( stage );
    SHOW_FLOW( 2, "Finished looking for other devices, stage %d", stage );

    return -1;
}



void phantom_register_drivers(isa_probe_t *drivers )
{
     board_drivers = drivers;
}




#define MAXALLDEV 256

static phantom_device_t *alldevs[MAXALLDEV];
static int nalldevs = 0;


static void add_alldevs( phantom_device_t *dev )
{
    if(nalldevs >= MAXALLDEV)
    {
        SHOW_ERROR0( 0, "Too many devices!");
        return;
    }

    alldevs[nalldevs++] = dev;

}


static void dump_alldevs( int ac, char **av )
{
    (void) ac;
    (void) av;

    int i;
    for( i = 0; i < nalldevs; i++ )
        printf( "%s,\t", alldevs[i]->name );
    printf("\n");
}

