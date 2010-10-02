/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * PCI type/vendor/dev tables
 *
**/

#define DEBUG_MSG_PREFIX "pci"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include "pci_devices.h"
#include <phantom_types.h>


char *get_pci_vendor(int vid)
{
// TODO bin search
    unsigned int i;
    for(i = 0; i <PCI_VENTABLE_LEN; i++)
        if(PciVenTable[i].VenId == vid)
            return PciVenTable[i].VenFull;

    return "?";
}

char *get_pci_device(int vid, int did)
{
    if( vid == 0x1AF4 && (did > 0x1000) )
        return "VirtIO device";

// TODO bin search
    unsigned int i;
    for(i = 0; i <PCI_DEVTABLE_LEN; i++)
        if(PciDevTable[i].VenId == vid && PciDevTable[i].DevId == did)
            return PciDevTable[i].ChipDesc;

    return "?";
}




char *
get_pci_class(u_int8_t base, u_int8_t sub_class)
{
	switch(base) {
		case 0:
			switch (sub_class) {
				case 0:
					return "legacy (non VGA)";
				case 1:
					return "legacy VGA";
			}
		case 0x01:
			switch (sub_class) {
				case 0:
					return "mass storage: scsi";
				case 1:
					return "mass storage: ide";
				case 2:
					return "mass storage: floppy";
				case 3:
					return "mass storage: ipi";
				case 4:
					return "mass storage: raid";
				case 5:
					return "mass storage: ata";
				case 6:
					return "mass storage: sata";
				case 0x80:
					return "mass storage: other";
			}
		case 0x02:
			switch (sub_class) {
				case 0:
					return "network: ethernet";
				case 1:
					return "network: token ring";
				case 2:
					return "network: fddi";
				case 3:
					return "network: atm";
				case 4:
					return "network: isdn";
				case 0x80:
					return "network: other";
			}
		case 0x03:
			switch (sub_class) {
				case 0:
					return "display: vga";
				case 1:
					return "display: xga";
				case 2:
					return "display: 3d";
				case 0x80:
					return "display: other";
			}
		case 0x04:
			switch (sub_class) {
				case 0:
					return "multimedia device: video";
				case 1:
					return "multimedia device: audio";
				case 2:
					return "multimedia device: telephony";
				case 0x80:
					return "multimedia device: other";
			}
		case 0x05:
			switch (sub_class) {
				case 0:
					return "memory: ram";
				case 1:
					return "memory: flash";
				case 0x80:
					return "memory: other";
			}
		case 0x06:
			switch (sub_class) {
				case 0:
					return "bridge: host bridge";
				case 1:
					return "bridge: isa";
				case 2:
					return "bridge: eisa";
				case 3:
					return "bridge: microchannel";
				case 4:
					return "bridge: PCI";
				case 5:
					return "bridge: PC Card";
				case 6:
					return "bridge: nubus";
				case 7:
					return "bridge: CardBus";
				case 8:
					return "bridge: raceway";
				case 9:
					return "bridge: stpci";
				case 10:
					return "bridge: infiniband";
				case 0x80:
					return "bridge: other";
			}
		case 0x07: return "simple comms controller";
		case 0x08: return "base system peripheral";
		case 0x09: return "input device";
		case 0x0a: return "docking station";
		case 0x0b: return "processor";
		case 0x0c:
			switch (sub_class) {
				case 0:
					return "bus: IEEE1394 FireWire";
				case 1:
					return "bus: ACCESS.bus";
				case 2:
					return "bus: SSA";
				case 3:
					return "bus: USB";
				case 4:
					return "bus: fibre channel";
			}
		case 0x0d: return "wireless";
		case 0x0e: return "intelligent i/o ??";
		case 0x0f: return "satellite";
		case 0x10: return "encryption";
		case 0x11: return "signal processing";
		default: return "unknown";
	}
}

