/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * Buses. Mostly unused.
 *
**/

#include "device.h"
#include <kernel/debug.h>
#include <kernel/init.h>

phantom_bus_t etc_bus =
{
    "Unknown Bus", PHANTOM_BUS_TYPE_UNKNOWN,
    0, 0, 0,
};


phantom_bus_t pci_bus =
{
    "PCI Bus", PHANTOM_BUS_TYPE_PCI,
    0, 0, &etc_bus,
};


phantom_bus_t isa_bus =
{
    "ISA Bus", PHANTOM_BUS_TYPE_ISA,
    0, 0, &pci_bus,
};



phantom_bus_t root_bus =
{
    "Root Bus", PHANTOM_BUS_TYPE_ROOT,
    0, &isa_bus, 0,
};


// TODO kernel debugger cmd - list devs

static void dump_dev(phantom_device_t *dev )
{
    if( dev == 0 )
        return;

    const char *bname = "?";
    if(dev->bus)
        bname = dev->bus->name;

    printf("\tDev: %s%d @ %s, io 0x%x irq %d mem %p/%d\n",
           dev->name, dev->seq_number, bname,
           dev->iobase, dev->irq,
           dev->iomem, dev->iomemsize
          );

    dump_dev( dev->next );
}

static void dump_bus(phantom_bus_t *bus, phantom_bus_t *parent )
{
    if( bus == 0 )
        return;

    const char *types = "?";

    switch( bus->type )
    {
    case PHANTOM_BUS_TYPE_UNKNOWN:
        types = "unknown??"; break;

    case PHANTOM_BUS_TYPE_ROOT:
        types = "root"; break;

    case PHANTOM_BUS_TYPE_PCI:
        types = "pci"; break;

    case PHANTOM_BUS_TYPE_ISA:
        types = "isa"; break;

    case PHANTOM_BUS_TYPE_USB:
        types = "usb"; break;

    case PHANTOM_BUS_TYPE_FW:
        types = "firewire"; break;

    }

    printf("Bus: %s%s%s (type: %s)\n",
           parent ? parent->name : "",
           parent ? " -> " : "",
           bus->name, types
          );

    dump_dev( bus->devices );

    dump_bus( bus->buses, bus );
    dump_bus( bus->next, bus );

}


static void dbg_dump_devs(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    printf("Bus/dev list:\n");

    dump_bus( &root_bus, 0 );
    dump_bus( &isa_bus, 0 );
    dump_bus( &pci_bus, 0 );

}

void init_buses(void)
{

    // add the debug command
    dbg_add_command(&dbg_dump_devs, "buses", "Dump buses and dev lists");

}

