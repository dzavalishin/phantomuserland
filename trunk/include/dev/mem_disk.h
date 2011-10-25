/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * In memory disk pseudo-driver.
 * Used to access mem-mapped flashes and ramdisks.
 *
**/


#include <sys/types.h>

// Passed address is virtual mem (already mapped)
#define MEMDISK_FLAG_VIRT       (1<<0)
// Memory can be written directly
#define MEMDISK_FLAG_RW         (1<<1)

errno_t driver_memdisk_init( physaddr_t disk_data, size_t disk_dat_size, int flags );

