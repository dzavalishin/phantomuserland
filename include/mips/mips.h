/************************************************************************
 *
 *  mips.h
 *
 *  MIPS processor definitions
 *
 *  The basic CPU definitions are found in the file ArchDefs.h, which
 *  is included by mips.h.
 *
 *  mips.h implements aliases for some of the definitions in ArchDefs.h
 *  and adds various definitions.
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

#ifndef MIPS_H
#define MIPS_H

/************************************************************************
 *  Include files
 ************************************************************************/

#ifndef MIPS_Release2
#define MIPS_Release2
#endif
#ifndef MIPS_MT
#define MIPS_MT
#endif
#include <ArchDefs.h>

/************************************************************************
 *  Definitions
*************************************************************************/

#ifndef MSK
#define MSK(n)			  ((1 << (n)) - 1)
#endif

/* CPU registers */
#define SYS_CPUREG_ZERO	0
#define SYS_CPUREG_AT	1
#define SYS_CPUREG_V0	2
#define SYS_CPUREG_V1	3
#define SYS_CPUREG_A0	4
#define SYS_CPUREG_A1	5
#define SYS_CPUREG_A2	6
#define SYS_CPUREG_A3	7
#define SYS_CPUREG_T0	8
#define SYS_CPUREG_T1	9
#define SYS_CPUREG_T2	10
#define SYS_CPUREG_T3	11
#define SYS_CPUREG_T4	12
#define SYS_CPUREG_T5	13
#define SYS_CPUREG_T6	14
#define SYS_CPUREG_T7	15
#define SYS_CPUREG_S0	16
#define SYS_CPUREG_S1	17
#define SYS_CPUREG_S2	18
#define SYS_CPUREG_S3	19
#define SYS_CPUREG_S4	20
#define SYS_CPUREG_S5	21
#define SYS_CPUREG_S6	22
#define SYS_CPUREG_S7	23
#define SYS_CPUREG_T8	24
#define SYS_CPUREG_T9	25
#define SYS_CPUREG_K0	26
#define SYS_CPUREG_K1	27
#define SYS_CPUREG_GP	28
#define SYS_CPUREG_SP	29
#define SYS_CPUREG_S8	30
#define SYS_CPUREG_FP	SYS_CPUREG_S8		
#define SYS_CPUREG_RA	31


/* CPU register fp ($30) has an alias s8 */
// Use asm.h, Luke
//#define s8		fp


/* C0_CONFIG register encoding */

/*  WC field.
 *
 *  This feature is present specifically to support configuration
 *  testing of the core in a lead vehicle, and is not supported
 *  in any other environment.  Attempting to use this feature
 *  outside of the scope of a lead vehicle is a violation of the
 *  MIPS Architecture, and may cause unpredictable operation of
 *  the processor.
 */
#define C0_CONFIG_WC_SHF		19
#define C0_CONFIG_WC_MSK    		(MSK(1) << C0_CONFIG_WC_SHF)
#define C0_CONFIG_WC_BIT		C0_CONFIG_WC_MSK


/* C0_Status register encoding */

/*  Note that the the definitions below indicate the interrupt number
 *  rather than the mask.
 *  (0..1 for SW interrupts and 2...7 for HW interrupts)
 */
#define C0_STATUS_IM_SW0		(S_StatusIM0 - S_StatusIM)
#define C0_STATUS_IM_SW1		(S_StatusIM1 - S_StatusIM)
#define C0_STATUS_IM_HW0		(S_StatusIM2 - S_StatusIM)
#define C0_STATUS_IM_HW1		(S_StatusIM3 - S_StatusIM)
#define C0_STATUS_IM_HW2		(S_StatusIM4 - S_StatusIM)
#define C0_STATUS_IM_HW3		(S_StatusIM5 - S_StatusIM)
#define C0_STATUS_IM_HW4		(S_StatusIM6 - S_StatusIM)
#define C0_STATUS_IM_HW5		(S_StatusIM7 - S_StatusIM)

/* Max interrupt code */
#define C0_STATUS_IM_MAX		C0_STATUS_IM_HW5


/* C0_PRId register encoding */

#define C0_PRID_COMP_NOT_MIPS32_64	0
#define C0_PRID_PRID_RM70XX  		0x27

#define MIPS_4Kc			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_Jade  << S_PRIdImp) )

#define MIPS_4Kmp			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_JadeLite  << S_PRIdImp) )

#define MIPS_4KEc			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_4KEc  << S_PRIdImp) )

#define MIPS_4KEc_R2			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_4KEc_R2 << S_PRIdImp) )

#define MIPS_4KEmp			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_4KEmp  << S_PRIdImp) )

#define MIPS_4KEmp_R2			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_4KEmp_R2 << S_PRIdImp) )

#define MIPS_4KSc			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_4KSc  << S_PRIdImp) )

#define MIPS_4KSd			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_4KSd  << S_PRIdImp) )

#define MIPS_5K				( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_Opal  << S_PRIdImp) )

#define MIPS_20Kc			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_Ruby  << S_PRIdImp) )

#define MIPS_M4K			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_M4K  <<  S_PRIdImp) )

#define MIPS_M14K			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_M14K << S_PRIdImp) )

#define MIPS_M14Kc			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_M14Kc << S_PRIdImp) )

#define MIPS_25Kf			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_Amethyst  << S_PRIdImp) )

#define MIPS_5KE			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_5KE   << S_PRIdImp) )

#define MIPS_24K			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_24K   << S_PRIdImp) )

#define MIPS_24KE			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_24KE   << S_PRIdImp) )

#define MIPS_34K			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_34K   << S_PRIdImp) )

#define MIPS_74K			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_74K   << S_PRIdImp) )

#define MIPS_1074K			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_1074K   << S_PRIdImp) )

#define MIPS_1004K			( (K_PRIdCoID_MIPS << S_PRIdCoID) | \
					  (K_PRIdImp_1004K << S_PRIdImp) )

#define QED_RM52XX			( (C0_PRID_COMP_NOT_MIPS32_64 << \
					      S_PRIdCoID) |	 \
					  (K_PRIdImp_R5200  << S_PRIdImp) )

#define QED_RM70XX			( (C0_PRID_COMP_NOT_MIPS32_64 << \
					      S_PRIdCoID) |	 \
					  (C0_PRID_PRID_RM70XX  << S_PRIdImp) )


/* cache operations */

#define CACHE_OP( code, type )			( ((code) << 2) | (type) )

#define ICACHE_INDEX_INVALIDATE			CACHE_OP(0x0, 0)
#define ICACHE_INDEX_LOAD_TAG			CACHE_OP(0x1, 0)
#define ICACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 0)
#define DCACHE_INDEX_WRITEBACK_INVALIDATE	CACHE_OP(0x0, 1)
#define DCACHE_INDEX_LOAD_TAG			CACHE_OP(0x1, 1)
#define DCACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 1)
#define SCACHE_INDEX_WRITEBACK_INVALIDATE	CACHE_OP(0x0, 3)
#define SCACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 3)

#define ICACHE_ADDR_HIT_INVALIDATE		CACHE_OP(0x4, 0)
#define ICACHE_ADDR_FILL			CACHE_OP(0x5, 0)
#define ICACHE_ADDR_FETCH_LOCK			CACHE_OP(0x7, 0)
#define DCACHE_ADDR_HIT_INVALIDATE		CACHE_OP(0x4, 1)
#define DCACHE_ADDR_HIT_WRITEBACK_INVALIDATE	CACHE_OP(0x5, 1)
#define DCACHE_ADDR_HIT_WRITEBACK		CACHE_OP(0x6, 1)
#define DCACHE_ADDR_FETCH_LOCK			CACHE_OP(0x7, 1)

#define SCACHE_ADDR_HIT_WRITEBACK_INVALIDATE	CACHE_OP(0x5, 3)

/*  Workaround for bug in early revisions of MIPS 4K family of 
 *  processors. Only relevant in early engineering samples of test
 *  chips (RTL revision <= 3.0).
 *
 *  The bug is described in :
 *
 *  MIPS32 4K(tm) Processor Core Family RTL Errata Sheet
 *  MIPS Document No: MD00003
 *
 *  The bug is identified as : C16
 */
#define ICACHE_INVALIDATE_WORKAROUND(reg) \
SET_PUSH();				  \
SET_MIPS0();				  \
	la     reg, 999f;		  \
SET_POP();				  \
	cache  ICACHE_ADDR_FILL, 0(reg);  \
	sync;				  \
	nop; nop; nop; nop;		  \
999:

#define ICACHE_INDEX_INVALIDATE_OP(index,scratch)		  \
	    ICACHE_INVALIDATE_WORKAROUND(scratch);		  \
	    cache ICACHE_INDEX_INVALIDATE, 0(index)

#define ICACHE_ADDR_INVALIDATE_OP(addr,scratch)			  \
	    ICACHE_INVALIDATE_WORKAROUND(scratch);		  \
	    cache ICACHE_ADDR_HIT_INVALIDATE, 0(addr)

#define SCACHE_ADDR_HIT_WB_INVALIDATE_OP(reg)			  \
	    cache   SCACHE_ADDR_HIT_WRITEBACK_INVALIDATE, 0(reg);

#define SCACHE_INDEX_WRITEBACK_INVALIDATE_OP(reg)			  \
	    cache   SCACHE_INDEX_WRITEBACK_INVALIDATE, 0(reg);

/* Config1 cache field decoding */
#define CACHE_CALC_SPW(s)	( 32 << (s) )
#define CACHE_CALC_LS(l)	( (l) ? 2 << (l) : 0 )
#define CACHE_CALC_BPW(l,s)	( CACHE_CALC_LS(l) * CACHE_CALC_SPW(s) )
#define CACHE_CALC_ASSOC(a)	( (a) + 1 )


/**** Move from/to Coprocessor operations ****/

/*  We use ssnop instead of nop operations in order to handle 
 *  superscalar CPUs.
 *  The "sll zero,zero,1" notation is compiler backwards compatible.
 */
#define SSNOP   sll zero,zero,1
#define EHB     sll zero,zero,3
#define NOPS	SSNOP; SSNOP; SSNOP; EHB

/*  Workaround for bug in early revisions of MIPS 4K family of 
 *  processors.
 *
 *  This concerns the nop instruction before mtc0 in the 
 *  MTC0 macro below.
 *
 *  The bug is described in :
 *
 *  MIPS32 4K(tm) Processor Core Family RTL Errata Sheet
 *  MIPS Document No: MD00003
 *
 *  The bug is identified as : C27
 */

#define MTC0(src, dst)       \
		nop;	     \
	        mtc0 src,dst;\
		NOPS

#define DMTC0(src, dst)       \
		nop;	      \
	        dmtc0 src,dst;\
		NOPS

#define MFC0(dst, src)       \
	  	mfc0 dst,src

#define DMFC0(dst, src)       \
	  	dmfc0 dst,src

#define MFC0_SEL_OPCODE(dst, src, sel)\
	  	.##word (0x40000000 | ((dst)<<16) | ((src)<<11) | (sel))

#define MTC0_SEL_OPCODE(src, dst, sel)\
	  	.##word (0x40800000 | ((src)<<16) | ((dst)<<11) | (sel));\
		NOPS

#define LDC1(dst, src, offs)\
		.##word (0xd4000000 | ((src)<<21) | ((dst)<<16) | (offs))

#define SDC1(src, dst, offs)\
		.##word (0xf4000000 | ((dst)<<21) | ((src)<<16) | (offs))

/* Release 2 */
#define RDPGPR( rd, rt )\
		.##word (0x41400000 | ((rd) <<11) | (rt<<16))

#define WRPGPR( rd, rt )\
		.##word (0x41c00000 | ((rd) <<11) | (rt<<16))


/* MT ASE */
#define DVPE(rt) \
		.##word (0x41600001 | ((rt)<<16))

#define DMT(rt)	\
		.##word (0x41600bc1 | ((rt)<<16))

#define EVPE(rt) \
		.##word (0x41600021 | ((rt)<<16))

#define EMT(rt)	\
		.##word (0x41600be1 | ((rt)<<16))

/* Instruction opcode fields */
#define OPC_SPECIAL   0x0
#define OPC_REGIM     0x1
#define OPC_J         0x2
#define OPC_JAL	      0x3
#define OPC_BEQ	      0x4
#define OPC_BNE	      0x5
#define OPC_BLEZ      0x6
#define OPC_BGTZ      0x7
#define OPC_COP1      0x11
#define OPC_JALX      0x1D
#define OPC_BEQL      0x14
#define OPC_BNEL      0x15
#define OPC_BLEZL     0x16
#define OPC_BGTZL     0x17

/* Instruction function fields */
#define FUNC_JR	      0x8
#define FUNC_JALR     0x9

/* Instruction rt fields */
#define RT_BLTZ	      0x0
#define RT_BGEZ	      0x1
#define RT_BLTZL      0x2
#define RT_BGEZL      0x3
#define RT_BLTZAL     0x10
#define RT_BGEZAL     0x11
#define RT_BLTZALL    0x12
#define RT_BGEZALL    0x13

/* Instruction rs fields */
#define RS_BC1	      0x08

/* Access macros for instruction fields */
#define MIPS_OPCODE( instr)	((instr) >> 26)
#define MIPS_FUNCTION(instr)	((instr) & MSK(6))
#define MIPS_RT(instr)		(((instr) >> 16) & MSK(5))
#define MIPS_RS(instr)		(((instr) >> 21) & MSK(5))
#define MIPS_OFFSET(instr)	((instr) & 0xFFFF)
#define MIPS_TARGET(instr)	((instr) & MSK(26))

/* Instructions */
#define OPCODE_DERET		0x4200001f
#define OPCODE_BREAK	  	0x0005000d
#define OPCODE_NOP		0
#define OPCODE_JUMP(addr)	( (OPC_J << 26) | (((addr) >> 2) & 0x3FFFFFF) )

#define DERET			.##word OPCODE_DERET

/* MIPS16e opcodes and instruction field access macros */

#define MIPS16E_OPCODE(inst)		(((inst) >> 11) & 0x1f)
#define MIPS16E_I8_FUNCTION(inst)	(((inst) >>  8) & 0x7)
#define MIPS16E_X(inst) 		(((inst) >> 26) & 0x1)
#define MIPS16E_RR_FUNCTION(inst)	(((inst) >>  0) & 0x1f)
#define MIPS16E_RY(inst)		(((inst) >>  5) & 0x3)
#define MIPS16E_OPC_EXTEND		0x1e
#define MIPS16E_OPC_JAL_X		0x03
#define MIPS16E_OPC_B			0x02
#define MIPS16E_OPC_BEQZ		0x04
#define MIPS16E_OPC_BNEZ		0x05
#define MIPS16E_OPC_I8			0x0c
#define MIPS16E_I8_FUNC_BTEQZ		0x00
#define MIPS16E_I8_FUNC_BTNEZ		0x01
#define MIPS16E_X_JALX			0x01
#define MIPS16E_OPC_RR			0x1d
#define MIPS16E_RR_FUNC_JALRC		0x00
#define MIPS16E_RR_RY_JRRX		0x00
#define MIPS16E_RR_RY_JRRA		0x01
#define MIPS16E_RR_RY_JALR		0x02
#define MIPS16E_RR_RY_JRCRX		0x04
#define MIPS16E_RR_RY_JRCRA		0x05
#define MIPS16E_RR_RY_JALRC		0x06

#define MIPS16E_OPCODE_BREAK		0xE805
#define MIPS16E_OPCODE_NOP		0x6500

/* MIPS reset vector */
#define MIPS_RESET_VECTOR       0x1fc00000

/* Clock periods per count register increment */
#define MIPS4K_COUNT_CLK_PER_CYCLE	2
#define MIPS5K_COUNT_CLK_PER_CYCLE	2
#define MIPS20Kc_COUNT_CLK_PER_CYCLE	1
#define MIPS24K_COUNT_CLK_PER_CYCLE     2
#define MIPS34K_COUNT_CLK_PER_CYCLE     2
#define MIPS74K_COUNT_CLK_PER_CYCLE     2
#define MIPS1074K_COUNT_CLK_PER_CYCLE   2
#define MIPS1004K_COUNT_CLK_PER_CYCLE   2
#define MIPSM4K_COUNT_CLK_PER_CYCLE     2

/**** MIPS 4K/5K families specific fields of CONFIG register ****/

#define C0_CONFIG_MIPS4K5K_K23_MSK   (MSK(3) << S_ConfigK23)
#define C0_CONFIG_MIPS4K5K_KU_MSK    (MSK(3) << S_ConfigKU)

/**** MIPS 20Kc specific fields of CONFIG register ****/

#define C0_CONFIG_MIPS20KC_EC_SHF    28
#define C0_CONFIG_MIPS20KC_EC_MSK    (MSK(3) << C0_CONFIG_MIPS20KC_EC_SHF)

#define C0_CONFIG_MIPS20KC_DD_SHF    27
#define C0_CONFIG_MIPS20KC_DD_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_DD_SHF)
#define C0_CONFIG_MIPS20KC_DD_BIT    C0_CONFIG_MIPS20KC_DD_MSK

#define C0_CONFIG_MIPS20KC_LP_SHF    26
#define C0_CONFIG_MIPS20KC_LP_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_LP_SHF)
#define C0_CONFIG_MIPS20KC_LP_BIT    C0_CONFIG_MIPS20KC_LP_MSK

#define C0_CONFIG_MIPS20KC_SP_SHF    25
#define C0_CONFIG_MIPS20KC_SP_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_SP_SHF)
#define C0_CONFIG_MIPS20KC_SP_BIT    C0_CONFIG_MIPS20KC_SP_MSK

#define C0_CONFIG_MIPS20KC_TI_SHF    24
#define C0_CONFIG_MIPS20KC_TI_MSK    (MSK(1) << C0_CONFIG_MIPS20KC_TI_SHF)
#define C0_CONFIG_MIPS20KC_TI_BIT    C0_CONFIG_MIPS20KC_TI_MSK

/**** MIPS L2 cache specific fields of CONFIG2 register ****/
#define S_Config2L2B		     12			/* L2 bypass */
#define M_Config2L2B		     (0x1 << S_Config2L2B)

/* TBD : Until these appear in ArchDefs.h */

#define R_C0_DErrCtl		26
#define R_C0_SelDErrCtl		0
#define R_C0_IErrCtl		26
#define R_C0_SelIErrCtl		1

#define R_C0_L23TagLo	        28
#define R_C0_SelL23TagLo	4

#define R_C0_L23DataLo		28
#define R_C0_SelL23DataLo	5

#define R_C0_L23TagHi		29
#define R_C0_SelL23TagHi	4

#define R_C0_IDataHi		29
#define R_C0_SelIDataHi		1
#define R_C0_DDataHi		29
#define R_C0_SelDDataHi		3
#define R_C0_L23DataHi		29
#define R_C0_SelL23DataHi	5

#define S_ConfigMM              18     /* 24K specific, merging enable/disable */
#define M_ConfigMM              (0x1 << S_ConfigMM)

#define R_C0_Config4		16
#define R_C0_SelConfig4		4
#define R_C0_Config5		16
#define R_C0_SelConfig5		5
#define R_C0_Config6		16
#define R_C0_SelConfig6		6
#define R_C0_Config7		16
#define R_C0_SelConfig7		7

#define S_Config7WII		31			/* Interrupt safe wait */
#define M_Config7WII		(0x1 << S_Config7WII)

#define S_Config7AR		16			/* Dcache antialiasing present */
#define M_Config7AR		(0x1 << S_Config7AR)

#define K_PRIdImp_1004K		K_PRIdImp_34KMP	/* MIPS32 1004K */
#define K_PRIdImp_1074K		K_PRIdImp_74KMP	/* MIPS32 1074K */
#define K_PRIdImp_M14K		0x9b	/* MIPS32 M14K */
#define K_PRIdImp_M14Kc		0x9c	/* MIPS32 M14Kc */

#define S_Config3CDMM		3
#define M_Config3CDMM		(0x1 << S_Config3CDMM)

#define R_C0_CDMMBase		15
#define R_C0_SelCDMMBase	2

#endif /* #ifndef MIPS_H */

