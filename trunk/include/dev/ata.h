/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Common ATA defs.
 *
**/

#ifndef ATA_H
#define ATA_H

#include <sys/types.h>

#define ATA_CMD_IDENT	0xEC	/* Identify Device		*/

#define ATA_CMD_RD_DMA	0xC8	/* Read DMA (with retries)	*/
#define ATA_CMD_RD_DMAN	0xC9	/* Read DMS ( no  retries)	*/
#define ATA_CMD_WR_DMA	0xCA	/* Write DMA (with retries)	*/
#define ATA_CMD_WR_DMAN	0xCB	/* Write DMA ( no  retires)	*/

#define ATA_CMD_READ_DMA	0xC8	/* Read DMA (with retries)	*/
#define ATA_CMD_READ_DMA_ONCE	0xC9	/* Read DMS ( no  retries)	*/
#define ATA_CMD_WRITE_DMA	0xCA	/* Write DMA (with retries)	*/
#define ATA_CMD_WRITE_DMA_ONCE	0xCB	/* Write DMA ( no  retires)	*/


#define ATA_CMD_READ_DMA_EXT                 0x25
#define ATA_CMD_WRITE_DMA_EXT                0x35

#define ATA_CMD_READ_FPDMA_QUEUED            0x60
#define ATA_CMD_WRITE_FPDMA_QUEUED           0x61


// Data set management
#define ATA_CMD_DSM                          0x06

/* feature values for Data Set Management */
#define DSM_TRIM                        0x01

#if 0
/*
 * structure returned by ATA_CMD_IDENT, as per ANSI ATA2 rev.2f spec
 */
typedef struct hd_driveid {
    unsigned short	config;		/* lots of obsolete bit flags */
    unsigned short	cyls;		/* "physical" cyls */
    unsigned short	reserved2;	/* reserved (word 2) */
    unsigned short	heads;		/* "physical" heads */
    unsigned short	track_bytes;	/* unformatted bytes per track */
    unsigned short	sector_bytes;	/* unformatted bytes per sector */
    unsigned short	sectors;	/* "physical" sectors per track */
    unsigned short	vendor0;	/* vendor unique */
    unsigned short	vendor1;	/* vendor unique */
    unsigned short	vendor2;	/* vendor unique */
    unsigned char	serial_no[20];	/* 0 = not_specified */
    unsigned short	buf_type;
    unsigned short	buf_size;	/* 512 byte increments; 0 = not_specified */
    unsigned short	ecc_bytes;	/* for r/w long cmds; 0 = not_specified */
    unsigned char	fw_rev[8];	/* 0 = not_specified */
    unsigned char	model[40];	/* 0 = not_specified */
    unsigned char	max_multsect;	/* 0=not_implemented */
    unsigned char	vendor3;	/* vendor unique */
    unsigned short	dword_io;	/* 0=not_implemented; 1=implemented */
    unsigned char	vendor4;	/* vendor unique */
    unsigned char	capability;	/* bits 0:DMA 1:LBA 2:IORDYsw 3:IORDYsup*/
    unsigned short	reserved50;	/* reserved (word 50) */
    unsigned char	vendor5;	/* vendor unique */
    unsigned char	tPIO;		/* 0=slow, 1=medium, 2=fast */
    unsigned char	vendor6;	/* vendor unique */
    unsigned char	tDMA;		/* 0=slow, 1=medium, 2=fast */
    unsigned short	field_valid;	/* bits 0:cur_ok 1:eide_ok */
    unsigned short	cur_cyls;	/* logical cylinders */
    unsigned short	cur_heads;	/* logical heads */
    unsigned short	cur_sectors;	/* logical sectors per track */
    unsigned short	cur_capacity0;	/* logical total sectors on drive */
    unsigned short	cur_capacity1;	/*  (2 words, misaligned int)     */
    unsigned char	multsect;	/* current multiple sector count */
    unsigned char	multsect_valid;	/* when (bit0==1) multsect is ok */
    unsigned int	lba_capacity;	/* total number of sectors */
    unsigned short	dma_1word;	/* single-word dma info */
    unsigned short	dma_mword;	/* multiple-word dma info */
    unsigned short  eide_pio_modes; /* bits 0:mode3 1:mode4 */
    unsigned short  eide_dma_min;	/* min mword dma cycle time (ns) */
    unsigned short  eide_dma_time;	/* recommended mword dma cycle time (ns) */
    unsigned short  eide_pio;       /* min cycle time (ns), no IORDY  */
    unsigned short  eide_pio_iordy; /* min cycle time (ns), with IORDY */
    unsigned short	words69_70[2];	/* reserved words 69-70 */
    unsigned short	words71_74[4];	/* reserved words 71-74 */
    unsigned short  queue_depth;	/*  */
    unsigned short  words76_79[4];	/* reserved words 76-79 */
    unsigned short  major_rev_num;	/*  */
    unsigned short  minor_rev_num;	/*  */
    unsigned short  command_set_1;	/* bits 0:Smart 1:Security 2:Removable 3:PM */
    unsigned short	command_set_2;	/* bits 14:Smart Enabled 13:0 zero 10:lba48 support*/
    unsigned short  cfsse;		/* command set-feature supported extensions */
    unsigned short  cfs_enable_1;	/* command set-feature enabled */
    unsigned short  cfs_enable_2;	/* command set-feature enabled */
    unsigned short  csf_default;	/* command set-feature default */
    unsigned short  	dma_ultra;	/*  */
    unsigned short	word89;		/* reserved (word 89) */
    unsigned short	word90;		/* reserved (word 90) */
    unsigned short	CurAPMvalues;	/* current APM values */
    unsigned short	word92;		/* reserved (word 92) */
    unsigned short	hw_config;	/* hardware config */
    unsigned short	words94_99[6];/* reserved words 94-99 */
    /*unsigned long long  lba48_capacity; /--* 4 16bit values containing lba 48 total number of sectors */
    unsigned short	lba48_capacity[4]; /* 4 16bit values containing lba 48 total number of sectors */
    unsigned short	words104_125[22];/* reserved words 104-125 */
    unsigned short	last_lun;	/* reserved (word 126) */
    unsigned short	word127;	/* reserved (word 127) */
    unsigned short	dlf;		/* device lock function
    * 15:9	reserved
    * 8	security level 1:max 0:high
    * 7:6	reserved
    * 5	enhanced erase
    * 4	expire
    * 3	frozen
    * 2	locked
    * 1	en/disabled
    * 0	capability
    */
    unsigned short  csfo;		/* current set features options
    * 15:4	reserved
    * 3	auto reassign
    * 2	reverting
    * 1	read-look-ahead
    * 0	write cache
    */
    unsigned short	words130_155[26];/* reserved vendor words 130-155 */
    unsigned short	word156;
    unsigned short	words157_159[3];/* reserved vendor words 157-159 */
    unsigned short	words160_162[3];/* reserved words 160-162 */
    unsigned short	cf_advanced_caps;
    unsigned short	words164_255[92];/* reserved words 164-255 */
} hd_driveid_t;
#endif


#endif // ATA_H


