/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Disk drive properties.
 *
**/

#include <kernel/info/idisk.h>
#include <phantom_libc.h>

void dump_i_disk( i_disk_t *info )
{
    u_int64_t dsize = info->sectorSize * info->nSectors;
    dsize /= 1024*1024;

    char *type_s = "?";
    switch(info->type)
    {
    case unknown:    type_s = "unknown";	break;
    case ata:        type_s = "ata"; 	break;
    case atapi:      type_s = "atapi"; 	break;
    case cf:         type_s = "cf"; 		break;
    }

    printf("Disk type %s size %u Mb, sect %d b, multisect %d\n",
           type_s, (int)dsize, info->sectorSize, info->maxMultSectors );

    printf(" * Has: %b\n", info->has, "\020\01LBA28\02LBA48\03Trim\04DMA\05Smart\06Multisector\07Flush\08Removable" );

    printf(" * Model '%s'\n", info->model );
    printf(" * Serial '%s'\n", info->serial );

}

static void intcpy( char *to, const char *from, int nwords )
{
    while( nwords-- )
    {
        to[0] = from[1];
        to[1] = from[0];

        to += 2;
        from += 2;
    }
}


// Fields in the structure returned by the IDENTIFY DEVICE (ECh) and
// IDENTIFY PACKET DEVICE (A1h) commands (WORD offsets)
#define IDENTIFY_FIELD_VALIDITY        53
#define IDENTIFY_DMA_MODES             63
#define IDENTIFY_ADVANCED_PIO          64
#define IDENTIFY_48BIT_ADDRESSING      83
#define IDENTIFY_COMMAND_SET_SUPPORT   83
#define IDENTIFY_COMMAND_SET_ENABLED   86
#define IDENTIFY_UDMA_MODES            88


void parse_i_disk_ata( i_disk_t *info, u_int16_t p[256] )
{
    // TODO ATA only, no ATAPI/CF support

    bzero( info, sizeof(*info ) );

    info->sectorSize = 512; // Right?

    if( p[47] & 0x8000 )
    {
        info->maxMultSectors = p[47] & ~0x8000;
        info->has |= I_DISK_HAS_MULT;
    }
    else
        info->maxMultSectors = 1;

    if( (p[IDENTIFY_48BIT_ADDRESSING] >> 10) & 0x1 )
        info->has |= I_DISK_HAS_LBA48;

    if( p[169] & 0x1 )
        info->has |= I_DISK_HAS_TRIM;

    // Don't recall which is which, in fact impossible to meet disk without both of them
    if( (p[49] & 0x0300) )
        info->has |= I_DISK_HAS_LBA28|I_DISK_HAS_DMA;

    if( p[85] & 1 )
        info->has |= I_DISK_HAS_SMART;

    if( p[83] & (1 <<12) )
        info->has |= I_DISK_HAS_FLUSH;

    if( p[0] & 0x80 )
        info->has |= I_DISK_HAS_REMOV;

    if( p[0] & 0x8000 )
        info->type = ata;

    //printf("msect %d\n", p[59] & ~0x100 );

    intcpy( info->serial, ((char*)p)+10*2, I_DISK_MAX_SERIAL/2 );
    info->serial[I_DISK_MAX_SERIAL] = 0;

    intcpy( info->model, ((char*)p)+27*2, I_DISK_MAX_MODEL/2 );
    info->model[I_DISK_MAX_MODEL] = 0;

    info->nSectors = (p[61] << 16) | p[60];


}

