// code to access floppy drives.
//
// Copyright (C) 2008,2009  Kevin O'Connor <kevin@koconnor.net>
// Copyright (C) 2002  MandrakeSoft S.A.
//
// This file may be distributed under the terms of the GNU LGPLv3 license.
//
// Converted for Phantom by dz (C) 2011
//

#define DEBUG_MSG_PREFIX "floppy"
#define debug_level_flow 9
#define debug_level_error 10
#define debug_level_info 10

#include <i386/pio.h>
#include <device.h>
#include <kernel/ia32/rtc.h>
#include <compat/seabios.h>



/*
#include "types.h" // u8
#include "disk.h" // DISK_RET_SUCCESS
#include "config.h" // CONFIG_FLOPPY
#include "biosvar.h" // SET_BDA
#include "util.h" // wait_irq
#include "cmos.h" // inb_cmos
#include "pic.h" // eoi_pic1
#include "bregs.h" // struct bregs
#include "boot.h" // boot_add_floppy
#include "pci.h" // pci_to_bdf
#include "pci_ids.h" // PCI_CLASS_BRIDGE_ISA
*/

#define CMOS_FLOPPY_DRIVE_TYPE   0x10


#define FLOPPY_SIZE_CODE 0x02 // 512 byte sectors
#define FLOPPY_DATALEN 0xff   // Not used - because size code is 0x02
#define FLOPPY_MOTOR_TICKS 37 // ~2 seconds
#define FLOPPY_FILLBYTE 0xf6
#define FLOPPY_GAPLEN 0x1B
#define FLOPPY_FORMAT_GAPLEN 0x6c

#define NFLOPPY 2


static u8 floppy_last_data_rate;
static u8 floppy_motor_counter;
static u8 floppy_recalibration_status;
static u8 floppy_return_status[8];

static u8 floppy_track[NFLOPPY];
static u8 floppy_media_state[NFLOPPY];


static struct drive_s *drive[NFLOPPY];


// Floppy "Disk Base Table"
struct floppy_dbt_s {
    u8 specify1;
    u8 specify2;
    u8 shutoff_ticks;
    u8 bps_code;
    u8 sectors;
    u8 interblock_len;
    u8 data_len;
    u8 gap_len;
    u8 fill_byte;
    u8 settle_time;
    u8 startup_time;
} PACKED;

struct floppy_ext_dbt_s {
    struct floppy_dbt_s dbt;
    // Extra fields
    u8 max_track;
    u8 data_rate;
    u8 drive_type;
} PACKED;



// New diskette parameter table adding 3 parameters from IBM
// Since no provisions are made for multiple drive types, most
// values in this table are ignored.  I set parameters for 1.44M
// floppy here
struct floppy_ext_dbt_s diskette_param_table2 VAR16VISIBLE = {
    .dbt = {
        .specify1       = 0xAF, // step rate 12ms, head unload 240ms
        .specify2       = 0x02, // head load time 4ms, DMA used
        .shutoff_ticks  = FLOPPY_MOTOR_TICKS, // ~2 seconds
        .bps_code       = FLOPPY_SIZE_CODE,
        .sectors        = 18,
        .interblock_len = FLOPPY_GAPLEN,
        .data_len       = FLOPPY_DATALEN,
        .gap_len        = FLOPPY_FORMAT_GAPLEN,
        .fill_byte      = FLOPPY_FILLBYTE,
        .settle_time    = 0x0F, // 15ms
        .startup_time   = 0x08, // 1 second
    },
    .max_track      = 79,   // maximum track
    .data_rate      = 0,    // data transfer rate
    .drive_type     = 4,    // drive type in cmos
};

// Since no provisions are made for multiple drive types, most
// values in this table are ignored.  I set parameters for 1.44M
// floppy here
struct floppy_dbt_s diskette_param_table
VAR16FIXED(0xefc7)
= {
    .specify1       = 0xAF,
    .specify2       = 0x02,
    .shutoff_ticks  = FLOPPY_MOTOR_TICKS,
    .bps_code       = FLOPPY_SIZE_CODE,
    .sectors        = 18,
    .interblock_len = FLOPPY_GAPLEN,
    .data_len       = FLOPPY_DATALEN,
    .gap_len        = FLOPPY_FORMAT_GAPLEN,
    .fill_byte      = FLOPPY_FILLBYTE,
    .settle_time    = 0x0F,
    .startup_time   = 0x08,
};

struct floppyinfo_s {
    struct chs_s chs;
    u8 config_data;
    u8 media_state;
};

struct floppyinfo_s FloppyInfo[] VAR16VISIBLE = {
    // Unknown
    { {0, 0, 0}, 0x00, 0x00},
    // 1 - 360KB, 5.25" - 2 heads, 40 tracks, 9 sectors
    { {2, 40, 9}, 0x00, 0x25},
    // 2 - 1.2MB, 5.25" - 2 heads, 80 tracks, 15 sectors
    { {2, 80, 15}, 0x00, 0x25},
    // 3 - 720KB, 3.5"  - 2 heads, 80 tracks, 9 sectors
    { {2, 80, 9}, 0x00, 0x17},
    // 4 - 1.44MB, 3.5" - 2 heads, 80 tracks, 18 sectors
    { {2, 80, 18}, 0x00, 0x17},
    // 5 - 2.88MB, 3.5" - 2 heads, 80 tracks, 36 sectors
    { {2, 80, 36}, 0xCC, 0xD7},
    // 6 - 160k, 5.25"  - 1 heads, 40 tracks, 8 sectors
    { {1, 40, 8}, 0x00, 0x27},
    // 7 - 180k, 5.25"  - 1 heads, 40 tracks, 9 sectors
    { {1, 40, 9}, 0x00, 0x27},
    // 8 - 320k, 5.25"  - 2 heads, 40 tracks, 8 sectors
    { {2, 40, 8}, 0x00, 0x27},
};

struct drive_s *
init_floppy(int floppyid, unsigned int ftype)
{
    SHOW_FLOW( 0, "Init floppy %d, type %d", floppyid, ftype );
    if (ftype <= 0 || ftype >= ARRAY_SIZE(FloppyInfo)) {
        dprintf(1, "Bad floppy type %d\n", ftype);
        return NULL;
    }

    struct drive_s *drive_g = malloc_fseg(sizeof(*drive_g));
    if (!drive_g) {
        warn_noalloc();
        return NULL;
    }
    memset(drive_g, 0, sizeof(*drive_g));
    drive_g->cntl_id = floppyid;
    drive_g->type = DTYPE_FLOPPY;
    drive_g->blksize = DISK_SECTOR_SIZE;
    drive_g->floppy_type = ftype;
    drive_g->sectors = (u64)-1;

    memcpy(&drive_g->lchs, &FloppyInfo[ftype].chs
           , sizeof(FloppyInfo[ftype].chs));
    return drive_g;
}

static void
addFloppy(int floppyid, int ftype)
{
    assert(floppyid < NFLOPPY);

    struct drive_s *drive_g = init_floppy(floppyid, ftype);
    if (!drive_g)
        return;

    drive[floppyid] = drive_g;

    //char *desc = znprintf(MAXDESCSIZE, "Floppy [drive %c]", 'A' + floppyid);
    //int bdf = pci_find_class(PCI_CLASS_BRIDGE_ISA); /* isa-to-pci bridge */
    //int prio = bootprio_find_fdc_device(bdf, PORT_FD_BASE, floppyid);
    //boot_add_floppy(drive_g, desc, prio);
}


static void floppy_interrupt(void *arg);

#if 0
// Find a floppy type that matches a given image size.
int
find_floppy_type(u32 size)
{
    unsigned int i;
    for (i=1; i<ARRAY_SIZE(FloppyInfo); i++) {
        struct chs_s *c = &FloppyInfo[i].chs;
        if (c->cylinders * c->heads * c->spt * DISK_SECTOR_SIZE == (int32_t)size)
            return i;
    }
    return -1;
}
#endif

/****************************************************************
 * Low-level floppy IO
 ****************************************************************/

static void
floppy_reset_controller(void)
{
    // Reset controller
    u8 val8 = inb(PORT_FD_DOR);
    outb( PORT_FD_DOR, val8 & ~0x04 );
    outb( PORT_FD_DOR, val8 | 0x04 );

    // Wait for controller to come out of reset
    while ((inb(PORT_FD_STATUS) & 0xc0) != 0x80)
        ;
}


static int
wait_floppy_irq(void)
{
    //ASSERT16();
    u8 v;
    for (;;) {
        if (!GET_BDA(floppy_motor_counter))
            return -1;
        v = GET_BDA(floppy_recalibration_status);
        if (v & FRS_TIMEOUT)
            break;
        // Could use wait_irq() here, but that causes issues on
        // bochs, so use yield() instead.
        yield();
    }

    v &= ~FRS_TIMEOUT;
    SET_BDA(floppy_recalibration_status, v);
    return 0;
}

static void
floppy_prepare_controller(u8 floppyid)
{
    CLEARBITS_BDA(floppy_recalibration_status, FRS_TIMEOUT);

    // turn on motor of selected drive, DMA & int enabled, normal operation
    u8 prev_reset = inb(PORT_FD_DOR) & 0x04;
    u8 dor = 0x10;
    if (floppyid)
        dor = 0x20;
    dor |= 0x0c;
    dor |= floppyid;
    outb( PORT_FD_DOR, dor );

    // reset the disk motor timeout value of INT 08
    SET_BDA(floppy_motor_counter, FLOPPY_MOTOR_TICKS);

    // wait for drive readiness
    while ((inb(PORT_FD_STATUS) & 0xc0) != 0x80)
        ;

    if (!prev_reset)
        wait_floppy_irq();
}

static int
floppy_pio(u8 *cmd, u8 cmdlen)
{
    floppy_prepare_controller(cmd[1] & 1);

    // send command to controller
    u8 i;
    for (i=0; i<cmdlen; i++)
        outb( PORT_FD_DATA, cmd[i] );

    int ret = wait_floppy_irq();
    if (ret) {
        floppy_reset_controller();
        return -1;
    }

    return 0;
}

static int
floppy_cmd( u32 addr, u16 count, u8 *cmd, u8 cmdlen)
{
    // es:bx = pointer to where to place information from diskette
    //u32 addr = (u32)op->buf_fl;

    // check for 64K boundary overrun
    u16 end = count - 1;
    u32 last_addr = addr + end;
    if ((addr >> 16) != (last_addr >> 16))
        return DISK_RET_EBOUNDARY;

    u8 mode_register = 0x4a; // single mode, increment, autoinit disable,
    if (cmd[0] == 0xe6)
        // read
        mode_register = 0x46;

    //DEBUGF("floppy dma c2\n");
    outb( PORT_DMA1_MASK_REG, 0x06 );
    outb( PORT_DMA1_CLEAR_FF_REG, 0x00 ); // clear flip-flop
    outb( PORT_DMA_ADDR_2, (addr_t)addr );
    outb( PORT_DMA_ADDR_2, (addr_t)addr>>8 );
    outb( PORT_DMA1_CLEAR_FF_REG, 0x00 ); // clear flip-flop
    outb( PORT_DMA_CNT_2, (addr_t)end );
    outb( PORT_DMA_CNT_2, (addr_t)end>>8 );

    // port 0b: DMA-1 Mode Register
    // transfer type=write, channel 2
    outb( PORT_DMA1_MODE_REG, mode_register );

    // port 81: DMA-1 Page Register, channel 2
    outb( PORT_DMA_PAGE_2, (addr_t)addr>>16 );

    outb( PORT_DMA1_MASK_REG, 0x02 ); // unmask channel 2

    int ret = floppy_pio(cmd, cmdlen);
    if (ret)
        return DISK_RET_ETIMEOUT;

    // check port 3f4 for accessibility to status bytes
    if ((inb(PORT_FD_STATUS) & 0xc0) != 0xc0)
        return DISK_RET_ECONTROLLER;

    // read 7 return status bytes from controller
    u8 i;
    for (i=0; i<7; i++) {
        u8 v = inb(PORT_FD_DATA);
        cmd[i] = v;
        SET_BDA(floppy_return_status[i], v);
    }

    return DISK_RET_SUCCESS;
}


/****************************************************************
 * Floppy media sense
 ****************************************************************/

static inline void
set_diskette_current_cyl(u8 floppyid, u8 cyl)
{
    SET_BDA(floppy_track[floppyid], cyl);
}

static void
floppy_drive_recal(u8 floppyid)
{
    // send Recalibrate command (2 bytes) to controller
    u8 data[12];
    data[0] = 0x07;  // 07: Recalibrate
    data[1] = floppyid; // 0=drive0, 1=drive1
    floppy_pio(data, 2);

    SETBITS_BDA(floppy_recalibration_status, 1<<floppyid);
    set_diskette_current_cyl(floppyid, 0);
}


static int
floppy_media_sense(struct drive_s *drive_g)
{
    // for now cheat and get drive type from CMOS,
    // assume media is same as drive type

    // ** config_data **
    // Bitfields for diskette media control:
    // Bit(s)  Description (Table M0028)
    //  7-6  last data rate set by controller
    //        00=500kbps, 01=300kbps, 10=250kbps, 11=1Mbps
    //  5-4  last diskette drive step rate selected
    //        00=0Ch, 01=0Dh, 10=0Eh, 11=0Ah
    //  3-2  {data rate at start of operation}
    //  1-0  reserved

    // ** media_state **
    // Bitfields for diskette drive media state:
    // Bit(s)  Description (Table M0030)
    //  7-6  data rate
    //    00=500kbps, 01=300kbps, 10=250kbps, 11=1Mbps
    //  5  double stepping required (e.g. 360kB in 1.2MB)
    //  4  media type established
    //  3  drive capable of supporting 4MB media
    //  2-0  on exit from BIOS, contains
    //    000 trying 360kB in 360kB
    //    001 trying 360kB in 1.2MB
    //    010 trying 1.2MB in 1.2MB
    //    011 360kB in 360kB established
    //    100 360kB in 1.2MB established
    //    101 1.2MB in 1.2MB established
    //    110 reserved
    //    111 all other formats/drives

    u8 ftype = GET_GLOBAL(drive_g->floppy_type);
    SET_BDA(floppy_last_data_rate, GET_GLOBAL(FloppyInfo[ftype].config_data));
    u8 floppyid = GET_GLOBAL(drive_g->cntl_id);
    SET_BDA(floppy_media_state[floppyid]
            , GET_GLOBAL(FloppyInfo[ftype].media_state));
    return DISK_RET_SUCCESS;
}

static int
check_recal_drive(struct drive_s *drive_g)
{
    u8 floppyid = GET_GLOBAL(drive_g->cntl_id);
    if ((GET_BDA(floppy_recalibration_status) & (1<<floppyid))
        && (GET_BDA(floppy_media_state[floppyid]) & FMS_MEDIA_DRIVE_ESTABLISHED))
        // Media is known.
        return DISK_RET_SUCCESS;

    // Recalibrate drive.
    floppy_drive_recal(floppyid);

    // Sense media.
    return floppy_media_sense(drive_g);
}


/****************************************************************
 * Floppy handlers
 ****************************************************************/

/*
static void
lba2chs(struct disk_op_s *op, u8 *track, u8 *sector, u8 *head)
{
    u32 lba = op->lba;

    u32 tmp = lba + 1;
    u16 nlspt = GET_GLOBAL(op->drive_g->lchs.spt);
    *sector = tmp % nlspt;

    tmp /= nlspt;
    u16 nlh = GET_GLOBAL(op->drive_g->lchs.heads);
    *head = tmp % nlh;

    tmp /= nlh;
    *track = tmp;
}
*/

static void
lba2chs1( struct drive_s *drive_g, u32 lba, u8 *track, u8 *sector, u8 *head)
{
    u32 tmp = lba + 1;
    u16 nlspt = GET_GLOBAL(drive_g->lchs.spt);
    *sector = tmp % nlspt;

    tmp /= nlspt;
    u16 nlh = GET_GLOBAL(drive_g->lchs.heads);
    *head = tmp % nlh;

    tmp /= nlh;
    *track = tmp;
}


#if 0
// diskette controller reset
static int
floppy_reset(u8 floppyid)
{
    assert(floppyid < NFLOPPY);
    //u8 floppyid = GET_GLOBAL(op->drive_g->cntl_id);
    set_diskette_current_cyl(floppyid, 0); // current cylinder
    return DISK_RET_SUCCESS;
}
#endif

// Read Diskette Sectors
static int
floppy_read(u8 floppyid, int sectNo, int sectCount, u32 addr )
{
    assert(floppyid < NFLOPPY);

    int res = 0;
    //*done = 0;

    struct drive_s *drive_g = drive[floppyid];
    if(drive_g == 0)
    {
        res = DISK_RET_EPARAM;
        goto fail;
    }

    res = check_recal_drive(drive_g);
    if (res)
        goto fail;

    u8 track, sector, head;
    //lba2chs(op, &track, &sector, &head);
    lba2chs1( drive_g, sectNo, &track, &sector, &head);

    // send read-normal-data command (9 bytes) to controller
    //u8 floppyid = GET_GLOBAL(drive_g->cntl_id);
    u8 data[12];
    data[0] = 0xe6; // e6: read normal data
    data[1] = (head << 2) | floppyid; // HD DR1 DR2
    data[2] = track;
    data[3] = head;
    data[4] = sector;
    data[5] = FLOPPY_SIZE_CODE;
    data[6] = sector + sectCount - 1; // last sector to read on track
    data[7] = FLOPPY_GAPLEN;
    data[8] = FLOPPY_DATALEN;

    res = floppy_cmd( addr, sectCount * DISK_SECTOR_SIZE, data, 9);
    if (res)
        goto fail;

    if (data[0] & 0xc0) {
        res = DISK_RET_ECONTROLLER;
        goto fail;
    }

    // ??? should track be new val from return_status[3] ?
    set_diskette_current_cyl(floppyid, track);
    return DISK_RET_SUCCESS;
fail:
    //op->count = 0; // no sectors read
    //*done = 0;
    return res;
}

// Write Diskette Sectors
static int
floppy_write(u8 floppyid, int sectNo, int sectCount, u32 addr )
{
    assert(floppyid < NFLOPPY);

    struct drive_s *drive_g = drive[floppyid];

    int res = check_recal_drive(drive_g);
    if (res)
        goto fail;

    u8 track, sector, head;
    //lba2chs(op, &track, &sector, &head);
    lba2chs1( drive_g, sectNo, &track, &sector, &head);

    // send write-normal-data command (9 bytes) to controller
    //u8 floppyid = GET_GLOBAL(op->drive_g->cntl_id);
    u8 data[12];
    data[0] = 0xc5; // c5: write normal data
    data[1] = (head << 2) | floppyid; // HD DR1 DR2
    data[2] = track;
    data[3] = head;
    data[4] = sector;
    data[5] = FLOPPY_SIZE_CODE;
    data[6] = sector + sectCount - 1; // last sector to write on track
    data[7] = FLOPPY_GAPLEN;
    data[8] = FLOPPY_DATALEN;

    res = floppy_cmd( addr, sectCount * DISK_SECTOR_SIZE, data, 9);
    if (res)
        goto fail;

    if (data[0] & 0xc0) {
        if (data[1] & 0x02)
            res = DISK_RET_EWRITEPROTECT;
        else
            res = DISK_RET_ECONTROLLER;
        goto fail;
    }

    // ??? should track be new val from return_status[3] ?
    set_diskette_current_cyl(floppyid, track);
    return DISK_RET_SUCCESS;
fail:
    //op->count = 0; // no sectors read
    return res;
}

#if 0
// Verify Diskette Sectors
static int
floppy_verify(struct disk_op_s *op)
{
    int res = check_recal_drive(op->drive_g);
    if (res)
        goto fail;

    u8 track, sector, head;
    lba2chs(op, &track, &sector, &head);

    // ??? should track be new val from return_status[3] ?
    u8 floppyid = GET_GLOBAL(op->drive_g->cntl_id);
    set_diskette_current_cyl(floppyid, track);
    return DISK_RET_SUCCESS;
fail:
    op->count = 0; // no sectors read
    return res;
}
#endif

#if 0
// format diskette track
static int
floppy_format(struct disk_op_s *op)
{
    int ret = check_recal_drive(op->drive_g);
    if (ret)
        return ret;

    u8 head = op->lba;

    // send format-track command (6 bytes) to controller
    u8 floppyid = GET_GLOBAL(op->drive_g->cntl_id);
    u8 data[12];
    data[0] = 0x4d; // 4d: format track
    data[1] = (head << 2) | floppyid; // HD DR1 DR2
    data[2] = FLOPPY_SIZE_CODE;
    data[3] = op->count; // number of sectors per track
    data[4] = FLOPPY_FORMAT_GAPLEN;
    data[5] = FLOPPY_FILLBYTE;

    ret = floppy_cmd(op, op->count * 4, data, 6);
    if (ret)
        return ret;

    if (data[0] & 0xc0) {
        if (data[1] & 0x02)
            return DISK_RET_EWRITEPROTECT;
        return DISK_RET_ECONTROLLER;
    }

    set_diskette_current_cyl(floppyid, 0);
    return DISK_RET_SUCCESS;
}
#endif


/****************************************************************
 * HW irqs
 ****************************************************************/

// INT 0Eh Diskette Hardware ISR Entry Point
static void floppy_interrupt(void *arg)
{
    (void) arg;

    SHOW_FLOW0(10, "floppy intr");

    if( !CONFIG_FLOPPY )
        return;

    if ((inb(PORT_FD_STATUS) & 0xc0) != 0xc0) {
        outb( PORT_FD_DATA, 0x08 ); // sense interrupt status
        while ((inb(PORT_FD_STATUS) & 0xc0) != 0xc0)
            ;
        do {
            inb(PORT_FD_DATA);
        } while ((inb(PORT_FD_STATUS) & 0xc0) == 0xc0);
    }
    // diskette interrupt has occurred
    SETBITS_BDA(floppy_recalibration_status, FRS_TIMEOUT);

//done:    //eoi_pic1();
}

// Called from int08 handler.
static void
floppy_tick(void)
{
    if (! CONFIG_FLOPPY)
        return;

    // time to turn off drive(s)?
    u8 fcount = GET_BDA(floppy_motor_counter);
    if (fcount) {
        fcount--;
        SET_BDA(floppy_motor_counter, fcount);
        if (fcount == 0)
            // turn motor(s) off
            outb( PORT_FD_DOR, inb(PORT_FD_DOR) & 0xcf );
    }
}

// Reset DMA controller
static void
init_dma(void)
{
    // first reset the DMA controllers
    outb( PORT_DMA1_MASTER_CLEAR, 0 );
    outb( PORT_DMA2_MASTER_CLEAR, 0 );

    // then initialize the DMA controllers
    outb( PORT_DMA2_MODE_REG, 0xc0 );
    outb( PORT_DMA2_MASK_REG, 0x00 );
}


// -----------------------------------------------------------------------
// OS interface
// -----------------------------------------------------------------------

#include <kernel/page.h>
#include <pager_io_req.h>
#include <disk.h>
#include <errno.h>
#include <dpc.h>

#define LOWBUF_SZ (64*1024)

static hal_mutex_t fmutex;
static dpc_request fd_dpc;
static pager_io_request *cur_rq;
static u8 rq_floppyid;

static physaddr_t lowbuf_p;
static void *     lowbuf_v;

static void dpc_func(void *a)
{
    (void) a;

    pager_io_request *rq = cur_rq;
    assert(rq);
    cur_rq = 0;

    size_t bytes = rq->nSect * 512;

    if(rq->flag_pageout)
    {
        memcpy_p2v( lowbuf_v, rq->phys_page, bytes );
        rq->rc = floppy_write( rq_floppyid, rq->blockNo, rq->nSect, lowbuf_p );
    }
    else
    {
        rq->rc = floppy_read( rq_floppyid, rq->blockNo, rq->nSect, lowbuf_p );
        memcpy_v2p( rq->phys_page, lowbuf_v, bytes );
        //hexdump( lowbuf_v, bytes, 0, 0 );
    }

    if(rq->rc)
    {
        rq->flag_ioerror = 1;
        rq->rc = EIO;
    }

    //rq->pager_callback( rq, rq->flag_pagein );
    pager_io_request_done( rq );

}


static errno_t floppy_AsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    (void) p;

    // Does it syncronously in fact

    //hal_mutex_lock( &fmutex );

    assert( cur_rq == 0 );

    rq_floppyid = ((int)p->specific)-1;
    cur_rq = rq;

    rq->flag_ioerror = 0;
    rq->rc = 0;


    dpc_request_trigger( &fd_dpc, 0);

    //hal_mutex_unlock( &fmutex );

    return 0;
}

static phantom_disk_partition_t *phantom_create_floppy_partition_struct( long size, int unit )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size);

    ret->asyncIo = floppy_AsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;

    ret->specific = (void *)unit; // must be not 0 for real disk

    //struct disk_q *q = calloc( 1, sizeof(struct disk_q) );
    //phantom_init_disk_q( q, startIoFunc );

    snprintf( ret->name, sizeof(ret->name), "Floppy%d", unit );


    //q->device = private;
    //q->unit = unit; // if this is multi-unit device, let 'em distinguish

    // errno_t phantom_register_disk_drive(ret);


    return ret;
}


static timedcall_t floppy_timer =
{
    (void *)floppy_tick, 0,
    20, // msec
    0, // spin

    0, { 0, 0 }, 0
};

static int seq_number = 0;
phantom_device_t * driver_isa_floppy_probe( int port, int irq, int stage )
{
    (void) stage;
    (void) port;
    (void) irq;

    if (! CONFIG_FLOPPY)
        return 0;

    if( seq_number == 0 )
    {
        SHOW_FLOW0( 3, "init floppy controller" );
        init_dma();

        hal_mutex_init( &fmutex, "floppyIo" );
        dpc_request_init( &fd_dpc, dpc_func );

        //hal_pv_alloc( &lowbuf_p, &lowbuf_v, LOWBUF_SZ );
        int npages = BYTES_TO_PAGES(LOWBUF_SZ);

        assert( !hal_alloc_phys_pages_low( &lowbuf_p, npages ) );
        assert( !hal_alloc_vaddress( &lowbuf_v, npages ) );

        hal_pages_control( lowbuf_p, lowbuf_v, npages, page_map_io, page_readwrite );


        outb( PORT_DMA1_MASK_REG, 0x02 );

        phantom_request_timed_call( &floppy_timer, TIMEDCALL_FLAG_PERIODIC );

        if( hal_irq_alloc( 6, &floppy_interrupt, 0, HAL_IRQ_SHAREABLE ) )
        {
            SHOW_ERROR( 0, "IRQ %d is busy", 6 );
            return 0;
        }
    }

    SHOW_FLOW0( 3, "init floppy drives" );

    u8 type = isa_rtc_read_reg(CMOS_FLOPPY_DRIVE_TYPE);

    phantom_device_t * ret = 0;
    int gotit = 0;

    if( (seq_number == 0) && (type & 0xf0) )
    {
        addFloppy(0, type >> 4);
        gotit = 1;
    }

    if( (seq_number == 1) && (type & 0x0f) )
    {
        addFloppy(1, type & 0x0f);
        gotit = 1;
    }

    if( gotit )
    {
        seq_number++;

#if 1
        int size = 1440*2; // TODO set actual from driver?
        phantom_disk_partition_t *p = phantom_create_floppy_partition_struct( size, seq_number );
        if(p == 0)
        {
            SHOW_ERROR0( 0, "Failed to create floppy disk partition" );
            //return 0;
        }
        else
        {
#if 0
            run_thread((void *)phantom_register_disk_drive,p);
#else
            errno_t err = phantom_register_disk_drive(p);
            if(err)
            {
                SHOW_ERROR( 0, "floppy %d err %d", seq_number, err );
                //return;
            }
#endif

        }
#endif
    }

    return ret;
}


