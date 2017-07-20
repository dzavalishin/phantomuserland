/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Driver entries. TODO reimplement with constructor func which registers driver into the global map.
 *
 *
**/

#ifndef DRIVERS_H
#define DRIVERS_H

#include <kernel/bus/pci.h>
#include <device.h>


// ---------------------------------------------------------------
//                               PCI
// ---------------------------------------------------------------


phantom_device_t * driver_virtio_disk_probe( pci_cfg_t *pci, int stage );
phantom_device_t * driver_virtio_net_probe( pci_cfg_t *pci, int stage );
phantom_device_t * driver_virtio_random_probe( pci_cfg_t *pci, int stage );
phantom_device_t * driver_virtio_baloon_probe( pci_cfg_t *pci, int stage );

// USB Open Host Controller Interface
#define OHCI_BASE_CLASS     0x0c
#define OHCI_SUB_CLASS      0x03
#define OHCI_INTERFACE      0x10

#define UHCI_BASE_CLASS     0x0c
#define UHCI_SUB_CLASS      0x03
#define UHCI_INTERFACE      0x0

#define EHCI_BASE_CLASS     0x0c
#define EHCI_SUB_CLASS      0x03
#define EHCI_INTERFACE      0x20




phantom_device_t * driver_ohci_probe( pci_cfg_t *pci, int stage );
phantom_device_t * driver_uhci_probe( pci_cfg_t *pci, int stage );

/* Intel Vendor Id */
#define INTEL_VENDORID                        0x8086

/* AMD Vendor ID */
#define AMD_VENDORID                          0x1022

/* PCNet/Home Device IDs */
#define PCNET_DEVICEID                        0x2000
#define PCHOME_DEVICEID                       0x2001
phantom_device_t * driver_pcnet_pchome_probe( pci_cfg_t *pci, int stage );


#define RTL8139_VENDORID                      0x10EC
#define RTL8139_DEVICEID                      0x8139
phantom_device_t * driver_rtl_8139_probe( pci_cfg_t *pci, int stage );
phantom_device_t * driver_rtl_8169_probe( pci_cfg_t *pci, int stage );

phantom_device_t * driver_intel_82559_probe( pci_cfg_t *pci, int stage );

phantom_device_t * driver_pci_ne2000_probe( pci_cfg_t *pci, int stage );

// buses & pwer mgmt

phantom_device_t * driver_intel_1237_bridge_probe( pci_cfg_t *pci, int stage );
phantom_device_t * driver_intel_PIIX4_pm_probe( pci_cfg_t *pci, int stage );


// Sound

#define ENSONIQ_VENDORID 0x1274
phantom_device_t * driver_es1370_probe( pci_cfg_t *pci, int stage );

phantom_device_t * driver_embox_ide_probe( pci_cfg_t *pci, int stage );


// ---------------------------------------------------------------
//                               ISA
// ---------------------------------------------------------------


phantom_device_t * driver_isa_com_probe( int port, int irq, int stage );
phantom_device_t * driver_isa_lpt_probe( int port, int irq, int stage );

phantom_device_t * driver_isa_vga_probe( int port, int irq, int stage );
phantom_device_t * driver_isa_ps2m_probe( int port, int irq, int stage );
phantom_device_t * driver_isa_ps2k_probe( int port, int irq, int stage );

phantom_device_t * driver_isa_ne2000_probe( int port, int irq, int stage );

phantom_device_t * driver_isa_beep_probe( int port, int irq, int stage );

phantom_device_t * driver_isa_sb16_probe( int port, int irq, int stage );

phantom_device_t * driver_isa_floppy_probe( int port, int irq, int stage );

// ---------------------------------------------------------------
//                               Video
// ---------------------------------------------------------------

phantom_device_t * driver_intel_810_pci_probe( pci_cfg_t *pci, int stage );

phantom_device_t * driver_vmware_svga_pci_probe( pci_cfg_t *pci, int stage );

phantom_device_t * driver_parallels_svga_pci_probe( pci_cfg_t *pci, int stage );

// ---------------------------------------------------------------
//                               Others
// ---------------------------------------------------------------

phantom_device_t * driver_null_probe( const char *name, int stage );
phantom_device_t * driver_console_probe( const char *name, int stage );
phantom_device_t * driver_framebuf_probe( const char *name, int stage );

phantom_device_t * driver_etc_smbios_probe( const char *name, int stage );
phantom_device_t * driver_etc_acpi_probe( const char *name, int stage );

// Generic registers cloning SVGA driver
phantom_device_t * driver_video_gen_clone_pci_probe( pci_cfg_t *pci, int stage );



phantom_device_t * driver_ahci_probe( pci_cfg_t *pci, int stage );



#endif // DRIVERS_H
