#if 0
/*
 * Copyright (C) 2005-2007 by egnite Software GmbH. All rights reserved.
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
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY EGNITE SOFTWARE GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EGNITE
 * SOFTWARE GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

/*!
 * \brief Basic block device driver for multimedia cards.
 *
 * The driver uses SPI mode, but doesn't include any low level hardware
 * access. This must be provided by some additional routines.
 *
 * \verbatim
 *
 * $Log$
 * Revision 1.15  2009/02/13 14:52:05  haraldkipp
 * Include memdebug.h for heap management debugging support.
 *
 * Revision 1.14  2009/02/06 15:40:29  haraldkipp
 * Using newly available strdup() and calloc().
 * Replaced NutHeap routines by standard malloc/free.
 * Replaced pointer value 0 by NULL.
 *
 * Revision 1.13  2009/01/17 11:26:46  haraldkipp
 * Getting rid of two remaining BSD types in favor of stdint.
 * Replaced 'u_int' by 'unsinged int' and 'uptr_t' by 'uintptr_t'.
 *
 * Revision 1.12  2008/08/11 06:59:42  haraldkipp
 * BSD types replaced by stdint types (feature request #1282721).
 *
 * Revision 1.11  2008/07/14 13:09:30  haraldkipp
 * Allow small MultiMedia Cards without partition table.
 *
 * Revision 1.10  2007/08/30 12:15:06  haraldkipp
 * Configurable MMC timings.
 *
 * Revision 1.9  2006/10/08 16:48:09  haraldkipp
 * Documentation fixed
 *
 * Revision 1.8  2006/07/05 08:03:12  haraldkipp
 * Bugfix. Trailing slash in mount path not properly handled.
 * Thanks to Michael Fischer.
 *
 * Revision 1.7  2006/06/18 16:34:46  haraldkipp
 * Mutex deadlock fixed.
 *
 * Revision 1.6  2006/05/25 09:34:21  haraldkipp
 * Added mutual exclusion lock for multithreaded access.
 *
 * Revision 1.5  2006/04/07 12:29:03  haraldkipp
 * Number of read retries increased. Memory hole fixed.
 * Added ioctl(NUTBLKDEV_MEDIAAVAIL).
 * Card change ioctl() will also return 1 if no card is available.
 *
 * Revision 1.4  2006/02/23 15:43:56  haraldkipp
 * Timeout value increased. Some cards have long write latencies.
 *
 * Revision 1.3  2006/01/22 17:36:31  haraldkipp
 * Some cards need more time to enter idle state.
 * Card access now returns an error after card change detection.
 *
 * Revision 1.2  2006/01/19 18:40:08  haraldkipp
 * Timeouts increased and long time sleeps decreased for better performance.
 *
 * Revision 1.1  2006/01/05 16:30:49  haraldkipp
 * First check-in.
 *
 *
 * \endverbatim
 */


#define DEBUG_MSG_PREFIX "mmcard"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10


#include <mmcard.h>

//#if 0
/* Use for local debugging. */
//#define NUTDEBUG
#include <stdio.h>
//#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>
//#include <memdebug.h>

//#include <sys/heap.h>
//#include <sys/timer.h>
//#include <sys/event.h>
//#include <fs/dospart.h>
//#include <fs/fs.h>

//#include <dev/blockdev.h>
#include <mmcard.h>

#include <hal.h>

/*!
 * \addtogroup xgMmCard
 */
/*@{*/

#ifndef MMC_BLOCK_SIZE
/*!
 * \brief Block size.
 *
 * Block size in bytes. Do not change unless you are sure that both, 
 * the file system and the hardware support it.
 */
#define MMC_BLOCK_SIZE          512
#endif

#ifndef MMC_MAX_INIT_POLLS
/*!
 * \brief Card init timeout.
 *
 * Max. number of loops waiting for card's idle mode after initialization. 
 * An additional delay of 1 ms is added to each loop after one quarter of 
 * this value elapsed.
 */
#define MMC_MAX_INIT_POLLS      512
#endif

#ifndef MMC_MAX_RESET_POLLS
/*!
 * \brief Card reset timeout.
 *
 * Max. number of loops waiting for card's idle mode after resetting it.
 */
#define MMC_MAX_RESET_POLLS     255
#endif

#ifndef MMC_MAX_WRITE_POLLS
/*!
 * \brief Card write timeout.
 *
 * Max. number of loops waiting for card's idle mode after resetting it. 
 * An additional delay of 1 ms is added to each loop after 31/32 of this 
 * value elapsed.
 */
#define MMC_MAX_WRITE_POLLS     1024
#endif

#ifndef MMC_MAX_WRITE_RETRIES
/*!
 * \brief Card write retries.
 *
 * Max. number of retries while writing.
 */
#define MMC_MAX_WRITE_RETRIES   32
#endif

#ifndef MMC_MAX_READ_RETRIES
/*!
 * \brief Card read retries.
 *
 * Max. number of retries while reading.
 */
#define MMC_MAX_READ_RETRIES    8
#endif

#ifndef MMC_MAX_REG_POLLS
/*!
 * \brief Register read timeout.
 *
 * Max. number of loops while reading a card's register.
 */
#define MMC_MAX_REG_POLLS       512
#endif

#ifndef MMC_MAX_CMDACK_POLLS
/*!
 * \brief Command acknowledge timeout.
 *
 * Max. number of loops waiting for card's acknowledge of a command. 
 * An additional delay of 1 ms is added to each loop after three quarter 
 * of this value elapsed.
 */
#define MMC_MAX_CMDACK_POLLS    1024
#endif

#ifndef MMC_MAX_R1_POLLS
/*!
 * \brief R1 response timeout.
 *
 * Max. number of loops waiting for card's R1 response.
 */
#define MMC_MAX_R1_POLLS        1024
#endif

/*!
 * \brief Local multimedia card mount information.
 */
typedef struct _MMCFCB {
    /*! \brief Attached file system device. 
     */
    NUTDEVICE *fcb_fsdev;

    /*! \brief Partition table entry of the currently mounted partition.
     */
    //DOSPART fcb_part;

    /*! \brief Next block number to read.
     *
     * The file system driver will send a NUTBLKDEV_SEEK control command
     * to set this value before calling the read or the write routine.
     *
     * The number is partition relative.
     */
    uint32_t fcb_blknum;

    /*! \brief Internal block buffer.
     *
     * A file system driver may use this one or optionally provide it's 
     * own buffers.
     *
     * Minimal systems may share their external bus interface with
     * device I/O lines, in which case the buffer must be located
     * in internal memory.
     */
    uint8_t fcb_blkbuf[MMC_BLOCK_SIZE];
} MMCFCB;

/*
 * Several routines call NutSleep, which results in a context switch.
 * This mutual exclusion semaphore takes care, that multiple threads
 * do not interfere with each other.
 */
static hal_mutex_t mutex;

/*!
 * \brief Send command to multimedia card.
 *
 * \param ifc   Specifies the hardware interface.
 * \param cmd   Command code. See MMCMD_ macros.
 * \param param Optional command parameter.
 */
static void MmCardTxCmd(MMCIFC * ifc, uint8_t cmd, uint32_t param)
{
    unsigned int tmo = MMC_MAX_CMDACK_POLLS;
    uint8_t ch;

    /* Enable card select. */
    (*ifc->mmcifc_cs) (1);
    /*
     * Repeat sending nothing until we receive nothing. Actually
     * it should be sufficient to send a single 0xFF value, but
     * running a loop seems to fix certain kind of sync problems.
     */
    while ((ch = (*ifc->mmcifc_io) (0xFF)) != 0xFF) {
        if (--tmo == 0) {
#ifdef NUTDEBUG
            printf("[MMCmd%u Timeout %02X]\n", cmd, ch);
#endif
            break;
        }
        if (tmo < MMC_MAX_CMDACK_POLLS / 4) {
            NutSleep(1);
        }
    }
    /* Send command and parameter. */
    (*ifc->mmcifc_io) (MMCMD_HOST | cmd);
    (*ifc->mmcifc_io) ((uint8_t) (param >> 24));
    (*ifc->mmcifc_io) ((uint8_t) (param >> 16));
    (*ifc->mmcifc_io) ((uint8_t) (param >> 8));
    (*ifc->mmcifc_io) ((uint8_t) param);
    /*
     * We are running with CRC disabled. However, the reset command must
     * be send with a valid CRC. Fortunately this command is sent with a
     * fixed parameter value of zero, which results in a fixed CRC value
     */
    (*ifc->mmcifc_io) (MMCMD_RESET_CRC);
}

/*!
 * \brief Receive an R1 response token from the card.
 *
 * In SPI mode, the card sends an R1 response after every command except
 * after the SEND_STATUS and the READ_OCR commands.
 *
 * \param ifc Specifies the hardware interface.
 *
 * \return R1 response token or 0xFF on timeout.
 */
static uint8_t MmCardRxR1(MMCIFC * ifc)
{
    uint8_t rc;
    int i;

    for (i = 0; i < MMC_MAX_R1_POLLS; i++) {
        if ((rc = (*ifc->mmcifc_io) (0xFF)) != 0xFF) {
            break;
        }
    }
    return rc;
}

/*!
 * \brief Receive an R2 response token from the card.
 *
 * In SPI mode the card sends this token in response to the SEND_STATUS
 * command.
 *
 * \param ifc Specifies the hardware interface.
 *
 * \return R2 response token or 0xFFFF on timeout.
 */
static uint16_t MmCardRxR2(MMCIFC * ifc)
{
    uint16_t rc;

    rc = MmCardRxR1(ifc);
    rc <<= 8;
    rc += (*ifc->mmcifc_io) (0xFF);

    return rc;
}

/*!
 * \brief Receive an R3 response token from the card.
 *
 * In SPI mode the card sends this token in response to the READ_OCR
 * command.
 *
 * \param ifc Specifies the hardware interface.
 * \param ocr Points to a buffer which receives the OCR register contents.
 *
 * \return R1 response token or 0xFF on timeout.
 */
static uint8_t MmCardRxR3(MMCIFC * ifc, uint32_t * ocr)
{
    uint8_t rc;
    int i;

    /* The first byte is equal to the R1 response. */
    rc = MmCardRxR1(ifc);
    /* Receive the operating condition. */
    for (i = 0; i < 4; i++) {
        *ocr <<= 8;
        *ocr |= (*ifc->mmcifc_io) (0xFF);
    }
    return rc;
}

/*!
 * \brief Configure card for SPI mode.
 *
 * \param ifc Specifies the hardware interface.
 *
 * \return 0 on success, -1 otherwise.
 */
static int MmCardReset(MMCIFC * ifc)
{
    int i;
    uint8_t rsp;

    /*
     * Initialize the low level card interface.
     */
    if ((*ifc->mmcifc_in) ()) {
        return -1;
    }

    /*
     * 80 bits of ones with deactivated chip select will put the card 
     * in SPI mode.
     */
    (*ifc->mmcifc_cs) (0);
    for (i = 0; i < 10; i++) {
        (*ifc->mmcifc_io) (0xFF);
    }

    /*
     * Switch to idle state and wait until initialization is running
     * or idle state is reached.
     */
    for (i = 0; i < MMC_MAX_RESET_POLLS; i++) {
        MmCardTxCmd(ifc, MMCMD_GO_IDLE_STATE, 0);
        rsp = MmCardRxR1(ifc);
        (*ifc->mmcifc_cs) (0);
        if (rsp == MMR1_IDLE_STATE || rsp == MMR1_NOT_IDLE) {
            return 0;
        }
    }
    return -1;
}

/*!
 * \brief Initialize the multimedia card.
 *
 * This routine will put a newly powered up card into SPI mode.
 * It is called by MmCardMount().
 *
 * \param ifc Specifies the hardware interface.
 *
 * \return 0 on success, -1 otherwise.
 */
static int MmCardInit(MMCIFC * ifc)
{
    int i;
    uint8_t rsp;

    /*
     * Try to switch to SPI mode. Looks like a retry helps to fix
     * certain synchronization problems.
     */
    if (MmCardReset(ifc)) {
        if (MmCardReset(ifc)) {
#ifdef NUTDEBUG
            printf("[CardReset failed]");
#endif
            return -1;
        }
    }

    hal_mutex_init(&mutex,"mmcard");

    /*
     * Wait for a really long time until card is initialized
     * and enters idle state.
     */
    for (i = 0; i < MMC_MAX_INIT_POLLS; i++) {
        /*
         * In SPI mode SEND_OP_COND is a dummy, used to poll the card
         * for initialization finished. Thus, there are no parameters
         * and no operation condition data is sent back.
         */
        MmCardTxCmd(ifc, MMCMD_SEND_OP_COND, 0);
        rsp = MmCardRxR1(ifc);
        (*ifc->mmcifc_cs) (0);
        if (rsp == MMR1_IDLE_STATE) {
#ifdef NUTDEBUG
            printf("[CardIdle]");
#endif
            /* Initialize MMC access mutex semaphore. */
            //NutEventPost(&mutex);
            return 0;
        }
        if (i > MMC_MAX_INIT_POLLS / 4) {
            NutSleep(1);
        }
    }
#ifdef NUTDEBUG
    printf("[CardInit failed]");
#endif
    return -1;
}

/*!
 * \brief Read or verify a single block.
 *
 * \param ifc Specifies the hardware interface.
 * \param blk Block number to read or verify.
 * \param buf Data buffer. Receives the data or is verified against the
 *            data being read from the card.
 *
 * \return 0 on success, -1 otherwise.
 */
static int MmCardReadOrVerify(MMCIFC * ifc, uint32_t blk, uint8_t * buf, int vflg)
{
    int rc = -1;
    int retries = 64;
    int i;
    uint8_t rsp;

    /* Gain mutex access. */
    hal_mutex_lock(&mutex);

    while (retries--) {
        MmCardTxCmd(ifc, MMCMD_READ_SINGLE_BLOCK, blk << 9);
        if ((rsp = MmCardRxR1(ifc)) == 0x00) {
            if ((rsp = MmCardRxR1(ifc)) == 0xFE) {
                rc = 0;
                if (vflg) {
                    for (i = 0; i < MMC_BLOCK_SIZE; i++) {
                        if (*buf != (*ifc->mmcifc_io) (0xFF)) {
                            rc = -1;
                        }
                        buf++;
                    }
                } else {
                    for (i = 0; i < MMC_BLOCK_SIZE; i++) {
                        *buf = (*ifc->mmcifc_io) (0xFF);
                        buf++;
                    }
                }
                (*ifc->mmcifc_io) (0xff);
                (*ifc->mmcifc_io) (0xff);
                (*ifc->mmcifc_cs) (0);
                break;
            }
        }
        (*ifc->mmcifc_cs) (0);
    }

    /* Release mutex access. */
    hal_mutex_unlock(&mutex);

    return rc;
}

/*!
 * \brief Write a single block.
 *
 * \param ifc Specifies the hardware interface.
 * \param blk Block number to read or verify.
 * \param buf Pointer to a buffer which holds the data to write.
 *
 * \return 0 on success, -1 otherwise.
 */
static int MmCardWrite(MMCIFC * ifc, uint32_t blk, CONST uint8_t * buf)
{
    int rc = -1;
    int retries = MMC_MAX_WRITE_RETRIES;
    int tmo;
    int i;
    uint8_t rsp;

    /* Gain mutex access. */
    hal_mutex_lock(&mutex);

    while (retries--) {
        MmCardTxCmd(ifc, MMCMD_WRITE_BLOCK, blk << 9);
        if ((rsp = MmCardRxR1(ifc)) == 0x00) {
            (*ifc->mmcifc_io) (0xFF);
            (*ifc->mmcifc_io) (0xFE);
            for (i = 0; i < MMC_BLOCK_SIZE; i++) {
                (*ifc->mmcifc_io) (*buf);
                buf++;
            }
            // (*ifc->mmcifc_io)(0xFF);
            // (*ifc->mmcifc_io)(0xFF);
            if ((rsp = MmCardRxR1(ifc)) == 0xE5) {
                for (tmo = 0; tmo < MMC_MAX_WRITE_POLLS; tmo++) {
                    if ((*ifc->mmcifc_io) (0xFF) == 0xFF) {
                        break;
                    }
                    if (tmo > MMC_MAX_WRITE_POLLS - MMC_MAX_WRITE_POLLS / 32) {
                        NutSleep(1);
                    }
                }
                if (tmo) {
                    rc = 0;
                    break;
                }
#ifdef NUTDEBUG
                printf("[MMCWR Timeout]\n");
#endif
            }
        }
        (*ifc->mmcifc_cs) (0);
    }
    (*ifc->mmcifc_cs) (0);

    /* Release mutex access. */
    hal_mutex_unlock(&mutex);

    return rc;
}

#if MMC_FIO
/*!
 * \brief Read data blocks from a mounted partition.
 *
 * Applications should not call this function directly, but use the
 * stdio interface.
 *
 * \param nfp    Pointer to a ::NUTFILE structure, obtained by a previous 
 *               call to MmCardMount().
 * \param buffer Pointer to the data buffer to fill.
 * \param num    Maximum number of blocks to read. However, reading 
 *               multiple blocks is not yet supported by this driver.
 *
 * \return The number of blocks actually read. A return value of -1 
 *         indicates an error.
 */
int MmCardBlockRead(NUTFILE * nfp, void *buffer, int num)
{
    MMCFCB *fcb = (MMCFCB *) nfp->nf_fcb;
    uint32_t blk = fcb->fcb_blknum;
    NUTDEVICE *dev = (NUTDEVICE *) nfp->nf_dev;
    MMCIFC *ifc = (MMCIFC *) dev->dev_icb;

    if ((*ifc->mmcifc_cd) () != 1) {
        return -1;
    }
    if (buffer == 0) {
        buffer = fcb->fcb_blkbuf;
    }
    blk += fcb->fcb_part.part_sect_offs;

#ifdef MMC_VERIFY_AFTER
    {
        int i;
        /*
         * It would be much better to verify the checksum than to re-read
         * and verify the data block.
         */
        for (i = 0; i < MMC_MAX_READ_RETRIES; i++) {
            if (MmCardReadOrVerify(ifc, blk, buffer, 0) == 0) {
                if (MmCardReadOrVerify(ifc, blk, buffer, 1) == 0) {
                    return 1;
                }
            }
        }
    }
#else
    if (MmCardReadOrVerify(ifc, blk, buffer, 0) == 0) {
        return 1;
    }
#endif
    return -1;
}

/*!
 * \brief Write data blocks to a mounted partition.
 *
 * Applications should not call this function directly, but use the
 * stdio interface.
 *
 * \param nfp    Pointer to a \ref NUTFILE structure, obtained by a previous 
 *               call to MmCardMount().
 * \param buffer Pointer to the data to be written.
 * \param num    Maximum number of blocks to write. However, writing
 *               multiple blocks is not yet supported by this driver.
 *
 * \return The number of blocks written. A return value of -1 indicates an 
 *         error.
 */
int MmCardBlockWrite(NUTFILE * nfp, CONST void *buffer, int num)
{
    MMCFCB *fcb = (MMCFCB *) nfp->nf_fcb;
    uint32_t blk = fcb->fcb_blknum;
    NUTDEVICE *dev = (NUTDEVICE *) nfp->nf_dev;
    MMCIFC *ifc = (MMCIFC *) dev->dev_icb;

    if ((*ifc->mmcifc_cd) () != 1) {
        return -1;
    }
    if (buffer == 0) {
        buffer = fcb->fcb_blkbuf;
    }
    blk += fcb->fcb_part.part_sect_offs;

#ifdef MMC_VERIFY_AFTER
    {
        int i;

        for (i = 0; i < MMC_MAX_READ_RETRIES; i++) {
            if (MmCardWrite(ifc, blk, buffer) == 0) {
                if (MmCardReadOrVerify(ifc, blk, (void *) buffer, 1) == 0) {
                    return 1;
                }
                if (MmCardReadOrVerify(ifc, blk, (void *) buffer, 1) == 0) {
                    return 1;
                }
            }
        }
    }
#else
    if (MmCardWrite(ifc, blk, buffer) == 0) {
        return 1;
    }
#endif
    return -1;
}


/*!
 * \brief Mount a partition.
 *
 * Nut/OS doesn't provide specific routines for mounting. Instead routines
 * for opening files are used.
 *
 * Applications should not directly call this function, but use the high
 * level stdio routines for opening a file.
 *
 * \param dev  Pointer to the MMC device.
 * \param name Partition number followed by a slash followed by a name
 *             of the file system device. Both items are optional. If no 
 *             file system driver name is given, the first file system
 *             driver found in the list of registered devices will be 
 *             used. If no partition number is specified or if partition
 *             zero is given, the first active primary partition will be 
 *             used.
 * \param mode Opening mode. Currently ignored, but 
 *             \code _O_RDWR | _O_BINARY \endcode should be used for
 *             compatibility with future enhancements.
 * \param acc  File attributes, ignored.
 *
 * \return Pointer to a newly created file pointer to the mounted
 *         partition or NUTFILE_EOF in case of any error.
 */
NUTFILE *MmCardMount(NUTDEVICE * dev, CONST char *name, int mode, int acc)
{
    int partno = 0;
    int i;
    NUTDEVICE *fsdev;
    NUTFILE *nfp;
    MMCFCB *fcb;
    DOSPART *part;
    MMCIFC *ifc = (MMCIFC *) dev->dev_icb;
    FSCP_VOL_MOUNT mparm;

    /* Return an error if no card is detected. */
    if ((*ifc->mmcifc_cd) () == 0) {
        errno = ENODEV;
        return NUTFILE_EOF;
    }

    /* Set the card in SPI mode. */
    if (MmCardInit(ifc)) {
        errno = ENODEV;
        return NUTFILE_EOF;
    }

    /* Parse the name for a partition number and a file system driver. */
    if (*name) {
        partno = atoi(name);
        do {
            name++;
        } while (*name && *name != '/');
        if (*name == '/') {
            name++;
        }
    }

    /*
     * Check the list of registered devices for the given name of the
     * files system driver. If none has been specified, get the first
     * file system driver in the list. Hopefully the application
     * registered one only.
     */
    for (fsdev = nutDeviceList; fsdev; fsdev = fsdev->dev_next) {
        if (*name == 0) {
            if (fsdev->dev_type == IFTYP_FS) {
                break;
            }
        } else if (strcmp(fsdev->dev_name, name) == 0) {
            break;
        }
    }

    if (fsdev == 0) {
#ifdef NUTDEBUG
        printf("[No FSDriver]");
#endif
        errno = ENODEV;
        return NUTFILE_EOF;
    }

    if ((fcb = calloc(1, sizeof(MMCFCB))) == 0) {
        errno = ENOMEM;
        return NUTFILE_EOF;
    }
    fcb->fcb_fsdev = fsdev;

    /* Read MBR. */
    if (MmCardReadOrVerify(ifc, 0, fcb->fcb_blkbuf, 0)) {
        free(fcb);
        return NUTFILE_EOF;
    }
    /* Check for the cookie at the end of this sector. */
	if (fcb->fcb_blkbuf[DOSPART_MAGICPOS] != 0x55 || fcb->fcb_blkbuf[DOSPART_MAGICPOS + 1] != 0xAA) {
        free(fcb);
        return NUTFILE_EOF;
	}
    /* Check for the partition table. */
	if(fcb->fcb_blkbuf[DOSPART_TYPEPOS] == 'F' && 
       fcb->fcb_blkbuf[DOSPART_TYPEPOS + 1] == 'A' &&
       fcb->fcb_blkbuf[DOSPART_TYPEPOS + 2] == 'T') {
        /* No partition table. Assume FAT12 and 32MB size. */
        fcb->fcb_part.part_type = PTYPE_FAT12;
        fcb->fcb_part.part_sect_offs = 0;
        fcb->fcb_part.part_sects = 65536; /* How to find out? */
	}
    else {
        /* Read partition table. */
        part = (DOSPART *) & fcb->fcb_blkbuf[DOSPART_SECTORPOS];
        for (i = 1; i <= 4; i++) {
            if (partno) {
                if (i == partno) {
                    /* Found specified partition number. */
                    fcb->fcb_part = *part;
                    break;
                }
            } else if (part->part_state & 0x80) {
                /* Located first active partition. */
                fcb->fcb_part = *part;
                break;
            }
            part++;
        }

        if (fcb->fcb_part.part_type == PTYPE_EMPTY) {
            free(fcb);
            return NUTFILE_EOF;
        }
    }

    if ((nfp = NutHeapAlloc(sizeof(NUTFILE))) == 0) {
        free(fcb);
        errno = ENOMEM;
        return NUTFILE_EOF;
    }
    nfp->nf_next = 0;
    nfp->nf_dev = dev;
    nfp->nf_fcb = fcb;

    /*
     * Mount the file system volume.
     */
    mparm.fscp_bmnt = nfp;
    mparm.fscp_part_type = fcb->fcb_part.part_type;
    if (fsdev->dev_ioctl(fsdev, FS_VOL_MOUNT, &mparm)) {
        MmCardUnmount(nfp);
        return NUTFILE_EOF;
    }
    return nfp;
}

/*!
 * \brief Unmount a previously mounted partition.
 *
 * Applications should not directly call this function, but use the high
 * level stdio routines for closing a previously opened file.
 *
 * \return 0 on success, -1 otherwise.
 */
int MmCardUnmount(NUTFILE * nfp)
{
    int rc = -1;

    if (nfp) {
        MMCFCB *fcb = (MMCFCB *) nfp->nf_fcb;

        if (fcb) {
            NUTDEVICE *dev = (NUTDEVICE *) nfp->nf_dev;
            MMCIFC *ifc = (MMCIFC *) dev->dev_icb;

            if ((*ifc->mmcifc_cd) () == 1) {
                rc = fcb->fcb_fsdev->dev_ioctl(fcb->fcb_fsdev, FS_VOL_UNMOUNT, NULL);
            }
            free(fcb);
        }
        free(nfp);
    }
    return rc;
}
#endif


/*!
 * \brief Retrieve contents of a card register.
 *
 * \param ifc Specifies the hardware interface.
 * \param cmd This command is sent to the card to retrieve the contents
 *            of a specific register.
 * \param rbp Pointer to the buffer that receives the register contents.
 * \param siz Size of the specified register.
 *
 * \return 0 on success, -1 otherwise.
 */
static int MmCardGetReg(MMCIFC * ifc, uint8_t cmd, uint8_t * rbp, int siz)
{
    int rc = -1;
    int retries = MMC_MAX_REG_POLLS;
    int i;

    /* Gain mutex access. */
    hal_mutex_lock(&mutex);

    while (retries--) {
        /* Send the command to the card. This will select the card. */
        MmCardTxCmd(ifc, cmd, 0);
        /* Wait for OK response from the card. */
        if (MmCardRxR1(ifc) == 0x00) {
            /* Wait for data from the card. */
            if (MmCardRxR1(ifc) == 0xFE) {
                for (i = 0; i < siz; i++) {
                    *rbp++ = (*ifc->mmcifc_io) (0xFF);
                }
                /* Ignore the CRC. */
                (*ifc->mmcifc_io) (0xFF);
                (*ifc->mmcifc_io) (0xFF);
                /* De-activate card selection. */
                (*ifc->mmcifc_cs) (0);
                /* Return a positive result. */
                rc = 0;
                break;
            }
        }
        /* De-activate card selection. */
        (*ifc->mmcifc_cs) (0);
    }

    /* Release mutex access. */
    hal_mutex_unlock(&mutex);

    return rc;
}

/*!
 * \brief Perform MMC control functions.
 *
 * This function is called by the ioctl() function of the C runtime
 * library. Applications should not directly call this function.
 *
 * \todo Card change detection should verify the serial card number.
 *
 * \param dev  Identifies the device that receives the device-control
 *             function.
 * \param req  Requested control function. May be set to one of the
 *             following constants:
 *             - \ref NUTBLKDEV_MEDIACHANGE
 *             - \ref NUTBLKDEV_INFO
 *             - \ref NUTBLKDEV_SEEK
 *             - \ref MMCARD_GETCID
 *             - \ref MMCARD_GETCSD
 *
 * \param conf Points to a buffer that contains any data required for
 *             the given control function or receives data from that
 *             function.
 * \return 0 on success, -1 otherwise.
 */
int MmCardIOCtl(NUTDEVICE * dev, int req, void *conf)
{
#if 1
    (void) dev;
    (void) req;
    (void) conf;
    return EFAULT;
#else
    int rc = 0;
    MMCIFC *ifc = (MMCIFC *) dev->dev_icb;

    switch (req) {
    case NUTBLKDEV_MEDIAAVAIL:
        {
            int *flg = (int *) conf;
            *flg = (*ifc->mmcifc_cd) ();
        }
        break;
    case NUTBLKDEV_MEDIACHANGE:
        {
            int *flg = (int *) conf;
            if ((*ifc->mmcifc_cd) () != 1) {
                *flg = 1;
            } else {
                *flg = 0;
            }
        }
        break;
    case NUTBLKDEV_INFO:
        {
            BLKPAR_INFO *par = (BLKPAR_INFO *) conf;
            MMCFCB *fcb = (MMCFCB *) par->par_nfp->nf_fcb;

            par->par_nblks = fcb->fcb_part.part_sects;
            par->par_blksz = MMC_BLOCK_SIZE;
            par->par_blkbp = fcb->fcb_blkbuf;
        }
        break;
    case NUTBLKDEV_SEEK:
        {
            BLKPAR_SEEK *par = (BLKPAR_SEEK *) conf;
            MMCFCB *fcb = (MMCFCB *) par->par_nfp->nf_fcb;

            fcb->fcb_blknum = par->par_blknum;
        }
        break;
    case MMCARD_GETSTATUS:
        {
            uint16_t *s = (uint16_t *) conf;

            /* Gain mutex access. */
            hal_mutex_lock(&mutex, 0);

            MmCardTxCmd(ifc, MMCMD_SEND_STATUS, 0);
            *s = MmCardRxR2(ifc);

            /* Release mutex access. */
            hal_mutex_unlock(&mutex);
        }
        break;
    case MMCARD_GETOCR:
        /* Gain mutex access. */
        hal_mutex_lock(&mutex, 0);

        MmCardTxCmd(ifc, MMCMD_READ_OCR, 0);
        if (MmCardRxR3(ifc, (uint32_t *) conf) != MMR1_IDLE_STATE) {
            rc = -1;
        }

        /* Release mutex access. */
        hal_mutex_unlock(&mutex);
        break;
    case MMCARD_GETCID:
        rc = MmCardGetReg(ifc, MMCMD_SEND_CID, (uint8_t *) conf, sizeof(MMC_CID));
        break;
    case MMCARD_GETCSD:
        rc = MmCardGetReg(ifc, MMCMD_SEND_CSD, (uint8_t *) conf, sizeof(MMC_CSD));
        break;
    default:
        rc = -1;
        break;
    }
    return rc;
#endif
}

/*!
 * \brief Initialize high level MMC driver.
 *
 * Applications should not directly call this function. It is 
 * automatically executed during during device registration by 
 * NutRegisterDevice().
 *
 * \param dev  Identifies the device to initialize.
 *
 * \return Always zero.
 * /
int MmCardDevInit(NUTDEVICE * dev)
{
    (void) dev;
    return 0;
} */



//errno_t MmCardStart(NUTDEVICE * dev, MMCIFC *ifc )
errno_t MmCardStart(MMCIFC *ifc)
{
    /* Return an error if no card is detected. */
    if ((*ifc->mmcifc_cd)() == 0)
        return ENODEV;

    /* Set the card in SPI mode. */
    if (MmCardInit(ifc))
        return ENODEV;

    return 0;
}

/*

memory capacity = BLOCKNR * BLOCK_LEN
where
BLOCKNR = (C_SIZE+1) * MULT
MULT = 2C_SIZE_MULT+2 (C_SIZE_MULT < 8)
BLOCK_LEN = 2READ_BL_LEN, (READ_BL_LEN < 12)
Therefore, the maximal capacity which can be coded is 4096*512*2048 = 4 GBytes. Example: A 4 MByte card with
BLOCK_LEN = 512 can be coded by C_SIZE_MULT = 0 and C_SIZE = 2047.

*/

static unsigned long MmCardGetSize(MMCIFC *ifc)
{
    MMC_CSD csd;

    int rc = MmCardGetReg(ifc, MMCMD_SEND_CSD, (void *) &csd, sizeof(MMC_CSD));
    if( rc )
    {
        SHOW_FLOW( 1, "can't MmCardGetReg rc=read %d", rc );
        return 0;
    }

    unsigned c_size = 0;

    c_size |= (csd.mmcsd_rfld[0] & 0x03u) << 10;
    c_size |= (csd.mmcsd_rfld[1] & 0xFFu) << 2;
    c_size |= (csd.mmcsd_rfld[2] >> 6) & 0x03u;

    unsigned mult = 0;

    mult |= (csd.mmcsd_rfld[3] & 0x03u) << 1;
    mult |= (csd.mmcsd_rfld[4] >> 7) & 0x01u;

    unsigned long sz = (c_size+1)*(unsigned long)mult;

    //SHOW_FLOW( 1, "blk sz? %d", csd.mmcsd_ccc_bl[0] & 0xF );
    //SHOW_FLOW( 1, "blk sz? %d", (csd.mmcsd_ccc_bl[0] >> 4) & 0xF );
    //SHOW_FLOW( 1, "blk sz? %d", csd.mmcsd_ccc_bl[1] & 0xF );
    SHOW_FLOW( 1, "blk sz? %d", (csd.mmcsd_ccc_bl[1] >> 4) & 0xF );

    SHOW_FLOW( 1, "csize %d mult %d sz = %ld", c_size, mult, sz );

    return sz;
}



// -----------------------------------------------------------------------

#include <disk.h>

#define DEV_NAME "mmcard"


// TODO actually this is syncronous interface, need disk_q to make it async.
static errno_t mmcard_AsyncIo( struct phantom_disk_partition *part, pager_io_request *rq )
{
    phantom_device_t *dev = part->specific;
    MMCIFC *llio = dev->drv_private;

    rq->flag_ioerror = 0;
    rq->rc = 0;

    size_t size = rq->nSect * part->block_size;
    off_t shift = rq->blockNo * part->block_size;

    /*
    if( size+shift > dev->iomemsize )
    {
        rq->flag_ioerror = 1;
        rq->rc = EIO;
        pager_io_request_done( rq );
        return EIO;
    }

    if(rq->flag_pageout)
    {
        rq->flag_ioerror = 1;
        rq->rc = EIO;
    }
    else
    {
        // read
        memcpy_v2p( rq->phys_page, (void *)dev->iomem+shift, size );
    }
    */

    u_int8_t buf[512];

    physaddr_t pp = rq->phys_page;

    if(rq->flag_pageout)
    {
        //int rc = MmCardReadOrVerify( llio, shift/512, buf, 0);

        rq->flag_ioerror = 1;
        rq->rc = EIO;
    }
    else
    {
        while( size >= 512 )
        {
            int rc = MmCardReadOrVerify( llio, shift/512, buf, 0);

            if( rc )
            {
                rq->flag_ioerror = 1;
                rq->rc = EIO;
                break;
            }

            // read
            memcpy_v2p( pp, buf, 512 );

            pp += 512;
            size -= 512;
            shift += 512;
        }
    }



    pager_io_request_done( rq );
    return rq->rc;
}



phantom_disk_partition_t *phantom_create_mmcard_partition_struct( phantom_device_t *dev, long size, int unit )
{
    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size );

    ret->asyncIo = mmcard_AsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;

    ret->specific = dev;
    strlcpy( ret->name, "MMCARD0", sizeof(ret->name) );

    ret->name[4] += unit;

    return ret;
}


static void mmcard_connect( phantom_device_t *dev, int nSect )
{
    phantom_disk_partition_t *part = phantom_create_mmcard_partition_struct( dev, nSect, 0 );
    if(part == 0)
    {
        SHOW_ERROR0( 0, "Failed to create whole disk partition" );
        return;
    }
#if 1
    errno_t err = phantom_register_disk_drive(part);
    if(err)
    {
        SHOW_ERROR( 0, "Disk %p err %d", dev, err );
        return;
    }
#endif
}


static int mmcard_read(phantom_device_t *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    //mmcard_t *es = dev->drv_private;

    return -1;
}

static int mmcard_write(phantom_device_t *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    //mmcard_t *es = dev->drv_private;

    return -1;
}




static int seq_number = 0;
errno_t driver_mmcard_init( MMCIFC *llio )
{
    errno_t rc = MmCardStart( llio );
    if( rc ) return rc;

    unsigned long sz = MmCardGetSize(llio);

    SHOW_FLOW( 1, "Start " DEV_NAME " sz=%ld", sz );

    phantom_device_t * dev = calloc(sizeof(phantom_device_t),1);

    dev->drv_private = llio;

    dev->name = DEV_NAME;
    dev->seq_number = seq_number++;

    //dev->dops.start = mmcard_start;
    //dev->dops.stop  = mmcard_stop;
    dev->dops.read  = mmcard_read;
    dev->dops.write = mmcard_write;
    //dev->dops.ioctl = mmcard_ioctl;

#warning predefined 64Gb
    long nsect = 64L*1024*1024*1024/512;

    mmcard_connect( dev, nsect );

    return 0;

//free1:
    //free(es);

//free:
    free(dev);
    return ENXIO;
}











/*@}*/
#endif
