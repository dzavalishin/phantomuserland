/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Block io requests queue.
 *
 *
**/

#ifndef DISK_Q_H
#define DISK_Q_H

#include <hal.h>
#include <disk.h>
#include <queue.h>

/**
 * \ingroup BlockIO
 * @{
**/

// Separate read and write queues, see #135, fails yet on second snap/reboot
#define DISK_Q_RW_SEPARATE 0

// TODO bring 'em to one place, make unique
#define DISK_Q_STRUCT_ID 0x12120001

//! Is pointed by partition 's specific field
struct disk_q
{
    unsigned                            struct_id;

    //! Used by driver to link with it's private device struct
    void                                *device;
    int                                 unit;

    hal_spinlock_t                      lock;

#if DISK_Q_RW_SEPARATE
    queue_head_t                        requests_r;
    queue_head_t                        requests_w;
#else
    queue_head_t                        requests;
#endif

    pager_io_request                    *current; // The one we do now or null

    //! Supplied by driver, called by queue code to init next io
    void        (*startIo)( struct disk_q *q );

    //! Supplied by queue code, called by driver to report io done
    void        (*ioDone)( struct disk_q *q, errno_t rc );
};

/** initialize disk_q structure. */
void phantom_init_disk_q(struct disk_q *q, void (*startIo)( struct disk_q *q ) );

phantom_disk_partition_t *phantom_create_disk_partition_struct(long size, void *private, int unit, void (*startIoFunc)( struct disk_q *q ) );

/** @} */

#endif // DISK_Q_H
