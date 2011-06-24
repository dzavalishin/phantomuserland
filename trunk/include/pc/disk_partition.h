#ifndef DISK_PART_PC_H
#define DISK_PART_PC_H

#include <phantom_types.h>

struct pc_partition
{
    u_int8_t        	bootable;

    u_int8_t            _head;
    u_int16_t           _sec_cyl;

    u_int8_t            type;

    u_int8_t            _e_head;
    u_int16_t           _e_sec_cyl;

    u_int32_t           start;
    u_int32_t           size;

};

#endif // DISK_PART_PC_H

