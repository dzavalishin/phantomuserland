
/************************************************************************
 *
 *  qed.h
 *
 *  Definitions for QED processors
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


#ifndef QED_H
#define QED_H


/************************************************************************
 *  Include files
 ************************************************************************/

/************************************************************************
 *  Register encodings
 ************************************************************************/

/**** CONFIG ****/

#define C0_CONFIG_QED_EC_SHF	28
#define C0_CONFIG_QED_EC_MSK	(MSK(3) << C0_CONFIG_QED_EC_SHF)
#define C0_CONFIG_QED_EC_X2	0
#define C0_CONFIG_QED_EC_X3	1
#define C0_CONFIG_QED_EC_X4	2
#define C0_CONFIG_QED_EC_X5	3
#define C0_CONFIG_QED_EC_X6	4
#define C0_CONFIG_QED_EC_X7	5
#define C0_CONFIG_QED_EC_X8	6
#define C0_CONFIG_QED_EC_RSVD	7

#define C0_CONFIG_QED_EP_SHF	24
#define C0_CONFIG_QED_EP_MSK	(MSK(4) << C0_CONFIG_QED_EP_SHF)
#define C0_CONFIG_QED_EP_DDDD			0
#define C0_CONFIG_QED_EP_DDXDDX			1
#define C0_CONFIG_QED_EP_DDXXDDXX		2
#define C0_CONFIG_QED_EP_DXDXDXDX		3
#define C0_CONFIG_QED_EP_DDXXXDDXXX		4
#define C0_CONFIG_QED_EP_DDXXXXDDXXXX		5
#define C0_CONFIG_QED_EP_DXXDXXDXXDXX		6
#define C0_CONFIG_QED_EP_DDXXXXXXDDXXXXXX	7	
#define C0_CONFIG_QED_EP_DXXXDXXXDXXXDXXX	8

#define C0_CONFIG_QED_SB_SHF	22
#define C0_CONFIG_QED_SB_MSK	(MSK(2) << C0_CONFIG_QED_SB_SHF)
#define C0_CONFIG_QED_SB_4	0
#define C0_CONFIG_QED_SB_8	1
#define C0_CONFIG_QED_SB_16	2
#define C0_CONFIG_QED_SB_32	3

#define C0_CONFIG_QED_SS_SHF	20
#define C0_CONFIG_QED_SS_MSK	(MSK(2) << C0_CONFIG_QED_SS_SHF)

#define C0_CONFIG_QED_EW_SHF	18	
#define C0_CONFIG_QED_EW_MSK	(MSK(2) << C0_CONFIG_QED_EW_SHF)
#define C0_CONFIG_QED_EW_64	0
#define C0_CONFIG_QED_EW_32	1

#define C0_CONFIG_QED_SCN_SHF	17	
#define C0_CONFIG_QED_SCN_MSK	(MSK(1) << C0_CONFIG_QED_SCN_SHF)
#define C0_CONFIG_QED_SCN_BIT	C0_CONFIG_QED_SCN_MSK

#define C0_CONFIG_QED_BE_SHF	15
#define C0_CONFIG_QED_BE_MSK	(MSK(1) << C0_CONFIG_QED_BE_SHF)
#define C0_CONFIG_QED_BE_BIT	C0_CONFIG_QED_BE_MSK

#define C0_CONFIG_QED_EM_SHF	14
#define C0_CONFIG_QED_EM_MSK	(MSK(1) << C0_CONFIG_QED_EM_SHF)
#define C0_CONFIG_QED_EM_BIT	C0_CONFIG_QED_EM_MSK

#define C0_CONFIG_QED_EB_SHF	13
#define C0_CONFIG_QED_EB_MSK	(MSK(1) << C0_CONFIG_QED_EB_SHF)
#define C0_CONFIG_QED_EB_BIT	C0_CONFIG_QED_EB_MSK
#define C0_CONFIG_QED_EB_SEQ	0
#define C0_CONFIG_QED_EB_SUB	1

#define C0_CONFIG_QED_IC_SHF	9
#define C0_CONFIG_QED_IC_MSK	(MSK(3) << C0_CONFIG_QED_IC_SHF)

#define C0_CONFIG_QED_DC_SHF	6
#define C0_CONFIG_QED_DC_MSK 	(MSK(3) << C0_CONFIG_QED_DC_SHF)

#define C0_CONFIG_QED_IB_SHF	5
#define C0_CONFIG_QED_IB_MSK	(MSK(1) << C0_CONFIG_QED_IB_SHF)
#define C0_CONFIG_QED_IB_BIT	C0_CONFIG_QED_IB_MSK
#define C0_CONFIG_QED_IB_16	0
#define C0_CONFIG_QED_IB_32	1

#define C0_CONFIG_QED_DB_SHF	4
#define C0_CONFIG_QED_DB_MSK	(MSK(1) << C0_CONFIG_QED_DB_SHF)
#define C0_CONFIG_QED_DB_BIT	C0_CONFIG_QED_DB_MSK
#define C0_CONFIG_QED_DB_16	0
#define C0_CONFIG_QED_DB_32	1

#define C0_CONFIG_QED_K0_SHF		0
#define C0_CONFIG_QED_K0_MSK		(MSK(3) << C0_CONFIG_QED_K0_SHF)
#define C0_CONFIG_QED_K0_WTHRU_NOALLOC	0
#define C0_CONFIG_QED_K0_WTHRU_ALLOC	1
#define C0_CONFIG_QED_K0_UNCACHED	2
#define C0_CONFIG_QED_K0_NONCOHERENT	3

#define C0_CONFIG_QED_K0_WTHRU_NOALLOC  0
#define C0_CONFIG_QED_K0_WTHRU_ALLOC 	1
#define C0_CONFIG_QED_K0_UNCACHED 	2
#define C0_CONFIG_QED_K0_NONCOHERENT 	3
#define C0_CONFIG_QED_K0_BYPASS 	7   /* RM70XX specific */

/* RM70XX specifics (SC, SE) fields */
#define C0_CONFIG_QED_RM70XX_SC_SHF	31
#define C0_CONFIG_QED_RM70XX_SC_MSK	(MSK(1) << C0_CONFIG_QED_RM70XX_SC_SHF)
#define C0_CONFIG_QED_RM70XX_SC_BIT	C0_CONFIG_QED_RM70XX_SC_MSK

#define C0_CONFIG_QED_RM70XX_SE_SHF	3
#define C0_CONFIG_QED_RM70XX_SE_MSK	(MSK(1) << C0_CONFIG_QED_RM70XX_SE_SHF)
#define C0_CONFIG_QED_RM70XX_SE_BIT	C0_CONFIG_QED_RM70XX_SE_MSK

/**** STATUS ****/

#define C0_STATUS_QED_CU2_SHF		30
#define C0_STATUS_QED_CU2_MSK		(MSK(1) << C0_STATUS_QED_CU2_SHF)
#define C0_STATUS_QED_CU2_BIT		C0_STATUS_QED_CU2_MSK

#define C0_STATUS_QED_CU1_SHF		29
#define C0_STATUS_QED_CU1_MSK		(MSK(1) << C0_STATUS_QED_CU1_SHF)
#define C0_STATUS_QED_CU1_BIT		C0_STATUS_QED_CU1_MSK

#define C0_STATUS_QED_CU0_SHF		28
#define C0_STATUS_QED_CU0_MSK		(MSK(1) << C0_STATUS_QED_CU0_SHF)
#define C0_STATUS_QED_CU0_BIT		C0_STATUS_QED_CU0_MSK

#define C0_STATUS_QED_FR_SHF		26
#define C0_STATUS_QED_FR_MSK		(MSK(1) << C0_STATUS_QED_FR_SHF)
#define C0_STATUS_QED_FR_BIT		C0_STATUS_QED_FR_MSK

#define C0_STATUS_QED_RE_SHF		25
#define C0_STATUS_QED_RE_MSK		(MSK(1) << C0_STATUS_QED_RE_SHF)
#define C0_STATUS_QED_RE_BIT		C0_STATUS_QED_RE_MSK

#define C0_STATUS_QED_IM_SHF		8
#define C0_STATUS_QED_IM_MSK		(MSK(8) << C0_STATUS_QED_IM_SHF)

#define C0_STATUS_QED_KX_SHF		7
#define C0_STATUS_QED_KX_MSK		(MSK(1) << C0_STATUS_QED_KX_SHF)
#define C0_STATUS_QED_KX_BIT		C0_STATUS_QED_KX_MSK

#define C0_STATUS_QED_SX_SHF		6
#define C0_STATUS_QED_SX_MSK		(MSK(1) << C0_STATUS_QED_SX_SHF)
#define C0_STATUS_QED_SX_BIT		C0_STATUS_QED_SX_MSK

#define C0_STATUS_QED_UX_SHF		5
#define C0_STATUS_QED_UX_MSK		(MSK(1) << C0_STATUS_QED_UX_SHF)
#define C0_STATUS_QED_UX_BIT		C0_STATUS_QED_UX_MSK

#define C0_STATUS_QED_KSU_SHF		3
#define C0_STATUS_QED_KSU_MSK		(MSK(2) << C0_STATUS_QED_KSU_SHF)
#define C0_STATUS_QED_KSU_USER		2
#define C0_STATUS_QED_KSU_SUP		1
#define C0_STATUS_QED_KSU_KERNEL	0

#define C0_STATUS_QED_ERL_SHF		2
#define C0_STATUS_QED_ERL_MSK		(MSK(1) << C0_STATUS_QED_ERL_SHF)
#define C0_STATUS_QED_ERL_BIT		C0_STATUS_QED_ERL_MSK

#define C0_STATUS_QED_EXL_SHF		1
#define C0_STATUS_QED_EXL_MSK		(MSK(1) << C0_STATUS_QED_EXL_SHF)
#define C0_STATUS_QED_EXL_BIT		C0_STATUS_QED_EXL_MSK

#define C0_STATUS_QED_IE_SHF		0
#define C0_STATUS_QED_IE_MSK		(MSK(1) << C0_STATUS_QED_IE_SHF)
#define C0_STATUS_QED_IE_BIT		C0_STATUS_QED_IE_MSK

#define C0_STATUS_QED_DL_SHF		(16 + 8)
#define C0_STATUS_QED_DL_MSK		(MSK(1) << C0_STATUS_QED_DL_SHF)
#define C0_STATUS_QED_DL_BIT		C0_STATUS_QED_DL_MSK

#define C0_STATUS_QED_IL_SHF		(16 + 7)
#define C0_STATUS_QED_IL_MSK		(MSK(1) << C0_STATUS_QED_IL_SHF)
#define C0_STATUS_QED_IL_BIT		C0_STATUS_QED_IL_MSK

#define C0_STATUS_QED_BEV_SHF		(16 + 6)
#define C0_STATUS_QED_BEV_MSK		(MSK(1) << C0_STATUS_QED_BEV_SHF)
#define C0_STATUS_QED_BEV_BIT		C0_STATUS_QED_BEV_MSK

#define C0_STATUS_QED_SR_SHF		(16 + 4)
#define C0_STATUS_QED_SR_MSK		(MSK(1) << C0_STATUS_QED_SR_SHF)
#define C0_STATUS_QED_SR_BIT		C0_STATUS_QED_SR_MSK

#define C0_STATUS_QED_CE_SHF		(16 + 1)
#define C0_STATUS_QED_CE_MSK		(MSK(1) << C0_STATUS_QED_CE_SHF)
#define C0_STATUS_QED_CE_BIT		C0_STATUS_QED_CE_MSK

#define C0_STATUS_QED_DE_SHF		(16 + 0)
#define C0_STATUS_QED_DE_MSK		(MSK(1) << C0_STATUS_QED_DE_SHF)
#define C0_STATUS_QED_DE_BIT		C0_STATUS_QED_DE_MSK


/************************************************************************
 *  Cache
 ************************************************************************/

#define QED_RM5261_ICACHE_SIZE		(32 * 1024)
#define QED_RM5261_ICACHE_LSIZE		 32
#define QED_RM5261_ICACHE_ASSOC		 2
#define QED_RM5261_DCACHE_SIZE		(32 * 1024)
#define QED_RM5261_DCACHE_LSIZE		 32
#define QED_RM5261_DCACHE_ASSOC		 2

#define QED_RM7061A_ICACHE_SIZE		(16 * 1024)
#define QED_RM7061A_ICACHE_LSIZE	 32
#define QED_RM7061A_ICACHE_ASSOC	 4
#define QED_RM7061A_DCACHE_SIZE		(16 * 1024)
#define QED_RM7061A_DCACHE_LSIZE	 32
#define QED_RM7061A_DCACHE_ASSOC	 4

#define QED_RM7061A_L2_CACHE_SIZE	(256 * 1024)
#define QED_RM7061A_L2_CACHE_LSIZE	32
#define QED_RM7061A_L2_CACHE_ASSOC	4

/************************************************************************
 *  CP0 Count 
 ************************************************************************/

#define QED_RM5261_COUNT_CLK_PER_CYCLE	2     /* 2 clocks per increment */
#define QED_RM7061A_COUNT_CLK_PER_CYCLE	2     /* 2 clocks per increment */

/************************************************************************
 *  TLB
 ************************************************************************/

#define QED_RM52XX_TLB_ENTRIES		48

/*  For RM70XX, the number of TLB entries depends on mode bit 24.
 *  bit 24 = 0 -> 48 entries
 *  bit 24 = 1 -> 64 entries
 *  We assume that bit 24 = 0 since this is the case for the
 *  CoreBonito64 board.
 */
#define QED_RM70XX_TLB_ENTRIES		48


#endif /* #ifndef QED_H */

