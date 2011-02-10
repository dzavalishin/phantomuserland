#ifndef PCI_H
#define PCI_H

//#include <arch/cache.h>

#include <phantom_types.h>
#include <shorttypes.h>
#include <errno.h>

//! A structure to write to the PCI configuration register.
//! It represents a location on the PCI bus.
typedef struct confadd
{
    uint8_t reg:8;
    uint8_t func:3;
    uint8_t dev:5;
    uint8_t bus:8;
    uint8_t rsvd:7;
    uint8_t enable:1;
} confadd_t;

//! PCI device class structure. It is used to recognize to what
//! class a device belongs.
typedef const struct classes
{
    uint8_t base_class; //!< The base class code.
    uint8_t sub_class;  //!< The sub class code.
    uint8_t interface;  //!< The interface code.
    const char *name;   //!< The class name.
} classes_t;

//! This is a normal PCI device.
#define PCI_HEADER_TYPE_NORMAL          0
//! This is a bridge PCI device.
#define PCI_HEADER_TYPE_BRIDGE          1
//! This is a card-bus PCI device.
#define PCI_HEADER_TYPE_CARDBUS         2

//! Support capability list.
#define PCI_STATUS_CAP_LIST             0x10

// --- PCI registers -------------------------------------------------- //

//! PCI command register (offset).
#define PCI_COMMAND                     0x04
//! PCI status register (offset).
#define PCI_STATUS                      0x06
//! PCI interrupt line register (offset).
#define PCI_INTERRUPT_LINE              0x3C
//! PCI interrupt pin register (offset).
#define PCI_INTERRUPT_PIN               0x3D
//! PCI subsystem vendor id register (offset).
#define PCI_SUBSYSTEM_VENDOR_ID         0x2C
//! PCI subsystem id register (offset).
#define PCI_SUBSYSTEM_ID                0x2E
//! PCI latency timer register (offset).
#define PCI_LATENCY_TIMER               0x0D
//! PCI of first capability list entry (offset).
#define PCI_CAPABILITY_LIST             0x34
//! PCI of first capability list cardbus (offset).
#define PCI_CB_CAPABILITY_LIST          0x14
//! The PCI cache line size register (offset).
#define PCI_CACHE_LINE_SIZE             0x0C
//! The PCI subsystem vendor id register for cardbus (offset).
#define PCI_CB_SUBSYSTEM_VENDOR_ID      0x40
//! The PCI subsystem id register for cardbuses (offset).
#define PCI_CB_SUBSYSTEM_ID             0x42
//! The PCI ROM address register for normal devices (offset).
#define PCI_ROM_ADDRESS                 0x30
//! The PCI ROM address register for PCI to PCI bridges (offset).
#define PCI_ROM_ADDRESS_1               0x38

// --- Power Management registers ------------------------------------- //

//! Power Management capabilities register.
#define PCI_PM_PMC                      2
//! Power Management control and status register
#define PCI_PM_CTRL                     4

// --- Supported features --------------------------------------------- //

//! D1 power state support.
#define PCI_PM_CAP_D1                   0x0200
//! D2 power state support.
#define PCI_PM_CAP_D2                   0x0400

// --- Actions -------------------------------------------------------- //

//! Enable ROM address.
#define PCI_ROM_ADDRESS_ENABLE          0x01
//! The device is I/O-based (a.k.a. can perform I/O operations
//! using the ports).
#define PCI_COMMAND_IO                  0x01
//! The device is memory-based (a.k.a. can perform I/O operations
//! by a memory-mapped buffer)
#define PCI_COMMAND_MEMORY              0x02
//! Enable bus master (a.k.a. 32-bit DMA).
#define PCI_COMMAND_MASTER              0x04

// --- Resources ------------------------------------------------------ //

//! First base address register. Every PCI device has up to 6 base
//! addresses (6 for normal devices, 2 for PCI to PCI bridges and
//! only 1 for cardbuses).
#define PCI_BASE_ADDRESS_0              0x10
//! The base address is I/O-based (a.k.a. it is a port value).
#define PCI_BASE_ADDRESS_SPACE          0x01
//! The base address is memory-based (a.k.a. it is a memory
//! address value).
#define PCI_BASE_ADDRESS_SPACE_MEMORY   0x00
//! The device is memory-based (a.k.a. can perform I/O operations
//! by a memory-mapped buffer).
#define PCI_IO_RESOURCE_MEM             0x00
//! The device is I/O-based (a.k.a. can perform I/O operations
//! using the ports).
#define PCI_IO_RESOURCE_IO              0x01

// --- Capability lists ----------------------------------------------- //

//! Capability ID.
#define PCI_CAP_LIST_ID                 0
//! Power Management.
#define PCI_CAP_ID_PM                   0x01
//! Accelerated Graphics Port.
#define PCI_CAP_ID_AGP                  0x02
//! Vital Product Data.
#define PCI_CAP_ID_VPD                  0x03
//! Slot Identification.
#define PCI_CAP_ID_SLOTID               0x04
//! Message Signalled Interrupts.
#define PCI_CAP_ID_MSI                  0x05
//! CompactPCI HotSwap.
#define PCI_CAP_ID_CHSWP                0x06
//! Next capability in the list.
#define PCI_CAP_LIST_NEXT               1
//! Capability defined flags (16-bits).
#define PCI_CAP_FLAGS                   2
//! Size of PCI capability.
#define PCI_CAP_SIZEOF                  4

// --- Masks ---------------------------------------------------------- //

//! PCI ROM address mask.
#define PCI_ROM_ADDRESS_MASK            (~0x7FFUL)
//! PCI base address mask (for memory-based devices).
#define PCI_BASE_ADDRESS_MEM_MASK       (~0x0FUL )
//! PCI base address mask (for I/O-based devices).
#define PCI_BASE_ADDRESS_IO_MASK        (~0x03UL )
//! Current power state (D0 to D3).
#define PCI_PM_CTRL_STATE_MASK          ( 0x0003 )

/** \ingroup Drivers
 *  \defgroup PCIDriver Peripheral Component Interconnect (PCI) bus
 *  The PCI (Peripheral Component Interconnect) bus driver and device
 *  identification.
 *  @{
 */

//! PCI configuration structure for a device.
//! Every PCI device belong to a known class. To find this class we have
//! to look at the #base_class, #sub_class and #interface code.
/**
 *      \image html pci_config_header.jpg "The PCI configuration header"
 */
typedef struct pci_cfg
{
    // --- Normal header stuff ------------------------------------ //
    // !!! DO NOT CHANGE THIS 4 DWORD HEADER !!!                    //

    //! The vendor ID of this device.
    uint16_t vendor_id;
    //! The device ID of this device.
    uint16_t device_id;

    //! By writing to this field the system controls the device.
    uint16_t command;
    //! This field gives the status of the device with the
    //! meaning of the bits of this field set by the standard.
    uint16_t status;

    //! The revision ID of this device.
    uint8_t revision_id;
    //! The interface code of this device.
    uint8_t interface;
    //! The sub-class ID of this device.
    uint8_t sub_class;
    //! The base-class code of this device.
    uint8_t base_class;

    //! The cache line size.
    uint8_t cache_line_size;
    //! The latency timer.
    uint8_t latency_timer;
    //! The header type.
    uint8_t header_type;
    //! Build-in Self Test. If BIST is implemented,
    //! can write to a 1 to initiate BIST.
    uint8_t bist;
    // ------------------------------------------------------------ //

    // --- Device info -------------------------------------------- //
    // Below here you can add everything you want!                  //

    //! The bus number where this device resides.
    uint8_t bus;
    //! The device number where this device resides.
    uint8_t dev;
    //! The function number where this device resides.
    uint8_t func;
    //! The IRQ line (if the device has it! otherwise it is 0).
    //! Remeber that a PCI IRQ line can be shared between more
    //! than one device. This is a fetature of the PCI devices
    //! only. If more than one device shares a IRQ line the
    //! software must provide an opportune interrupt handler that
    //! recognizes the right device which causes the interrupt and
    //! switches the execution between the dedicated routines.
    uint8_t irq;

    //! Base addresses (for both I/O and memory-based devices).
    //! Every PCI device has up to 6 base addresses (6 for
    //! normal devices, 2 for PCI to PCI bridges and only 1 for
    //! cardbuses).
    uint32_t base[6];
    //! Size of the I/O space. For memory-based devices it is the
    //! size of the memory-mapped buffer; for I/O based devices it
    //! is the maximum offset of the ports used.
    uint32_t size[6];
    //! Type of the I/O operation (memory based or I/O based).
    uint8_t type[6];
    //! The ROM base address.
    uint32_t rom_base;
    //! The ROM memory space.
    uint32_t rom_size;

    //! Subsystem vendor ID.
    uint16_t subsys_vendor;
    //! Subsystem device ID.
    uint16_t subsys_device;

    //! Power management state (from D0 to D3).
    uint8_t current_state;
} pci_cfg_t;

// --- Prototypes ----------------------------------------------------- //

bool pci_find_cfg(pci_cfg_t *cfg, bool enable);
void pci_scan();

/** @} */ // end of PCIDriver

#endif


