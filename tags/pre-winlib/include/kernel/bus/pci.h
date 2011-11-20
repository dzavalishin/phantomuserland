/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * PCI bus support interfaces.
 *
 *
**/

/* 
**
** Portions copyright 1999 Brian J. Swetland. All rights reserved.
** Distributed under the terms of the OpenBLT License
*/

#ifndef _PCI_H
#define _PCI_H

#include <phantom_types.h>
#include <errno.h>

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

int		phantom_pci_find(pci_cfg_t *cfg, u_int16_t vendor_id, u_int16_t device_id);
int 		phantom_pci_find_class( pci_cfg_t *cfg, u_int8_t class_id, u_int8_t subclass_id );


u_int32_t 	phantom_pci_read(int bus, int dev, int func, int reg, int bytes);
void 		phantom_pci_write(int bus, int dev, int func, int reg, u_int32_t v, int bytes);

int 		phantom_pci_probe(int bus, int dev, int func, pci_cfg_t *cfg);

/*
 *    Enable or disable a device's memory and IO space. This must be
 *    called to enable a device's resources after setting all
 *    applicable BARs. Also enables/disables bus mastering.
 */

void 		phantom_pci_enable(pci_cfg_t *cfg, int onoff);

void		phantom_pci_dump( pci_cfg_t *pci );


char *		get_pci_vendor(int vid);
char *		get_pci_device(int vid, int did);
char *		get_pci_class(u_int8_t base, u_int8_t sub_class);

// Table to fing one of possible pci devices
typedef struct 
{
    u_int16_t vendor_id;
    u_int16_t device_id;
	
    u_int8_t base_class;
    u_int8_t sub_class;

    u_int8_t interface;

    const char *name;             	

    // Driver's internal id for this kind of dev
    int         id; 

    // Driver's internal parameters table
    void *      param;

    // Driver's internal parameters - set of four :)
    int         p0; 
    int         p1; 
    int         p2; 
    int         p3; 

} pci_table_t;

// returns table entry nimber (0-based) or -1
// cfg is input
int pci_find_in_table( pci_cfg_t *cfg,  pci_table_t *tab );

// returns table entry nimber (0-based) or -1, looks in all pci devices
// cfg is output
int pci_find_any_in_table( pci_cfg_t *cfg, pci_table_t *tab );


#define PCI_CONFIG_COMMAND 4


#endif
