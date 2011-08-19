
/************************************************************************
 *
 *  init.h
 *
 *  Local definitions for init code
 *
 * ######################################################################
 *
 * mips_start_of_legal_notice
 * 
 * Copyright (c) 2011 MIPS Technologies, Inc. All rights reserved.
 *
 *
 * Unpublished rights (if any) reserved under the copyright laws of the
 * United States of America and other countries.
 *
 * This code is proprietary to MIPS Technologies, Inc. ("MIPS
 * Technologies"). Any copying, reproducing, modifying or use of this code
 * (in whole or in part) that is not expressly permitted in writing by MIPS
 * Technologies or an authorized third party is strictly prohibited. At a
 * minimum, this code is protected under unfair competition and copyright
 * laws. Violations thereof may result in criminal penalties and fines.
 *
 * MIPS Technologies reserves the right to change this code to improve
 * function, design or otherwise. MIPS Technologies does not assume any
 * liability arising out of the application or use of this code, or of any
 * error or omission in such code. Any warranties, whether express,
 * statutory, implied or otherwise, including but not limited to the implied
 * warranties of merchantability or fitness for a particular purpose, are
 * excluded. Except as expressly provided in any written license agreement
 * from MIPS Technologies or an authorized third party, the furnishing of
 * this code does not give recipient any license to any intellectual
 * property rights, including any patent rights, that cover this code.
 *
 * This code shall not be exported or transferred for the purpose of
 * reexporting in violation of any U.S. or non-U.S. regulation, treaty,
 * Executive Order, law, statute, amendment or supplement thereto.
 *
 * This code constitutes one or more of the following: commercial computer
 * software, commercial computer software documentation or other commercial
 * items. If the user of this code, or any related documentation of any
 * kind, including related technical data or manuals, is an agency,
 * department, or other entity of the United States government
 * ("Government"), the use, duplication, reproduction, release,
 * modification, disclosure, or transfer of this code, or any related
 * documentation of any kind, is restricted in accordance with Federal
 * Acquisition Regulation 12.212 for civilian agencies and Defense Federal
 * Acquisition Regulation Supplement 227.7202 for military agencies. The use
 * of this code by the Government is further restricted in accordance with
 * the terms of the license agreement(s) and/or applicable contract terms
 * and conditions covering this code from MIPS Technologies or an authorized
 * third party.
 *
 * 
 * mips_end_of_legal_notice
 * 
 *
 ************************************************************************/


#ifndef INIT_H
#define INIT_H


/************************************************************************
 *  Include files
 ************************************************************************/

#include <qed.h>
#include <mips.h>

/************************************************************************
 *  Definitions
*************************************************************************/

/* MIPS32/MIPS64 specifics */

/*  Setup of STATUS register used for MIPS32/MIPS64 processors
 *  FR field only relevant for MIPS64 (Read only for MIPS32)
 */
#define STATUS_MIPS32_64   (M_StatusBEV | M_StatusFR)

/*  Generic MIPS32/MIPS64 fields of STATUS register (ie the ones not 
 *  reserved for implementations)
 */
#define STATUS_MIPS32_64_MSK   0xfffcffff

/* Setup of CONFIG register used for MIPS32/MIPS64 processors */

#ifdef NO_CACHE

#define CONFIG0_MIPS32_64    (K_CacheAttrU << S_ConfigK0)

#else

#ifdef KSEG0_UNCACHED
#define CONFIG0_MIPS32_64    (K_CacheAttrU << S_ConfigK0)
#else
#define CONFIG0_MIPS32_64    (K_CacheAttrCN << S_ConfigK0)
#endif

#endif

/*  Generic MIPS32/MIPS64 fields of CONFIG0 register (ie the ones not
 *  reserved for implementations)
 */
#define CONFIG0_MIPS32_64_MSK  0x8000ffff


/* MIPS 4K/5K family specifics (excluding generic MIPS32/MIPS64 fields) */
#define STATUS_MIPS4K5K	    0
#define CONFIG0_MIPS4K5K ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU))

/* MIPS 24K specifics */
#define STATUS_MIPS24K	    0
#define CONFIG0_MIPS24K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 34K specifics */
#define STATUS_MIPS34K	    0
#define CONFIG0_MIPS34K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 74K specifics */
#define STATUS_MIPS74K	    0
#define CONFIG0_MIPS74K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 1074K specifics */
#define STATUS_MIPS1074K    0
#define CONFIG0_MIPS1074K  ((K_CacheAttrCN << S_ConfigK23) |\
			    (K_CacheAttrCN << S_ConfigKU)  |\
			    (M_ConfigMM))

/* MIPS 1004K specifics */
#define STATUS_MIPS1004K	    0
#define CONFIG0_MIPS1004K  ((K_CacheAttrCN << S_ConfigK23) |\
			  (K_CacheAttrCN << S_ConfigKU)  |\
			  (M_ConfigMM))

/* MIPS 20Kc/25Kf specifics (excluding generic MIPS32/MIPS64 fields) */
#ifdef WORKAROUND_20KC_25KF
#define STATUS_MIPS20KC_25KF	(0x1 << 16)
#else
#define STATUS_MIPS20KC_25KF	0
#endif
#define CONFIG0_MIPS20KC_25KF   0


/* QED specifics  */
#define STATUS_QED	((C0_STATUS_QED_KSU_KERNEL << C0_STATUS_QED_KSU_SHF) |\
			  C0_STATUS_QED_BEV_BIT | C0_STATUS_QED_FR_BIT)

#define CONFIG_QED	((C0_CONFIG_QED_EC_X3   << C0_CONFIG_QED_EC_SHF) |\
			 (C0_CONFIG_QED_EP_DDDD << C0_CONFIG_QED_EP_SHF) |\
			 (C0_CONFIG_QED_EW_64   << C0_CONFIG_QED_EW_SHF) |\
			 C0_CONFIG_QED_SCN_BIT  			 |\
			 C0_CONFIG_QED_EM_BIT				 |\
			 (C0_CONFIG_QED_EB_SUB  << C0_CONFIG_QED_EB_SHF) |\
			 (C0_CONFIG_QED_K0_NONCOHERENT << C0_CONFIG_QED_K0_SHF))


/*  Error handling. Each error requires the following :
 *  
 *  1) An error code as defined here.
 *  2) An error message string as defined by ERROR_MESSAGES (see below).
 *  3) An entry in error handling code as defined by ERROR_HANDLING (see below).
 */

/* Error codes */

#define ERROR_PLATFORM_UNKNOWN	  1
#define ERROR_CORE_UNKNOWN	  2
#define ERROR_PROCESSOR_UNKNOWN	  3
#define ERROR_RAM_LO		  4
#define ERROR_RAM_MI		  5	
#define ERROR_RAM_HI		  6
#define ERROR_MEMTEST_WORD	  7
#define ERROR_MEMTEST_BYTE	  8
#define ERROR_SPD		  9
#define ERROR_SDRAM_WIDTH	  10
#define ERROR_SDRAM_CASLAT	  11
#define ERROR_SDRAM_BURSTLEN	  12
#define ERROR_SDRAM_ERRORCHECK	  13
#define ERROR_SDRAM_DEV_BANKS	  14
#define ERROR_SDRAM_MOD_BANKS	  15
#define ERROR_SDRAM_CONFIG	  16
#define ERROR_SDRAM_SIZE	  17
#define ERROR_NB_CONFIG_WRITE	  18
#define ERROR_NB_CONFIG_READ	  19
#define	ERROR_NB_DECODE		  20
#define ERROR_STRUCTURE		  21


/**** Cpu specific initialisation ****/	

#if defined(_ASSEMBLER_) || defined(ASSEMBLER)

#define MSG( name, s ) \
	.##align 3;      \
name:   .##asciiz  s


/* Error messages */

#define ERROR_MESSAGES			        \
						\
MSG( msg_ERROR_PROCESSOR_UNKNOWN, "E:CPU"    ); \
MSG( msg_ERROR_RAM_LO,		  "E:RAM_LO" ); \
MSG( msg_ERROR_RAM_MI,		  "E:RAM_MI" ); \
MSG( msg_ERROR_RAM_HI,	          "E:RAM_HI" ); \
MSG( msg_ERROR_MEMTEST_WORD,	  "E:RAM_W"  ); \
MSG( msg_ERROR_MEMTEST_BYTE,	  "E:RAM_B"  ); \
MSG( msg_ERROR_SPD,		  "E:NO_RAM" ); \
MSG( msg_ERROR_SDRAM_WIDTH,       "E:RAM_WH" ); \
MSG( msg_ERROR_SDRAM_CASLAT,      "E:RAM_CL" ); \
MSG( msg_ERROR_SDRAM_BURSTLEN,    "E:RAM_BL" ); \
MSG( msg_ERROR_SDRAM_ERRORCHECK,  "E:RAM_EC" ); \
MSG( msg_ERROR_SDRAM_DEV_BANKS,   "E:RAM_DB" ); \
MSG( msg_ERROR_SDRAM_MOD_BANKS,   "E:RAM_MB" ); \
MSG( msg_ERROR_SDRAM_CONFIG,      "E:RAM_CF" ); \
MSG( msg_ERROR_SDRAM_SIZE,        "E:RAM_SZ" ); \
MSG( msg_ERROR_NB_CONFIG_WRITE,   "E:NB_CW"  ); \
MSG( msg_ERROR_NB_CONFIG_READ,    "E:NB_CR"  ); \
MSG( msg_ERROR_NB_DECODE,         "E:NB_DEC" ); \
MSG( msg_ERROR_STRUCTURE,         "E:STRUCT" ); \
MSG( msg_ERROR_UNKNOWN,	          "E:UNKNWN" )


/*  Error handling code.
 *
 *  ERROR0 macro : Don't display anything since we don't have
 *	 	   enough platform data to do this.
 *
 *  ERROR1 macro : Display error message in display.
 */

#define ERROR0( code )		  \
	li	t0, code;	  \
	bne	v0, t0, 1f;	  \
	nop;			  \
	b	error_loop;	  \
	nop;			  \
1:

#define ERROR1( code, msg )	  \
	li	t0, code;	  \
	bne	v0, t0, 1f;	  \
	nop;			  \
	la	t9, msg;	  \
	b	error_write;	  \
	nop;			  \
1:

#define ERROR_HANDLING						\
								\
ERROR0( ERROR_PLATFORM_UNKNOWN )				\
ERROR0( ERROR_CORE_UNKNOWN )					\
ERROR1( ERROR_PROCESSOR_UNKNOWN, msg_ERROR_PROCESSOR_UNKNOWN )	\
ERROR1( ERROR_RAM_LO,		 msg_ERROR_RAM_LO )		\
ERROR1( ERROR_RAM_MI,		 msg_ERROR_RAM_MI )		\
ERROR1( ERROR_RAM_HI,		 msg_ERROR_RAM_HI )		\
ERROR1( ERROR_MEMTEST_WORD,	 msg_ERROR_MEMTEST_WORD )	\
ERROR1( ERROR_MEMTEST_BYTE,	 msg_ERROR_MEMTEST_BYTE )	\
ERROR1( ERROR_SPD,		 msg_ERROR_SPD )		\
ERROR1( ERROR_SDRAM_WIDTH,	 msg_ERROR_SDRAM_WIDTH )	\
ERROR1( ERROR_SDRAM_CASLAT,	 msg_ERROR_SDRAM_CASLAT )	\
ERROR1( ERROR_SDRAM_BURSTLEN,	 msg_ERROR_SDRAM_BURSTLEN )	\
ERROR1( ERROR_SDRAM_ERRORCHECK,	 msg_ERROR_SDRAM_ERRORCHECK )	\
ERROR1( ERROR_SDRAM_DEV_BANKS,	 msg_ERROR_SDRAM_DEV_BANKS )	\
ERROR1( ERROR_SDRAM_MOD_BANKS,	 msg_ERROR_SDRAM_MOD_BANKS )	\
ERROR1( ERROR_SDRAM_CONFIG,	 msg_ERROR_SDRAM_CONFIG )	\
ERROR1( ERROR_SDRAM_SIZE,	 msg_ERROR_SDRAM_SIZE )		\
ERROR1( ERROR_NB_CONFIG_WRITE,	 msg_ERROR_NB_CONFIG_WRITE )	\
ERROR1( ERROR_NB_CONFIG_READ,	 msg_ERROR_NB_CONFIG_READ )	\
ERROR1( ERROR_NB_DECODE,	 msg_ERROR_NB_DECODE )		\
ERROR1( ERROR_STRUCTURE,	 msg_ERROR_STRUCTURE )		\
								\
la	t9, msg_ERROR_UNKNOWN;					\
								\
error_write:							\
	/* Write error message and idle loop forever */		\
	jal	sys_disp_string;				\
	nop;							\
error_loop:							\
	b	error_loop;					\
	nop




/* DISP_STR macro modifies t9..t5 and expects k0 to hold platformID */
#define DISP_STR( s );\
		la   t9, s;\
		KSEG1A( t9 );\
		move t8, ra;\
		jal  sys_disp_string;\
		nop;\
		move ra, t8


/* Core card specific function identification */
#define FUNC_INIT		0
#define FUNC_GET_PCIMEM_BASE	1
#define FUNC_CONFIG_WRITE	2
#define FUNC_CONFIGURE_SDRAM	4
#define FUNC_SETUP_DECODE	5
#define FUNC_REMAP_PCI_IO	6


#else  /* #ifdef _ASSEMBLER_ */


/************************************************************************
 *
 *                          arch_platform_init
 *  Description :
 *  -------------
 *
 *  Platform specific initialisation code
 *
 *  Return values :
 *  ---------------
 *
 *  None
 *
 ************************************************************************/
void
arch_platform_init(
    bool early );	/* TRUE -> Before initmodules(), FALSE -> after */


/************************************************************************
 *
 *                          arch_core_init
 *  Description :
 *  -------------
 *
 *  Core card specific initialisation code
 *
 *  Return values :
 *  ---------------
 *
 *  None
 *
 ************************************************************************/
void
arch_core_init( 
    bool   early,	/* TRUE -> Before initmodules(), FALSE -> after */
    UINT32 intline,	/* Interrupt line for North Bridge		*/
    bool   cpu_isr );	/* TRUE  -> Interrupt line connected to CPU.
			 * FALSE -> Interrupt line connected to
			 *          interrupt controller.
			 */


/************************************************************************
 *
 *                          arch_core_estimate_busfreq
 *  Description :
 *  -------------
 *
 *  Estimate external bus (SysAD) clock frequency.
 *
 *  Return values :
 *  ---------------
 *
 *  Estimated frequency in Hz.
 *
 ************************************************************************/
UINT32
arch_core_estimate_busfreq( void );


#endif /* #ifdef _ASSEMBLER_ */
     


#endif /* #ifndef INIT_H */
