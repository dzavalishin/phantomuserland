/* $Id$
**
** Copyright 1999 Brian J. Swetland. All rights reserved.
** Distributed under the terms of the OpenBLT License
*/

#ifndef _PCI_H
#define _PCI_H

#include <phantom_types.h>

typedef struct 
{
	/* normal header stuff */
    u_int16_t vendor_id;
    u_int16_t device_id;
	
    u_int16_t command;
    u_int16_t status;
	
    u_int8_t revision_id;
    u_int8_t interface;
    u_int8_t sub_class;
    u_int8_t base_class;
	
    u_int8_t cache_line_size;
    u_int8_t latency_timer;
    u_int8_t header_type;
    u_int8_t bist;
	
	/* device info */
    u_int8_t bus;
    u_int8_t dev;
    u_int8_t func;
    u_int8_t _pad; // why?

	/* base registers */	
    u_int32_t base[6];
    u_int32_t size[6];
    u_int8_t is_mem[6]; // true if this is memory addr

    u_int32_t interrupt;
	
} pci_cfg_t;

int phantom_pci_find(pci_cfg_t *cfg, u_int16_t vendor_id, u_int16_t device_id);
int phantom_pci_find_class( pci_cfg_t *cfg, u_int8_t class_id, u_int8_t subclass_id );


u_int32_t phantom_pci_read(int bus, int dev, int func, int reg, int bytes);
void phantom_pci_write(int bus, int dev, int func, int reg, u_int32_t v, int bytes);

int phantom_pci_probe(int bus, int dev, int func, pci_cfg_t *cfg);


char *get_pci_vendor(int vid);
char *get_pci_device(int vid, int did);
char *get_pci_class(u_int8_t base, u_int8_t sub_class);


#endif
