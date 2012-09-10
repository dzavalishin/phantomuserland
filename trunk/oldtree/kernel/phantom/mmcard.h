#ifndef _DEV_MMCARD_H_
#define _DEV_MMCARD_H_

/*
 * Copyright (C) 2005 by egnite Software GmbH. All rights reserved.
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
 * \file dev/mmcard.h
 * \brief Header file for basic multimedia card driver.
 *
 * \verbatim
 *
 * $Log$
 * Revision 1.5  2008/10/03 11:28:58  haraldkipp
 * Corrected and modified initialization of MultiMedia and SD Cards.
 *
 * Revision 1.4  2008/08/11 06:59:59  haraldkipp
 * BSD types replaced by stdint types (feature request #1282721).
 *
 * Revision 1.3  2006/10/08 16:48:09  haraldkipp
 * Documentation fixed
 *
 * Revision 1.2  2006/09/05 12:34:55  haraldkipp
 * Added commands for native MMC interface.
 *
 * Revision 1.1  2006/01/05 16:32:10  haraldkipp
 * First check-in.
 *
 *
 * \endverbatim
 */

#include <compat/nutos.h>
//#include <shorttypes.h>
#include <kernel/device.h>

//#include <sys/device.h>
//#include <sys/file.h>

/*!
 * \addtogroup xgMmCard
 */
/*@{*/

#define NUTMC_SF_CD     0x01
#define NUTMC_SF_WP     0x02

#define NUTMC_IND_OFF   0
#define NUTMC_IND_READ  1
#define NUTMC_IND_WRITE 2
#define NUTMC_IND_ERROR 4

#if 0
/*!
 * \brief Memory card support structure.
 */
typedef struct _MEMCARDSUPP {
    /*! \brief Status change flag. */
    uint8_t mcs_cf;
    /*! \brief Socket status. */
    uint8_t mcs_sf;
    /*! \brief Status reset. */
    int (*mcs_reset) (NUTDEVICE *);
    /*! \brief Set activity indicator. */
    void (*mcs_act) (int);
    /*! \brief Power up or down. */
    int (*mcs_power) (int);
} MEMCARDSUPP;
#endif

/*!
 * \brief Low level access information structure.
 */
typedef struct _MMCIFC {
    /*! Initialize the card. */
    int (*mmcifc_in) (void);
    /*! Read received byte and transmit a new one. */
     uint8_t(*mmcifc_io) (uint8_t);
    /*! Select or deselect the card. */
    int (*mmcifc_cs) (int);
    /*! Query card detect. */
    int (*mmcifc_cd) (void);
    /*! Query write protect. */
    int (*mmcifc_wp) (void);
} MMCIFC;

#define MMCMD_HOST                      0x40
#define MMCMD_RESET_CRC                 0x95

/*! \brief Reset card to idle state. 
 *
 * In idle state the card will not accept any other commands than
 * MMCMD_SEND_OP_COND or MMCMD_READ_OCR.
 */
#define MMCMD_GO_IDLE_STATE             0

/*! \brief Activate card's initialization process. */
#define MMCMD_SEND_OP_COND              1

/*! \brief Send operation condition register.
 *
 * Queries the operation condition register content from all cards, which
 * are in idle state.
 */
#define MMCMD_ALL_SEND_CID              2

/*! \brief Assign relative card address. */
#define MMCMD_SEND_RELATIVE_ADDR        3

/*! \brief Assign relative card address. */
#define MMCMD_SELECT_CARD               7

/*! \brief Query card's extended CSD. */
#define MMCMD_SEND_EXTCSD               8

/*! \brief Query card's CSD. */
#define MMCMD_SEND_CSD                  9

/*! \brief Query card's CID. */
#define MMCMD_SEND_CID                  10

/*! \brief Stop multiple block transmission. */
#define MMCMD_STOP_TRANSMISSION         12

/*! \brief Query card's status register. */
#define MMCMD_SEND_STATUS               13

/*! \brief Select block length for following read/write commands. */
#define MMCMD_SET_BLOCKLEN              16

/*! \brief Initiate single block read. */
#define MMCMD_READ_SINGLE_BLOCK         17

/*! \brief Initiate continuous block read. */
#define MMCMD_READ_MULTIPLE_BLOCK       18

/*! \brief Initiate single block write. */
#define MMCMD_WRITE_BLOCK               24

/*! \brief Initiate continuous block write. */
#define MMCMD_WRITE_MULTIPLE_BLOCK      25

/*! \brief Initiate programming of programmable CSD bits. */
#define MMCMD_PROGRAM_CSD               27

/*! \brief Enable card's optional write protection. */
#define MMCMD_SET_WRITE_PROTECT         28

/*! \brief Disable card's write protection. */
#define MMCMD_CLR_WRITE_PROTECT         29

/*! \brief Query card's write protect status. */
#define MMCMD_SEND_WRITE_PROTECT        30

/*! \brief Set address of the first erase group. */
#define MMCMD_TAG_ERASE_GROUP_START     35

/*! \brief Set address of the last erase group. */
#define MMCMD_TAG_ERASE_GROUP_END       36

/*! \brief Erase previously selected sectors. */
#define MMCMD_ERASE                     38

/*! \brief Activate SD card's initialization process. */
#define MMCMD_SEND_APP_OP_COND          41

/*! \brief Set/clear password or lock/unlock the card. */
#define MMCMD_LOCK_UNLOCK               42

/*! \brief Application command follows. */
#define MMCMD_SEND_APP_CMD              55

/*! \brief Query card's operating condition register. */
#define MMCMD_READ_OCR                  58

/*! \brief Enable or disable CRC mode. 
 *
 * In SPI mode CRC is disabled by default.
 */
#define MMCMD_CRC_ON_OFF                59

/*! \brief Card is idle. */
#define MMR1_IDLE_STATE         0x00
/*! \brief Card is busy. */
#define MMR1_NOT_IDLE           0x01
/*! \brief Erase sequence was cleared before execution. */
#define MMR1_ERASE_RESET        0x02
/*! \brief Illegal command code detected. */
#define MMR1_ILLEGAL_COMMAND    0x04
/*! \brief Bad command CRC detected. */
#define MMR1_COM_CRC_ERROR      0x08
/*! \brief Bad erase sequence. */
#define MMR1_ERASE_SEQ_ERROR    0x10
/*! \brief Misaligned address did not match block length. */
#define MMR1_ADDRESS_ERROR      0x20
/*! \brief Command parameter is out of range. */
#define MMR1_PARAMETER_ERROR    0x40


/*! \brief Card is locked. */
#define MMR2_CARD_LOCKED        0x01
/*! \brief Erasing write protected sector or password error. */
#define MMR2_WP_ERASE_SKIP      0x02
/*! \brief General or unknown error occured. */
#define MMR2_ERROR              0x04
/*! \brief Internal card controller error. */
#define MMR2_CC_ERROR           0x08
/*! \brief Bad internal ECC. */
#define MMR2_ECC_FAILED         0x10
/*! \brief Failed to write to protected block. */
#define MMR2_WP_VIOLATION       0x20
/*! \brief Invalid erase parameter. */
#define MMR2_ERASE_PARAMETER    0x40
/*! \brief Command parameter is out of range. */
#define MMR2_OUT_OF_RANGE       0x80


#define MMCSR_OUT_OF_RANGE      0x80000000
#define MMCSR_ADDRESS_ERROR     0x40000000
#define MMCSR_BLOCK_LEN_ERROR   0x20000000
#define MMCSR_ERASE_SEQ_ERROR   0x10000000
#define MMCSR_ERASE_PARAM       0x08000000
#define MMCSR_WP_VIOLATION      0x04000000
#define MMCSR_COM_CRC_ERROR     0x00800000
#define MMCSR_ILLEGAL_COMMAND   0x00400000
#define MMCSR_ERROR             0x00080000
#define MMCSR_CIDCSD_OVERWRITE  0x00010000
#define MMCSR_WP_ERASE_SKIP     0x00008000
#define MMCSR_CARD_ECC_DISABLED 0x00004000
#define MMCSR_ERASE_RESET       0x00002000
#define MMCSR_STATE_MASK        0x00001E00
#define MMCSR_READY_FOR_DATA    0x00000100

#define MMCSR_IS_IDLE           0x00000000
#define MMCSR_IS_READY          0x00000200
#define MMCSR_IS_IDENT          0x00000400
#define MMCSR_IS_STBY           0x00000600
#define MMCSR_IS_TRAN           0x00000800
#define MMCSR_IS_DATA           0x00000A00
#define MMCSR_IS_RCV            0x00000C00
#define MMCSR_IS_PRG            0x00000E00
#define MMCSR_IS_DIS            0x00001000

#define MMDR_ACCEPTED
#define MMDR_CRC_ERROR
#define MMDR_WRITE_ERROR

#define MMCERR_TIMEOUT      0x00000001

#define MMCOP_NBUSY         0x80000000

/*! \brief Number of bytes in the CID register. */
#define MMCARD_CIDR_SIZE                16

/*! \brief Number of bytes in the CSD register. */
#define MMCARD_CSDR_SIZE                16

/*! \brief Number of bytes in the operating condition register. */
#define MMCARD_OCR_SIZE                 32

/*!
 * \name Voltage Ranges
 */
/*@{*/
#define MMCARD_165_195V     0x00000080
#define MMCARD_20_21V       0x00000100
#define MMCARD_21_22V       0x00000200
#define MMCARD_22_23V       0x00000400
#define MMCARD_23_24V       0x00000800
#define MMCARD_24_25V       0x00001000
#define MMCARD_25_26V       0x00002000
#define MMCARD_26_27V       0x00004000
#define MMCARD_27_28V       0x00008000
#define MMCARD_28_29V       0x00010000
#define MMCARD_29_30V       0x00020000
#define MMCARD_30_31V       0x00040000
#define MMCARD_31_32V       0x00080000
#define MMCARD_32_33V       0x00100000
#define MMCARD_33_34V       0x00200000
#define MMCARD_34_35V       0x00400000
#define MMCARD_35_36V       0x00800000
/*@}*/

/*!
 * \name Control Codes
 */
/*@{*/

/*! \brief Retrieve card status. */
#define MMCARD_GETSTATUS    0x2001
/*! \brief Retrieve operation condition register. */
#define MMCARD_GETOCR       0x2002
/*! \brief Retrieve card identification. */
#define MMCARD_GETCID       0x2003
/*! \brief Retrieve card specific data. */
#define MMCARD_GETCSD       0x2004
/*! \brief Retrieve extended card specific data. */
#define MMCARD_GETEXTCSD    0x2005

/*@}*/

/*!
 * \brief Multimedia card identification register.
 */
typedef struct __attribute__ ((packed)) _MMC_CID {
    /*! \brief Manufacturer identifier. */
    uint8_t mmcid_mid;
    /*! \brief OEM/Application identifier. */
    uint16_t mmcid_oid;
    /*! \brief Product name. */
    uint8_t mmcid_pnm[6];
    /*! \brief Product revision. */
    uint8_t mmcid_rev;
    /*! \brief Serial number. */
    uint32_t mmcid_psn;
    /*! \brief Manufacturing date code. */
    uint8_t mmcid_mdt;
    /*! \brief CRC7 checksum. */
    uint8_t mmcid_crc;
} MMC_CID;

/*!
 * \brief Multimedia card identification register.
 */
typedef struct __attribute__ ((packed)) _MMC_CSD {
    /*! \brief Card specification. */
    uint8_t mmcsd_spec;
    /*! \brief Data read access time. */
    uint8_t mmcsd_taac;
    /*! \brief Data read access time 2. */
    uint8_t mmcsd_nsac;
    /*! \brief Maximum data transfer rate. */
    uint8_t mmcsd_speed;
    /*! \brief Card command classes and max. read block length. */
    uint8_t mmcsd_ccc_bl[2];
    /*! \brief Read-only fields. 
     *
     * - [0] 0..1 Device size bits 10..11.
     * - [0] 2..3 Reserved.
     * - [0] 4    DSR implemented.
     * - [0] 5    Read block misalignment.
     * - [0] 6    Write block misalignment.
     * - [0] 7    Partial blocks for read allowed.
     * - [1] 0..7 Device size bits 2..9.
     * - [2] 0..2 Max. read current at VDD max.
     * - [2] 3..5 Max. read current at VDD min.
     * - [2] 6..7 Device size bits 0..1.
     * - [3] 0..1 Device size multiplier bits 1..2.
     * - [3] 2..4 Max. write current at VDD max.
     * - [3] 5..7 Max. write current at VDD min.
     * - [4] 0..1 Erase group size multiplier bits 3..4.
     * - [4] 2..6 Erase group size.
     * - [4] 7    Device size multiplier bit 0.
     * - [5] 0..4 Write protect group size.
     * - [5] 5..7 Erase group size multiplier bits 0..2.
     * - [6] 0..1 Max. write data block length bits 2..3.
     * - [6] 2..4 Read to write speed factor.
     * - [6] 5..6 Reserved.
     * - [6] 7    Write protect group enable.
     * - [7] 0    Content protection application.
     * - [7] 1..4 Reserved.
     * - [7] 5    Partial blocks for write allowed.
     * - [7] 6..7 Max. write data block length bits 0..1.
     */
    uint8_t mmcsd_rfld[8];
    /*! \brief Programmable field. */
    uint8_t mmcsd_pfld;
    /*! \brief Checksum. */
    uint8_t mmcsd_crc;
} MMC_CSD;

/*@}*/

#define MMC_FIO 0

__BEGIN_DECLS
/* Prototypes */

#if MMC_FIO
extern NUTFILE *MmCardMount(NUTDEVICE * dev, CONST char *name, int mode, int acc);
extern int MmCardUnmount(NUTFILE * nfp);

extern int MmCardBlockRead(NUTFILE * nfp, void *buffer, int len);
extern int MmCardBlockWrite(NUTFILE * nfp, CONST void *buffer, int len);

extern NUTFILE *SpiMmcMount(NUTDEVICE * dev, CONST char *name, int mode, int acc);
extern int SpiMmcUnmount(NUTFILE * nfp);

extern int SpiMmcBlockRead(NUTFILE * nfp, void *buffer, int num);
extern int SpiMmcBlockWrite(NUTFILE * nfp, CONST void *buffer, int num);
#endif

extern int MmCardDevInit(NUTDEVICE * dev);
extern int MmCardIOCtl(NUTDEVICE * dev, int req, void *conf);

extern int SpiMmcInit(NUTDEVICE * dev);
extern int SpiMmcIOCtl(NUTDEVICE * dev, int req, void *conf);

__END_DECLS
/* End of prototypes */
#endif
