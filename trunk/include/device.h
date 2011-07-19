/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Device. Currently driver publishes just one device, but it will change.
 *
 *
**/

#ifndef _PHANTOM_DEVICE_H
#define _PHANTOM_DEVICE_H

#include <vm/object.h>
#include <errno.h>
#include <sys/types.h>


struct phantom_device;


struct phantom_dev_ops
{
    int (*start)(struct phantom_device *dev); // Start device (begin using)
    int (*stop)(struct phantom_device *dev);  // Stop device

    // Pass in objects to transfer from/to
    // Objects have to be binary or string.
    // Driver will call back (sent/rcvd)
    int (*send)(struct phantom_device *dev, pvm_object_t data);
    int (*recv)(struct phantom_device *dev, pvm_object_t data);

    // Pass in objects to transfer from/to
    // Objects have to be binary or string.
    // No call back. given thread will be awaken after object is done
    int (*sendw)(struct phantom_device *dev, pvm_object_t data, struct data_area_4_thread *tc);
    int (*recvw)(struct phantom_device *dev, pvm_object_t data, struct data_area_4_thread *tc);

    // Access from kernel - can block!
    int (*read)(struct phantom_device *dev, void *buf, int len);
    int (*write)(struct phantom_device *dev, const void *buf, int len);

    // For network devices - get MAC addr
    int (*get_address)(struct phantom_device *dev, void *buf, int len);

    // other ops :)
    errno_t (*ioctl)(struct phantom_device *dev, int type, void *buf, int len);

    // rich man's ioctl
    errno_t	(*getproperty)( struct phantom_device *dev, const char *pName, char *pValue, int vlen );
    errno_t	(*setproperty)( struct phantom_device *dev, const char *pName, const char *pValue );
    errno_t	(*listproperties)( struct phantom_device *dev, int nProperty, char *pValue, int vlen );

};

typedef struct phantom_dev_ops phantom_dev_ops_t;

struct phantom_os_ops
{
    // Driver passes back finished objects - ones that
    // was sent from or recvd to
    int (*sent)(struct phantom_device *dev, pvm_object_t data);
    int (*rcvd)(struct phantom_device *dev, pvm_object_t data);

};

typedef struct phantom_os_ops phantom_os_ops_t;







#define PHANTOM_BUS_TYPE_UNKNOWN 0
#define PHANTOM_BUS_TYPE_ROOT 1

#define PHANTOM_BUS_TYPE_PCI 2
#define PHANTOM_BUS_TYPE_ISA 3
#define PHANTOM_BUS_TYPE_USB 4
#define PHANTOM_BUS_TYPE_FW 5

struct phantom_bus
{
    const char *        	name;
    int                 	type;
    struct phantom_device *     devices; // list
    struct phantom_bus *        buses; // child buses list
    struct phantom_bus *        next; // if i'm a child - here is my next sibling

    u_int32_t			(*read32)(u_int32_t addr);
    void			(*write32)(u_int32_t addr, u_int32_t value);

    u_int16_t			(*read16)(u_int32_t addr);
    void			(*write16)(u_int32_t addr, u_int16_t value);

    u_int8_t			(*read8)(u_int32_t addr);
    void			(*write8)(u_int32_t addr, u_int8_t value);
};

typedef struct phantom_bus phantom_bus_t;

extern phantom_bus_t root_bus;
extern phantom_bus_t isa_bus;
extern phantom_bus_t pci_bus;
extern phantom_bus_t etc_bus;

struct properties;

struct phantom_device
{
    const char *                name;
    int                         seq_number; // Number of the device for the same name (driver)

    void *                      drv_private; // Driver's private part
    void *                      os_private; // Kernel's private part

    phantom_dev_ops_t           dops;
    phantom_os_ops_t            kops;

    phantom_bus_t *		bus;
    struct phantom_device *     next;

    // Main resources
    addr_t                      iobase;
    int                         irq;
    physaddr_t                  iomem;
    size_t                      iomemsize;

    struct properties           *props;

    int                         open_count; // how many times was open
};

typedef struct phantom_device phantom_device_t;

void devfs_register_dev( phantom_device_t* dev );


#define phantom_bus_add_dev( __bus, __dev ) do { (__dev)->next = (__bus)->devices; (__bus)->devices = (__dev); (__dev)->bus = (__bus); devfs_register_dev( __dev ); } while(0)
#define phantom_bus_add_bus( bus, child_bus ) do { (child_bus)->next = (bus)->buses; (bus)->buses = (child_bus); } while(0)


#endif //_PHANTOM_DEVICE_H

