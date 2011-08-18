/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 PCI support
 *
**/

#define DEBUG_MSG_PREFIX "pci"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

/* $Id$
 **
 ** Copyright 1999 Brian J. Swetland. All rights reserved.
 ** Distributed under the terms of the OpenBLT License
 */

#include <ia32/pio.h>
#include <phantom_libc.h>

#include <kernel/bus/pci.h>

typedef struct
{
    u_int8_t reg:8;
    u_int8_t func:3;
    u_int8_t dev:5;
    u_int8_t bus:8;
    u_int8_t rsvd:7;
    u_int8_t enable:1;
} confadd;

u_int32_t phantom_pci_read(int bus, int dev, int func, int reg, int bytes)
{
    u_int32_t base;

    union {
        confadd c;
        u_int32_t n;
    } u;

    u.c.enable = 1;
    u.c.rsvd = 0;
    u.c.bus = bus;
    u.c.dev = dev;
    u.c.func = func;
    u.c.reg = reg & 0xFC;

    outl(0xCF8,u.n);

    base = 0xCFC + (reg & 0x03);

    switch(bytes){
    case 1: return inb(base);
    case 2: return inw(base);
    case 4: return inl(base);
    default: return 0;
    }
}

void phantom_pci_write(int bus, int dev, int func, int reg, u_int32_t v, int bytes)
{
    u_int32_t base;

    union {
        confadd c;
        u_int32_t n;
    } u;

    u.c.enable = 1;
    u.c.rsvd = 0;
    u.c.bus = bus;
    u.c.dev = dev;
    u.c.func = func;
    u.c.reg = reg & 0xFC;

    base = 0xCFC + (reg & 0x03);
    outl( 0xCF8, u.n);
    switch(bytes){
    case 1: outb(base,v); break;
    case 2: outw(base,v); break;
    case 4: outl(base,v); break;
    }

}

int phantom_pci_probe(int bus, int dev, int func, pci_cfg_t *cfg)
{
    u_int32_t *word = (u_int32_t *) cfg;
    u_int32_t v;
    int i;
    for(i=0;i<4;i++){
        word[i] = phantom_pci_read(bus,dev,func,4*i,4);
    }
    if(cfg->vendor_id == 0xffff) return 1;

    cfg->bus = bus;
    cfg->dev = dev;
    cfg->func = func;

    cfg->interrupt = -1;
#if 0
    printf("Device Info: /bus/pci/%d/%d/%d\n",bus,dev,func);
    printf("  * Vendor: %x   Device: %x  Class/SubClass/Interface %X/%X/%X\n",
           cfg->vendor_id,cfg->device_id,cfg->base_class,cfg->sub_class,cfg->interface);
    printf("  * Status: %x  Command: %x  BIST/Type/Lat/CLS: %X/%X/%X/%X\n",
           cfg->status, cfg->command, cfg->bist, cfg->header_type,
           cfg->latency_timer, cfg->cache_line_size);
#endif

    switch(cfg->header_type & 0x7F){
    case 0: /* normal device */
        for(i=0;i<6;i++)
        {
            v = phantom_pci_read(bus,dev,func,i*4 + 0x10, 4);
            if(v) {
                int v2;
                phantom_pci_write(bus,dev,func,i*4 + 0x10, 0xffffffff, 4);
                v2 = phantom_pci_read(bus,dev,func,i*4+0x10, 4) & 0xfffffff0;
                phantom_pci_write(bus,dev,func,i*4 + 0x10, v, 4);
                v2 = 1 + ~v2;
                if(v & 1) {
                    //printf("  * Base Register %d IO: %x (%x)\n",i,v&0xfff0,v2&0xffff);
                    cfg->base[i] = v & 0xfff0; // TODO check if 0xfff0 is right
                    cfg->size[i] = v2 & 0xffff;
                    cfg->is_mem[i] = 0;
                } else {
                    //printf("  * Base Register %d MM: %x (%x)\n",i,v&0xfffffff0,v2);
                    cfg->base[i] = v;
                    cfg->size[i] = v2;
                    cfg->is_mem[i] = 1;
                }
            } else {
                cfg->base[i] = 0;
                cfg->size[i] = 0;
            }

        }
        v = phantom_pci_read(bus,dev,func,0x3c,1);
        if((v != 0xff) && (v != 0))
        {
            //printf("  * Interrupt Line: %X\n",v);
            cfg->interrupt = v;
        }
        break;
    case 1:
        printf("  * PCI <-> PCI Bridge\n");

#if 0
        // This is good in total enum only
        int bridge_count = 0;

        /* if PCI-PCI bridge, increment bus count */
        if ((my_id >> 16) == 0x0604) {
            bridge_count++;
            if (bridge_count == max_pci_bus) {
                printf("PCI bridge found: PCI%d:%d:%d\n", bus, dev, func);
                max_pci_bus++;
            }
        }

#endif

        break;
    default:
        //		printf("  * Unknown Header Type\n");
        ;
    }
    return 0;
}

int phantom_pci_find( pci_cfg_t *cfg, u_int16_t vendor_id, u_int16_t device_id )
{
    int bus,dev,func;

    for(bus=0;bus<255;bus++){
        for(dev=0;dev<32;dev++) {

            if(phantom_pci_probe(bus,dev,0,cfg)) continue;

            if((cfg->vendor_id == vendor_id) &&
               (cfg->device_id == device_id))
                return 0;

            if(cfg->header_type & 0x80)
            {
                for(func=1;func<8;func++)
                {
                    if(!phantom_pci_probe(bus,dev,func,cfg) &&
                       (cfg->vendor_id == vendor_id) &&
                       (cfg->device_id == device_id))
                        return 0;
                }
            }
        }
    }

    return -1;
}


int phantom_pci_find_class( pci_cfg_t *cfg, u_int16_t class_id )
{
    int bus,dev,func;

    for(bus=0;bus<255;bus++){
        for(dev=0;dev<32;dev++) {

            if(phantom_pci_probe(bus,dev,0,cfg)) continue;

            if( cfg->base_class == class_id )
                return 0;

            if(cfg->header_type & 0x80)
            {
                for(func=1;func<8;func++)
                {
                    if(!phantom_pci_probe(bus,dev,func,cfg) &&
                       (cfg->base_class == class_id) )
                        return 0;
                }
            }
        }
    }

    return -1;
}


