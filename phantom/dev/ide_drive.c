#ifdef ARCH_ia32
/*
 * @file
 * @brief
 *
 * @date 24.08.2012
 * @author Andrey Gazukin
 */


/*
 * hd.c
 *
 * IDE driver
 *
 * Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "ide.h"


//#include <util/indexator.h>
//#include <util/log.h>

//#include <asm/io.h>

//#include <embox/unit.h>
//#include <kernel/irq.h>
//#include <kernel/time/clock_source.h>

//#include <drivers/ide.h>
//#include <drivers/block_dev.h>
//#include <drivers/block_dev/partition.h>
//#include <mem/phymem.h>

#define DEBUG_MSG_PREFIX "ide/embox"
#include <debug_ext.h>
#define debug_level_flow 2
#define debug_level_error 12
#define debug_level_info 2
#define debug_level_info 2


#warning move me to ia32 dir
#include <ia32/pio.h>

#include <device.h>

#include <disk.h>
#include <disk_q.h>

#include <kernel/drivers.h>
#include <kernel/dpc.h>

#include <pager_io_req.h>

#include <sys/libkern.h>


#define DEV_NAME "ide/embox"


// Oh GOD, embox uses outb( val, port )

#define routb( ___v, ___p ) outb( ___p, ___v )
#define routl( ___v, ___p ) outl( ___p, ___v )


//static hdc_t hdctab[HD_CONTROLLERS];
//static hd_t  hdtab[HD_DRIVES];
//static struct ide_tab ide;
static long tmr_cmd_start_time;

/* general indexator for all ide disk */
//INDEX_DEF(harddisk_idx,0,HD_DRIVES);
//struct indexator *idedisk_idx;

static void hd_fixstring(unsigned char *s, int len) {
	unsigned char *p = s;
	unsigned char *end = s + len;

	/* Convert from big-endian to host byte order */
	for (p = end ; p != s;) {
		 unsigned short *pp = (unsigned short *) (p -= 2);
		*pp = ((*pp & 0x00FF) << 8) | ((*pp & 0xFF00) >> 8);
	}

	/* Strip leading blanks */
	while (s != end && *s == ' ') {
		++s;
	}

	/* Compress internal blanks and strip trailing blanks */
	while (s != end && *s) {
		if (*s++ != ' ' || (s != end && *s && *s != ' ')) {
			*p++ = *(s - 1);
		}
	}

	/* Wipe out trailing garbage */
	while (p != end) {
		*p++ = '\0';
	}
}

static void hd_error(char *func, unsigned char error) {

	if (error & HDCE_BBK)   log_debug("bad block  ");
	if (error & HDCE_UNC)   log_debug("uncorrectable data  ");
	if (error & HDCE_MC)    log_debug("media change  ");
	if (error & HDCE_IDNF)  log_debug("id not found  ");
	if (error & HDCE_MCR)   log_debug("media change requested  ");
	if (error & HDCE_ABRT)  log_debug("abort  ");
	if (error & HDCE_TK0NF) log_debug("track 0 not found  ");
	if (error & HDCE_AMNF)  log_debug("address mark not found  ");

	return;
}

static long system_read_timer(void) {
	//struct timespec ts;
	//ktime_get_timespec (&ts);
        //return (long) ts.tv_sec;
        return (long) hal_system_time() / 1000L; // msec
}

static void tmr_set_timeout(void) {
	/* get the command start time */
	tmr_cmd_start_time = system_read_timer();
}

static int tmr_chk_timeout(long timeout) {
	long curTime;

	/* get current time */
	curTime = system_read_timer();

	/* timed out yet ? */
	if (curTime >= (tmr_cmd_start_time + timeout)) {
	  return 1;      /* yes */
	}
	/* no timeout yet */
	return 0;
}


int ide_wait(hdc_t *hdc, unsigned char mask, unsigned int timeout) {
	unsigned char status, error;

	tmr_set_timeout();
	while (1) {
		status = inb(hdc->iobase + HDC_ALT_STATUS);
		if (status & HDCS_ERR) {
			error = inb(hdc->iobase + HDC_ERR);
			hd_error("hdwait", error);

			return -EIO;
		}

		if (!(status & HDCS_BSY) && ((status & mask) == mask)) {
			return 0;
		}
		if (tmr_chk_timeout(timeout)) {
			return -ETIMEDOUT;
		}
		if ((HDCS_DRQ == mask) && (0 == status)) {
			return -EIO;
		}
	}
	return -ETIMEDOUT;
}

void ide_select_drive(hd_t *hd) {
	routb(hd->drvsel, hd->hdc->iobase + HDC_DRVHD);
}

void hd_setup_transfer(hd_t *hd, blkno_t blkno, int nsects) {
	unsigned int track;
	unsigned int head;
	unsigned int sector;

	if (hd->lba) {
		track = (blkno >> 8) & 0xFFFF;
		head = ((blkno >> 24) & 0xF) | HD_LBA;
		sector = blkno & 0xFF;
	} else {
		track = blkno / (hd->heads * hd->sectors);
		head = (blkno / hd->sectors) % hd->heads;
		sector = blkno % hd->sectors + 1;
	}

	routb(0, hd->hdc->iobase + HDC_FEATURE);
	routb(nsects, hd->hdc->iobase + HDC_SECTORCNT);
	routb((unsigned char) sector, hd->hdc->iobase + HDC_SECTOR);
	routb((unsigned char) track, hd->hdc->iobase + HDC_TRACKLSB);
	routb((unsigned char) (track >> 8), hd->hdc->iobase + HDC_TRACKMSB);
	routb((((unsigned char) head & 0xFF) | (unsigned char) hd->drvsel),
		 hd->hdc->iobase + HDC_DRVHD);
}

void pio_read_buffer(hd_t *hd, char *buffer, int size) {
	hdc_t *hdc = hd->hdc;

	if (hd->use32bits) {
		insl(hdc->iobase + HDC_DATA, buffer, size / 4);
	} else {
		insw(hdc->iobase + HDC_DATA, buffer, size / 2);
	}
}

void pio_write_buffer(hd_t *hd, char *buffer, int size) {
	hdc_t *hdc = hd->hdc;

	if (hd->use32bits) {
		outsl(hdc->iobase + HDC_DATA, buffer, size / 4);
	} else {
		outsw(hdc->iobase + HDC_DATA, buffer, size / 2);
	}
}


static int hd_identify(hd_t *hd) {
	//struct block_dev *bdev = hd->bdev;
	/* Ignore interrupt for identify command */
	hd->hdc->dir = HD_XFER_IGNORE;

	/* Issue read drive parameters command */
	routb(0, hd->hdc->iobase + HDC_FEATURE);
	routb(hd->drvsel, hd->hdc->iobase + HDC_DRVHD);
	routb(hd->iftype == HDIF_ATAPI ? HDCMD_PIDENTIFY : HDCMD_IDENTIFY,
			hd->hdc->iobase + HDC_COMMAND);
	/*
	* Some controllers issues the interrupt before data is ready to be read
	* Make sure data is ready by waiting for DRQ to be set
	*/
	if (ide_wait(hd->hdc, HDCS_DRQ, HDTIMEOUT_DRQ) < 0) {
		return -EIO;
	}

	/* Read parameter data */
	insw(hd->hdc->iobase + HDC_DATA,
			(char *) &(hd->param), sizeof(hd->param) / 2);

        // ------------------------------------------------------
        // Phantom
        parse_i_disk_ata( &hd->disk_info, (void *) &(hd->param) ); // TODO check buf size
        dump_i_disk( &hd->disk_info );
        // ------------------------------------------------------


	/* XXX this was added when ide drive with reported block size equals 64
 	 * However, block dev tries to use this and fails * /
	if (bdev) {
		assert(bdev->size == 512);
		if (hd->param.unfbytes < bdev->block_size) {
			hd->param.unfbytes = bdev->block_size;
		}
                } TODO return me?*/

	/* Fill in drive parameters */
	hd->cyls = hd->param.cylinders;
	hd->heads = hd->param.heads;
	hd->sectors = hd->param.sectors;
	hd->use32bits = 0;/*hd->param.usedmovsd != 0; */
	hd->sectbufs = hd->param.buffersize;
	hd->multsect = hd->param.nsecperint;
	if (hd->multsect == 0) {
		hd->multsect = 1;
	}

	hd_fixstring((unsigned char *)hd->param.model, sizeof(hd->param.model));
	hd_fixstring((unsigned char *)hd->param.rev, sizeof(hd->param.rev));
	hd_fixstring((unsigned char *)hd->param.serial, sizeof(hd->param.serial));

	if (hd->iftype == HDIF_ATA) {
		hd->media = IDE_DISK;
	} else {
		hd->media = (hd->param.config >> 8) & 0x1f;
	}

	/* Determine LBA or CHS mode */
	if ((hd->param.caps & 0x0200) == 0) {
		hd->lba = 0;
		hd->blks = hd->cyls * hd->heads * hd->sectors;
		if (hd->cyls == 0 && hd->heads == 0 && hd->sectors == 0) {
			return -EIO;
		}
		if (hd->cyls == 0xFFFF && hd->heads == 0xFFFF
			&& hd->sectors == 0xFFFF) {
			return -EIO;
		}
	} else {
		hd->lba = 1;
		hd->blks = (hd->param.totalsec1 << 16) | hd->param.totalsec0;
		if (hd->media == IDE_DISK && (hd->blks == 0 ||
			hd->blks == 0xFFFFFFFF)) {
			return -EIO;
		}
	}

	return 0;
}

static int hd_cmd(hd_t *hd, unsigned int cmd,
				  unsigned int feat, unsigned int nsects) {
	/* Ignore interrupt for command */
	hd->hdc->dir = HD_XFER_IGNORE;

	/* Issue command */
	routb(feat, hd->hdc->iobase + HDC_FEATURE);
	routb(nsects, hd->hdc->iobase + HDC_SECTORCNT);
	routb(hd->drvsel, hd->hdc->iobase + HDC_DRVHD);
	routb(cmd, hd->hdc->iobase + HDC_COMMAND);

	/* Check status */
	if (hd->hdc->result < 0) {
		return -EIO;
	}

	return 0;
}
/*
int hd_ioctl(struct block_dev *bdev, int cmd, void *args, size_t size) {
	struct dev_geometry *geom;
	hd_t *hd = (hd_t *) bdev->privdata;

	switch (cmd) {
	case IOCTL_GETDEVSIZE:
		return hd->blks;

	case IOCTL_GETBLKSIZE:
		return hd->param.unfbytes;

	case IOCTL_GETGEOMETRY:
		if (!args || size != sizeof(struct dev_geometry)) {
			return -EINVAL;
		}
		geom = (struct dev_geometry *) args;
		geom->cyls = hd->cyls;
		geom->heads = hd->heads;
		geom->spt = hd->sectors;
		geom->sectorsize = hd->param.unfbytes;
		geom->sectors = hd->blks;
		return 0;

	case IOCTL_REVALIDATE:
		return create_partitions(bdev);
	}

	return -ENOSYS;
}
*/

static int hd_read_status(hdc_t *hdc, const char *op_name)
{
    hdc->status = inb(hdc->iobase + HDC_STATUS);

    if (hdc->status & HDCS_ERR)
    {
        unsigned char error;

        error = inb(hdc->iobase + HDC_ERR);
        hd_error("hdread", error);
        hdc->result = -EIO;

        return hdc->result;
    }

    return 0;
}




#if 0

static void hd_read_hndl(hdc_t *hdc) {
    //unsigned char error;
    int nsects;
    int n;

    //struct block_dev *bdev = hdc->active->bdev;

    int block_size = 512; // TODO FIXME was bdev->block_size

    if( !hd_read_status(hdc, "hd read" ) )
    {
        /* Read sector data */
        nsects = hdc->active->multsect;
        if (nsects > hdc->nsects) {
            nsects = hdc->nsects;
        }
        for (n = 0; n < nsects; n++) {
            pio_read_buffer(hdc->active, hdc->bufp, block_size);
            hdc->bufp += block_size;
        }
        hdc->nsects -= nsects;
    }
}

static void hd_write_hndl(hdc_t *hdc) {
    int nsects;
    int n;
    //struct block_dev *bdev = hdc->active->bdev;
    int block_size = 512; // TODO FIXME was bdev->block_size

    if( !hd_read_status(hdc, "hd write" ) )
    {
        /* Transfer next sector(s) or signal end of transfer */
        nsects = hdc->active->multsect;
        if (nsects > hdc->nsects) {
            nsects = hdc->nsects;
        }
        hdc->nsects -= nsects;

        if (hdc->nsects > 0) {
            nsects = hdc->active->multsect;
            if (nsects > hdc->nsects) {
                nsects = hdc->nsects;
            }

            for (n = 0; n < nsects; n++) {
                pio_write_buffer(hdc->active, hdc->bufp, block_size);
                hdc->bufp += block_size;
            }
        }
    }
}
#endif


static void hdc_handler(void *arg) {
    hdc_t *hdc = (hdc_t *) arg;

    switch (hdc->dir) {
    case HD_XFER_READ:
        //hd_read_hndl(hdc);
        hd_read_status(hdc, "hd read" );
        break;

    case HD_XFER_WRITE:
        //hd_write_hndl(hdc);
        hd_read_status(hdc, "hd write" );
        break;

    case HD_XFER_DMA:
        routb(inb(hdc->bmregbase + BM_STATUS_REG),
             hdc->bmregbase + BM_STATUS_REG);
        hdc->status = inb(hdc->iobase + HDC_STATUS);
        break;

    case HD_XFER_IGNORE:
        /* Read status to acknowledge interrupt */
        hdc->status = inb(hdc->iobase + HDC_STATUS);
        break;

    case HD_XFER_IDLE:
    default:
        /* Read status to acknowledge interrupt */
        hdc->status = inb(hdc->iobase + HDC_STATUS);
        break;
    }

    if ((0 == hdc->result) && (HD_XFER_IDLE != hdc->dir)
        && (HD_XFER_IGNORE != hdc->dir)) {
        hdc->result = 1;
        waitq_wakeup_all(&hdc->waitq);
    }

    //return IRQ_HANDLED;
}

static int probe_device(hdc_t *hdc, int drvsel) {
	unsigned char sc, sn;

        SHOW_INFO( 0, "probe iobase 0x%x, drvsel %d", hdc->iobase, drvsel );

	/* Probe for device on controller */
	routb(drvsel, hdc->iobase + HDC_DRVHD);
	idedelay();

	routb(0x55, hdc->iobase + HDC_SECTORCNT);
	routb(0xAA, hdc->iobase + HDC_SECTOR);

	routb(0xAA, hdc->iobase + HDC_SECTORCNT);
	routb(0x55, hdc->iobase + HDC_SECTOR);

	routb(0x55, hdc->iobase + HDC_SECTORCNT);
	routb(0xAA, hdc->iobase + HDC_SECTOR);

	sc = inb(hdc->iobase + HDC_SECTORCNT);
	sn = inb(hdc->iobase + HDC_SECTOR);

	if (sc == 0x55 && sn == 0xAA) {
		return 1;
	} else {
		return -EIO;
	}
}

static int wait_reset_done(hdc_t *hdc, int drvsel) {

	routb(drvsel, hdc->iobase + HDC_DRVHD);
	idedelay();

	tmr_set_timeout();
	while (1) {
		hdc->status = inb(hdc->iobase + HDC_STATUS);
		if ((hdc->status & HDCS_BSY) == 0) {
			return 0;
		}
		if (tmr_chk_timeout(HDTIMEOUT_DRDY)) {
			break;
		}
	}

	return -EBUSY;
}

static int get_interface_type(hdc_t *hdc, int drvsel) {
	unsigned char sc, sn, cl, ch, st;

	routb(drvsel, hdc->iobase + HDC_DRVHD);
	idedelay();

	sc = inb(hdc->iobase + HDC_SECTORCNT);
	sn = inb(hdc->iobase + HDC_SECTOR);

	if (sc == 0x01 && sn == 0x01) {
		cl = inb(hdc->iobase + HDC_TRACKLSB);
		ch = inb(hdc->iobase + HDC_TRACKMSB);
		st = inb(hdc->iobase + HDC_STATUS);

		if (cl == 0x14 && ch == 0xeb) {
			return HDIF_ATAPI;
		}
		if (cl == 0x00 && ch == 0x00 && st != 0x00) {
			return HDIF_ATA;
		}
	}

	return HDIF_UNKNOWN;
}

static int setup_controller(hdc_t *hdc, int iobase, int irq,
					int bmregbase, int *masterif, int *slaveif) {
	int res;

	*hdc = (struct hdc) {
		.iobase    = iobase,
		.irq       = irq,
		.bmregbase = bmregbase,
		.dir       = HD_XFER_IGNORE,
	};

        SHOW_INFO( 0, "setup controller iobase 0x%x, irq %d, bmreg 0x%x", iobase, irq, bmregbase );

	waitq_init(&hdc->waitq);

        if (hdc->bmregbase)
        {
            /* Allocate one page for PRD list */
            //hdc->prds = (struct prd *) phymem_alloc(1);

            hal_pv_alloc( &hdc->prds_phys, (void **)&hdc->prds, PAGESIZE );
        }

	/* Assume no devices connected to controller */
	*masterif = HDIF_NONE;
	*slaveif = HDIF_NONE;

	/* Setup device control register */
	routb(HDDC_HD15 | HDDC_NIEN, hdc->iobase + HDC_CONTROL);

	/* Probe for master and slave device on controller */
	if (probe_device(hdc, HD0_DRVSEL) >= 0) {
		*masterif = HDIF_PRESENT;
	}
	if (probe_device(hdc, HD1_DRVSEL) >= 0) {
		*slaveif = HDIF_PRESENT;
	}

	/* Reset controller */
	routb(HDDC_HD15 | HDDC_SRST | HDDC_NIEN, hdc->iobase + HDC_CONTROL);
	idedelay();
	routb(HDDC_HD15 | HDDC_NIEN, hdc->iobase + HDC_CONTROL);
	idedelay();

	/* Wait for reset to finish on all present devices */
	if (*masterif != HDIF_NONE) {
		int rc = wait_reset_done(hdc, HD0_DRVSEL);
		if (rc < 0) {
			*masterif = HDIF_NONE;
		}
	}

	if (*slaveif != HDIF_NONE) {
		int rc = wait_reset_done(hdc, HD1_DRVSEL);
		if (rc < 0) {
			*slaveif = HDIF_NONE;
		}
	}

	/* Determine interface types */
	if (*masterif != HDIF_NONE) {
		*masterif = get_interface_type(hdc, HD0_DRVSEL);
	}
	if (*slaveif != HDIF_NONE) {
		*slaveif = get_interface_type(hdc, HD1_DRVSEL);
	}

	/* Enable interrupts */
        //res = irq_attach(hdc->irq, hdc_handler, 0, hdc, "ide");
        res = hal_irq_alloc( hdc->irq, hdc_handler, hdc, 0 ); // not shareable?

	if (res != 0) {
            SHOW_ERROR( 0, "irq %d alloc error %d", irq, res );
            return res;
	}

	routb(HDDC_HD15, hdc->iobase + HDC_CONTROL);
	idedelay();

	return 0;
}

/*
static int ide_create_block_dev(hd_t *hd) {
	const struct block_dev_module *bdev;

	switch (hd->media) {
		case IDE_CDROM:
			bdev = block_dev_lookup("idecd");
			break;
		case IDE_DISK:
			if (hd->udmamode == -1) {
				bdev = block_dev_lookup("idedisk");
			} else {
				bdev = block_dev_lookup("idedisk_udma");
			}

			break;
		default:
			bdev = NULL;
	}
	if (bdev == NULL) {
		return 0;
	}
	bdev->dev_drv->probe(hd);

	return 0;
}
*/


static int setup_hd(hd_t *hd, hdc_t *hdc, int drvsel,
			int udmasel, int iftype ) {
	int rc;

	/* Initialize drive block */
	*hd = (struct hd) {
		.hdc    = hdc,
		.drvsel = drvsel,
		.iftype = iftype,
	};
	/* Get info block from device */
	rc = hd_identify(hd);
	if (rc < 0) {
		/* Try other interface type */
		if (hd->iftype == HDIF_ATA) {
			hd->iftype = HDIF_ATAPI;
		} else if (hd->iftype == HDIF_ATAPI) {
			hd->iftype = HDIF_ATA;
		}
		rc = hd_identify(hd);
                if (rc < 0) {
                    //ide.drive[numslot] = NULL;
                    return -1;
                }
	}
	//ide.drive[numslot]  = (hd_t *)hd;

	/* Determine UDMA mode */
	if (!hdc->bmregbase) {
		hd->udmamode = -1;
	} else if ((hd->param.valid & 4) &&
			(hd->param.dmaultra & (hd->param.dmaultra >> 8) & 0x3F)) {
		if ((hd->param.dmaultra >> 13) & 1) {
			hd->udmamode = 5; /* UDMA 100 */
		} else if ((hd->param.dmaultra >> 12) & 1) {
			hd->udmamode = 4; /* UDMA 66 */
		} else if ((hd->param.dmaultra >> 11) & 1) {
			hd->udmamode = 3; /* UDMA 44 */
		} else if ((hd->param.dmaultra >> 10) & 1) {
			hd->udmamode = 2; /* UDMA 33 */
		} else if ((hd->param.dmaultra >> 9) & 1) {
			hd->udmamode = 1; /* UDMA 25 */
		} else {
			hd->udmamode = 0; /* UDMA 16 */
		}
	} else {
		hd->udmamode = -1;
	}

	/* Set multi-sector mode if drive supports it */
	if (hd->multsect > 1) {
		rc = hd_cmd(hd, HDCMD_SETMULT, 0, hd->multsect);
		if (rc < 0) {
			hd->multsect = 1;
		}
	}

	/* Enable UDMA for drive if it supports it. */
	if (hd->udmamode != -1) {
		/* Enable drive in bus master status register */
		int dmastat = inb(hdc->bmregbase + BM_STATUS_REG);
		routb(dmastat | udmasel, hdc->bmregbase + BM_STATUS_REG);

		/* Set feature in IDE controller */
		rc = hd_cmd(hd, HDCMD_SETFEATURES, HDFEAT_XFER_MODE,
				HDXFER_MODE_UDMA | hd->udmamode);
	}

	/* Enable read ahead and write caching if supported * /
	if (hd->param.csfo & 2) {
		hd_cmd(hd, HDCMD_SETFEATURES, HDFEAT_ENABLE_RLA, 0);
	}
	if (hd->param.csfo & 1) {
		hd_cmd(hd, HDCMD_SETFEATURES, HDFEAT_ENABLE_WCACHE, 0);
	} */

	//ide_create_block_dev(hd); TODO rewrite me

        return 0;
}

/*
static int ide_init(void) {
    int rc;
    int masterif;
    int slaveif;
    int numhd;
    int i;
    int irq[] = {HDC0_IRQ, HDC1_IRQ};
    int iobase[] = {HDC0_IOBASE, HDC1_IOBASE};

    numhd = HD_DRIVES;

    idedisk_idx = &harddisk_idx;

    for (i = 0; i < HD_CONTROLLERS; i++) {
        if (numhd < i * 2 + 1)
            break;

        rc = setup_controller(&hdctab[i], iobase[i], irq[i], 0, &masterif, &slaveif);

        if (rc >= 0)
        {
            if (numhd >= i * 2 + 1 && masterif > HDIF_UNKNOWN)
                setup_hd(&hdtab[i * 2], &hdctab[i], HD0_DRVSEL, BM_SR_DRV0, masterif, i * 2);
            if (numhd >= i * 2 + 2 && slaveif > HDIF_UNKNOWN)
                setup_hd(&hdtab[i * 2 + 1], &hdctab[i], HD1_DRVSEL, BM_SR_DRV1, slaveif, i * 2 + 1);
        }
    }

    return 0;
}
*/


//struct ide_tab *ide_get_drive(void) {	return &ide;}

//EMBOX_UNIT_INIT(ide_init);



// -----------------------------------------------------------------------
// DMA disk io
// -----------------------------------------------------------------------




static void setup_dma(hdc_t *hdc, physaddr_t buffer, int count, int cmd) {
    int i;
    int len;
    physaddr_t next;

    i = 0;
    next = (buffer & ~(PAGESIZE - 1)) + PAGESIZE;
    while (1) {
        hdc->prds[i].addr = (unsigned long) buffer;
        len = next - buffer;
        if (count > len) {
            hdc->prds[i].len = len;
            count -= len;
            buffer = next;
            next += PAGESIZE;
            i++;
        } else {
            hdc->prds[i].len = count | 0x80000000;
            break;
        }
    }

    /* Setup PRD table */
    routl(hdc->prds_phys, hdc->bmregbase + BM_PRD_ADDR);

    /* Specify read/write */
    routb(cmd | BM_CR_STOP, hdc->bmregbase + BM_COMMAND_REG);

    /* Clear INTR & ERROR flags */
    routb(inb(hdc->bmregbase + BM_STATUS_REG) | BM_SR_INT | BM_SR_ERR,
         hdc->bmregbase + BM_STATUS_REG);
}

static void start_dma(hdc_t *hdc) {
    /* Start DMA operation */
    routb(inb(hdc->bmregbase + BM_COMMAND_REG) | BM_CR_START,
         hdc->bmregbase + BM_COMMAND_REG);
}

static int stop_dma(hdc_t *hdc) {
    int dmastat;

    /* Stop DMA channel and check DMA status */
    routb(inb(hdc->bmregbase + BM_COMMAND_REG) & ~BM_CR_START,
         hdc->bmregbase + BM_COMMAND_REG);

    /* Get DMA status */
    dmastat = inb(hdc->bmregbase + BM_STATUS_REG);

    /* Clear INTR && ERROR flags */
    routb(dmastat | BM_SR_INT | BM_SR_ERR, hdc->bmregbase + BM_STATUS_REG);

    /* Check for DMA errors */
    if (dmastat & BM_SR_ERR) {
        return -EIO;
    }

    return 0;
}







static int hd_read_udma(
                        //struct block_dev *bdev,
                        hd_t *hd,
                        physaddr_t mem,
                        size_t count, blkno_t blkno) {
    hdc_t *hdc;
    int sectsleft;
    int nsects;
    int result = 0;
    physaddr_t memp = mem;

    int block_size = 512; // TODO fix me

    if (count == 0)	return 0;

    hdc = hd->hdc;
    sectsleft = count; // / bdev->block_size;

    SHOW_FLOW( 11, "read udma blk %ld count %d", blkno, count );

    while (sectsleft > 0) {
        /* Select drive */
        ide_select_drive(hd);

        /* Wait for controller ready */
        result = ide_wait(hdc, HDCS_DRDY, HDTIMEOUT_DRDY);
        if (result != 0) {
            result = -EIO;
            SHOW_ERROR( 1, "can't wait, %d", result );
            break;
        }

        /* Calculate maximum number of sectors we can transfer */
        if (sectsleft > 256) {
            nsects = 256;
        } else {
            nsects = sectsleft;
        }

        if (nsects > MAX_DMA_XFER_SIZE / block_size) {
            nsects = MAX_DMA_XFER_SIZE / block_size;
        }

        /* Prepare transfer */
        result = 0;
        hdc->dir = HD_XFER_DMA;
        hdc->active = hd;

        hd_setup_transfer(hd, blkno, nsects);

        /* Setup DMA */
        setup_dma(hdc, memp, nsects * block_size, BM_CR_WRITE);

        /* Start read */
        routb(HDCMD_READDMA, hdc->iobase + HDC_COMMAND);
        start_dma(hdc);

        /* Stop DMA channel and check DMA status */
        result = stop_dma(hdc);
        if (result < 0) {
            SHOW_ERROR( 1, "can't stop dma, %d", result );
            break;
        }

        /* Check controller status */
        if (hdc->status & HDCS_ERR) {
            result = -EIO;
            SHOW_ERROR( 1, "io err, %d", result );
            break;
        }

        /* Advance to next */
        sectsleft -= nsects;
        memp += nsects * block_size;

        SHOW_FLOW( 11, "read udma %d sectors left", sectsleft );
    }

    /* Cleanup */
    hdc->dir = HD_XFER_IDLE;
    hdc->active = NULL;

    return result;
}




static int hd_write_udma(
                         hd_t *hd,
                         physaddr_t mem,
                         size_t count, blkno_t blkno) {
	hdc_t *hdc;
	int sectsleft;
	int nsects;
	int result = 0;
	physaddr_t memp = mem;

	if (count == 0) {		return 0;	}

        int block_size = 512; // TODO fix me

	hdc = hd->hdc;
	sectsleft = count; // / bdev->block_size;

	while (sectsleft > 0) {
		/* Select drive */
		ide_select_drive(hd);

		/* Wait for controller ready */
		result = ide_wait(hdc, HDCS_DRDY, HDTIMEOUT_DRDY);
		if (result != 0) {
			result = -EIO;
			break;
		}

		/* Calculate maximum number of sectors we can transfer */
		if (sectsleft > 256) {
			nsects = 256;
		} else {
			nsects = sectsleft;
		}
		if (nsects > MAX_DMA_XFER_SIZE / block_size) {
			nsects = MAX_DMA_XFER_SIZE / block_size;
		}

		/* Prepare transfer */
		result = 0;
		hdc->dir = HD_XFER_DMA;
		hdc->active = hd;

		hd_setup_transfer(hd, blkno, nsects);

		/* Setup DMA */
		setup_dma(hdc, memp, nsects * block_size, BM_CR_READ);

		/* Start write */
		routb(HDCMD_WRITEDMA, hdc->iobase + HDC_COMMAND);
		start_dma(hdc);

		/* Stop DMA channel and check DMA status */
		result = stop_dma(hdc);
		if (result < 0) {
			break;
		}

		/* Check controller status */
		if (hdc->status & HDCS_ERR) {
			result = -EIO;
			break;
		}

		/* Advance to next */
		sectsleft -= nsects;
		memp += nsects * block_size;
	}

	/* Cleanup */
	hdc->dir = HD_XFER_IDLE;
	hdc->active = NULL;

	return result;
}




/*
static int idedisk_udma_init( hd_t *drive ) {
    double size;
    char   path[PATH_MAX];


    // Make new device
    if ((drive->media == IDE_DISK) && (drive->udmamode != -1)) {
        *path = 0;
        strcat(path, "/dev/hd*");
        if (0 > (drive->idx = block_dev_named(path, idedisk_idx))) {
            return drive->idx;
        }
        drive->bdev = block_dev_create(path,
                                       &idedisk_udma_driver, drive);
        if (NULL != drive->bdev) {
            size = (double) drive->param.cylinders *
                (double) drive->param.heads *
                (double) drive->param.unfbytes *
                (double) (drive->param.sectors + 1);
            block_dev(drive->bdev)->size = (size_t) size;
        } else {
            return -1;
        }
        create_partitions(drive->bdev);
    }
    return 0;
}
*/


// -----------------------------------------------------------------------
// Disk io
// -----------------------------------------------------------------------

#if 0
// there's no io start code?!

// TODO param types
int ide_write_blk( hdc_t *hdc, int unit, physaddr_t mem, long blockNo, int nSect )
{
    int block_size = 512; // todo blocksize

    while( nSect > 0 )
    {
        int nsects = hdc->active->multsect;
        if (nsects > nSect) {
            nsects = nSect;
        }

        int n;
        for (n = 0; n < nsects; n++)
        {
            char buf[block_size];

            memcpy_p2v( buf, mem, sizeof( buf ) );
            pio_write_buffer(hdc->active, buf, block_size);
            mem += block_size;
            }

        nSect -= nsects;
    }

    return 0; // TODO error code?
}

int ide_read_blk ( hdc_t *hdc, int unit, physaddr_t mem, long blockNo, int nSect )
{
    int block_size = 512; // todo blocksize

    while( nSect > 0 )
    {
        int nsects = hdc->active->multsect;
        if (nsects > nSect) {
            nsects = nSect;
        }

        int n;
        for (n = 0; n < nsects; n++)
        {
            char buf[block_size];

            pio_read_buffer(hdc->active, buf, block_size );
            memcpy_v2p( mem, buf, sizeof( buf ) );

            mem += block_size;
        }

        nSect -= nsects;
    }

    return 0; // TODO error code?
}
#endif

// -----------------------------------------------------------------------
// Disk queue
// -----------------------------------------------------------------------



// Serves both uits
static phantom_disk_partition_t *embox_both;
static struct disk_q *ideq;
static dpc_request ide_dpc;
static hdc_t *embox_hdc; // TODO can't use specific for it is used for disk q, find solution


static void startIo( struct disk_q *q )
{
    ideq = q; // it is allways the same
    SHOW_FLOW( 11, "start on q %p", q );
    dpc_request_trigger( &ide_dpc, 0 );
}

static void dpc_func(void *a)
{
    (void) a;

    pager_io_request *rq = ideq->current;
    assert(rq);

    if( (rq->unit > 1) || (rq->unit < 0) )
    {
        SHOW_ERROR( 1, "wrong IDE unit %d", rq->unit );
        ideq->ioDone( ideq, ENXIO );
        return;
    }

    //hdc_t *hdc = embox_both->specific;
    hdc_t *hdc = embox_hdc; // TODO can't use specific for it is used for disk q, find solution
    hd_t *hd = &hdc->hdtab[rq->unit];

    if( (rq->blockNo >= hd->disk_info.nSectors) || (rq->blockNo < 0) )
    {
        SHOW_ERROR( 1, "wrong IDE blk %ld on unit %d", rq->blockNo, rq->unit );
        ideq->ioDone( ideq, EINVAL );
        return;
    }


    SHOW_FLOW( 7, "starting io on rq %p blk %ld", rq, rq->blockNo );

    errno_t rc;

    if(rq->flag_pageout)
    {
        rc = hd_write_udma( hd, rq->phys_page, rq->nSect, rq->blockNo );
        //rc = ide_write_blk( hdc, rq->unit, rq->phys_page, rq->blockNo, rq->nSect );
    }
    else
    {
        rc = hd_read_udma( hd, rq->phys_page, rq->nSect, rq->blockNo );
        //rc = ide_read_blk( hdc, rq->unit, rq->phys_page, rq->blockNo, rq->nSect );
    }

    SHOW_FLOW( 7, "calling iodone on q %p, rq %p", ideq, rq );
    assert(rq == ideq->current);
    ideq->ioDone( ideq, rc ? EIO : 0 );
}

static errno_t embox_u0AsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    rq->unit = 0;

    SHOW_FLOW( 11, "part io sect %ld, shift %ld, o sect %ld", rq->blockNo, p->shift, rq->blockNo + p->shift );

    return embox_both->asyncIo( embox_both, rq );
}

static errno_t embox_u1AsyncIo( struct phantom_disk_partition *p, pager_io_request *rq )
{
    rq->unit = 1;

    SHOW_FLOW( 11, "part io sect %ld, shift %ld, o sect %ld", rq->blockNo, p->shift, rq->blockNo + p->shift );

    return embox_both->asyncIo( embox_both, rq );
}


static errno_t ideDequeue( struct phantom_disk_partition *p, pager_io_request *rq )
{
    (void) p;

    if( 0 == embox_both->dequeue )
        return ENODEV;

    SHOW_FLOW( 11, "ide dequeue rq %p", rq );

    return embox_both->dequeue( embox_both, rq );
}

static errno_t ideRaise( struct phantom_disk_partition *p, pager_io_request *rq )
{
    (void) p;

    if( 0 == embox_both->raise )
        return ENODEV;

    SHOW_FLOW( 11, "ide raise rq prio %p", rq );

    return embox_both->raise( embox_both, rq );
}

// TODO stub!
static errno_t ideFence( struct phantom_disk_partition *p )
{
    (void) p;
    SHOW_ERROR0( 0, "Ide fence stub" );

    hdc_t *hdc = embox_hdc; // embox_both->specific;

    (void) hdc;

    /*
    if( hdc->have_dev[0] ) ide_flush( 0 );
    if( hdc->have_dev[1] ) ide_flush( 1 );
    */
// TODO write me
    return -1;
}



static void make_unit_part( int unit, void *aio, hd_t *hd )
{
    SHOW_INFO( 0, "setup ide unit %d", unit );
    /* TODO need this?
    errno_t rc = ide_reset( unit );
    if(rc)
    {
        SHOW_ERROR( 0, "skip IDE unit %d, can't reset", unit );
        return;
    }
    */

    phantom_disk_partition_t *p = phantom_create_partition_struct( embox_both, 0, hd->disk_info.nSectors );
    assert(p);
    p->flags |= PART_FLAG_IS_WHOLE_DISK;
    p->specific = (void *)-1; // disk supposed to have that
    p->base = 0; // and don't have this
    p->asyncIo = aio;
    p->dequeue = ideDequeue;
    p->raise = ideRaise;
    p->fence = ideFence;
#if IDE_TRIM
    p->trim = ideTrim;
#endif
    snprintf( p->name, sizeof(p->name), "Ide%d", unit );
    errno_t err = phantom_register_disk_drive(p);
    if(err)
        SHOW_ERROR( 0, "Ide %d register disk err=%d", unit, err );

}




// -----------------------------------------------------------------------
// Phantom OS connector
// -----------------------------------------------------------------------




static int ide_read_bytes( struct phantom_device *dev, void *buf, int len)
{
    hdc_t *hdc = (hdc_t *)dev->drv_private;

    (void) hdc;
    (void) buf;
    (void) len;
    /*
    if(len < ETHERNET_MAX_SIZE)
        return ERR_VFS_INSUFFICIENT_BUF;
        return pcnet32_rx(nic, buf, len);
        */

    return 0; // TODO write me
}

static int ide_write_bytes(struct phantom_device *dev, const void *buf, int len)
{
    hdc_t *hdc = (hdc_t *)dev->drv_private;
    (void) hdc;
    (void) buf;
    (void) len;

    return 0; // TODO write me
}



// TODO register us by class 1:1 in PCI dev table


static int seq_number = 0;

phantom_device_t * driver_embox_ide_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    int rc;

    if( seq_number )
    {
        SHOW_ERROR0( 0, "just one IDE controller yet");
        return 0;
    }

    int irq[] = {HDC0_IRQ, HDC1_IRQ};
    int iobase[] = {HDC0_IOBASE, HDC1_IOBASE};

    int bm_base_reg = pci->base[4];


    SHOW_INFO0( 0, "probe");

    //nic = pcnet32_new( PCNET_INIT_MODE0 | PCNET_INIT_RXLEN_128 | PCNET_INIT_TXLEN_32, 2048, 2048);

    hdc_t *hdc = calloc( 1, sizeof( hdc_t ) );
    if( hdc == 0 )
    {
        SHOW_ERROR0( 0, "out of mem");
        return 0;
    }

    int masterif;
    int slaveif;

    rc = setup_controller( hdc, iobase[seq_number], irq[seq_number], bm_base_reg, &masterif, &slaveif);
    if(rc)
    {
        SHOW_ERROR( 0, DEV_NAME "setup_controller returned %d\n", rc);
        return 0;
    }

    //nic->irq = pci->interrupt;
    /*
    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            nic->phys_base = (pci->base[i]);
            nic->phys_size = pci->size[i];
            //if(DEBUG) printf( " base 0x%lx, size 0x%lx\n", nic->phys_base, nic->phys_size);
        } else if( pci->base[i] > 0) {
            nic->io_port = pci->base[i];
            //if(DEBUG) printf( "io_port 0x%x\n", nic->io_port);
        }
    }
    */

    dpc_request_init( &ide_dpc, dpc_func );

    SHOW_INFO( 0, "master %d, slave %d", masterif, slaveif );

    if( (masterif > HDIF_UNKNOWN) || (slaveif > HDIF_UNKNOWN) )
    {
        long both_size = -1; //lmax(disk_info[0].nSectors, disk_info[1].nSectors);
        embox_both = phantom_create_disk_partition_struct( both_size, 0, 0, startIo );

        //embox_both->specific = hdc;
        embox_hdc = hdc;
    }

    if(masterif > HDIF_UNKNOWN)
    {
        hd_t *hd = &hdc->hdtab[0];
        rc = setup_hd( hd, hdc, HD0_DRVSEL, BM_SR_DRV0, masterif );

        embox_both->size = lmax( embox_both->size, hd->disk_info.nSectors );

        if( !rc )
        {
            make_unit_part( 0, &embox_u0AsyncIo, hd );
            hdc->have_dev[0] = 1;
        }
    }

    if(slaveif > HDIF_UNKNOWN)
    {
        hd_t *hd = &hdc->hdtab[1];
        rc = setup_hd( hd, hdc, HD1_DRVSEL, BM_SR_DRV1, slaveif );

        embox_both->size = lmax( embox_both->size, hd->disk_info.nSectors );

        if( !rc )
        {
            hdc->have_dev[1] = 1;
            make_unit_part( 1, &embox_u1AsyncIo, hd );
        }
    }


    //long both_size = lmax( hdc->hdtab[0].disk_info.nSectors, hdc->hdtab[1].disk_info.nSectors);
    //embox_both->size = both_size; // FIXME TODO hack, write partition_set_size?

/*
    if(masterif > HDIF_UNKNOWN)
    {
    }

    if(slaveif > HDIF_UNKNOWN)
    {
    }
*/

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    if( dev == 0 ) return 0; // TODO undo init/allocs

    dev->name = "ide/embox";
    dev->seq_number = seq_number++;
    dev->drv_private = hdc;

    dev->dops.read = ide_read_bytes;
    dev->dops.write = ide_write_bytes;
    dev->dops.get_address = NULL;

    dev->iobase = iobase[seq_number];
    dev->irq = irq[seq_number];
    dev->iomem = 0;
    dev->iomemsize = 0;


    return dev;

}



// TODO move to dev indep place and connect to regular disk io


void phantom_check_disk_check_virtmem( void * a, int n )
{
    (void) a;
    (void) n;
}

void phantom_check_disk_save_virtmem( void * a, int n )
{
    (void) a;
    (void) n;
}

















#endif // #ifdef ARCH_ia32
