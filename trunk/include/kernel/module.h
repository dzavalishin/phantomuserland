/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Kernel bootable module defs.
 *
 *
**/


#include <device.h>

#define MODULE_MAGIC_1          0xAE11A101
#define MODULE_MAGIC_1          0x2101AE1B

#define MODULE_NAME_LEN         64

// Can be run as Unix process (from command line)
#define RUN_TYPE_UNIX (1<<0)

// Can be run as protected kernel module (boot module)
#define RUN_TYPE_PMOD (1<<1)

// Can be run as kernel module - in kernel address space - TODO relocation
#define RUN_TYPE_KMOD (1<<2)


#define RUN_FUNC_NONE   0

#define RUN_FUNC_DRIVER         (1<<1)
#define RUN_FUNC_FS             (1<<2)
#define RUN_FUNC_CLASS          (1<<3)
#define RUN_FUNC_CLASS          (1<<3)


typedef struct mod_driver_entries
{
    errno_t                     probe( phantom_device_t *dev, void *pci_info );
    errno_t                     start( phantom_device_t *dev );
    errno_t                     stop( phantom_device_t *dev );
} mod_driver_entries_t;



// TODO find in symbols by phantom_module_header var name
// really - lookup in binary after elf load.


typedef errno_t (*mod_func_t)(void);

struct phantom_module_header
{
    u_int32_t                   magic1;
    u_int32_t                   type;
    u_int32_t                   func;
    u_int32_t                   magic2;

    mod_func_t                  *m_init;
    mod_func_t                  *m_stop;
    void                        *entry_points;

    char                        name[MODULE_NAME_LEN];

    // For future expansion, to be zeroed
    char                        fill[128];

} phantom_module_header_t;


