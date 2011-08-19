
/************************************************************************
 *
 *  cache_cpu.S
 *
 *  Cache functions
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


/************************************************************************
 *  Include files
 ************************************************************************/

#include <mips/asm.h>
//#include <mips/ArchDefs.h>
//#include <mips/SysDefs.h>
#include <mips.h>
#include <init.h>
		
/************************************************************************
 *  Definitions
 ************************************************************************/

#undef DISP_STR
#define DISP_STR(x)

#ifdef __ghs__
#define SET_MIPS3()
#define SET_MIPS0()
#define SET_PUSH()
#define SET_POP()
#else
#define SET_MIPS3() .##set mips3
#define SET_MIPS0() .##set mips0
#define SET_PUSH()  .##set push
#define SET_POP()   .##set pop
#endif

#define KUSEG_MSK		  0x80000000
#define KSEG_MSK		  0xE0000000
#define KUSEGBASE		  0x00000000
#define KSEG0BASE		  0x80000000
#define KSEG1BASE		  0xA0000000
#define KSSEGBASE		  0xC0000000
#define KSEG3BASE		  0xE0000000


/************************************************************************
 *  Public variables
 ************************************************************************/

/************************************************************************
 *  Static variables
 ************************************************************************/

/************************************************************************
 *  Implementation : Public functions
 ************************************************************************/

	.set noreorder
	
/************************************************************************
 *
 *                          sys_init_cache
 *  Description :
 *  -------------
 *
 *  Invalidate I and D caches
 *
 *  input : k1 = processor ID
 *	
 *  Return values :
 *  ---------------
 *
 *  Always 0
 *
 ************************************************************************/
LEAF(sys_init_cache)

	/**** Determine cache settings ****/

	/* This code was linked cached, but is running uncached since
	 * caches have not been initialised yet.
	 */
	
#define RA			t4
#define l2cache_size		t1
#define l2cache_linesize	t0
#define icache_size		t3
#define icache_linesize		t2
#define dcache_size		t1
#define dcache_linesize		t0

	DISP_STR( msg_cache )
	
	move	RA, ra

	move	a0, k1

	/*
	 * MIPSCMP
	 * if v1!=0 this is a secondary CPU,
	 * so don't initialise secondary cache again
	 */
	bnez	v1, 1f
	 nop	

        /* L2 cache */
	bal     sys_determine_l2cache_avail_flash
	nop
	beq	v1, zero, 1f
	nop

	bal	sys_determine_l2cache_linesize_flash
	nop
	move	l2cache_linesize, v0

	bal	sys_determine_l2cache_lines_flash
	nop
	multu	l2cache_linesize, v0
	mflo	l2cache_size

	beq	l2cache_size, zero, 1f
	nop

	/* Initialise L2 */
	DISP_STR( msg_init_l2cache )
	move	a0, l2cache_size
	move	a1, l2cache_linesize
	bal	sys_init_l2cache_flash
	 move	a2, k1
	
	/*
	 * MIPSCMP
	 * by definition only CPU 0 initialises the L2 cache, so
	 * reset the value of v1
	 */
	move	v1, zero

1:
	/* L1 cache */
	
	bal	sys_determine_icache_linesize_flash
	nop
	move	icache_linesize, v0

	bal	sys_determine_icache_lines_flash
	nop
	multu	icache_linesize, v0
	mflo	icache_size

	bal	sys_determine_dcache_linesize_flash
	nop
	move	dcache_linesize, v0

	bal	sys_determine_dcache_lines_flash
	nop
	multu	dcache_linesize, v0
	mflo	dcache_size

	beq	icache_size, zero, 1f
	nop
	beq	dcache_size, zero, 1f
	nop

	/* Initialise instruction cache */
	DISP_STR( msg_init_icache )
	move	a0, icache_size
	move	a1, icache_linesize
	bal	sys_init_icache
	 move	a2, k1
	
	/* Initialise data cache */
	DISP_STR( msg_init_dcache )
	move	a0, dcache_size
	move	a1, dcache_linesize
	bal	sys_init_dcache
	 move	a2, k1

	/* Done */
	jr	RA
	move	v0, zero

	/* D-cache or I-cache has size 0,  set CPU uncached */
1:
	MFC0(   v0, C0_Config )
	and	v0, ~M_ConfigK0
	or	v0, K_CacheAttrU
	MTC0(   v0, C0_Config )

	/* Done */
	jr	RA
	move	v0, zero

END(sys_init_cache)


/************************************************************************
 *
 *                          sys_determine_l2cache_linesize_flash
 *
 *  Description :
 *  -------------
 *  Determine L2 CACHE linesize
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = L2 CACHE linesize in bytes
 *
 ************************************************************************/
LEAF(sys_determine_l2cache_linesize_flash)	

	/* RM70xx? */
	and     t9, a0, M_PRIdCoID|M_PRIdImp
	beq	t9, QED_RM70XX, l2cache_linesize_qed_rm7061a
	 nop

	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), l2cache_linesize_mips
	 nop
	
		
l2cache_linesize_none:
	
	jr	ra
	 move	v0, zero
	
l2cache_linesize_qed_rm7061a:

	li	v0, QED_RM7061A_L2_CACHE_LSIZE
	jr	ra
	 nop

l2cache_linesize_mips:

	/* Read CONFIG1 register to check that CONFIG2 register is available */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )
	and	t9, M_Config1M
	beqz	t9, l2cache_linesize_none
	 nop
	
	/* Read CONFIG2 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config2, R_C0_SelConfig2 )

	/* L2 cache line size */
	and	t9, M_Config2SL
	beqz	t9, l2cache_linesize_none
	 nop

	srl	t9, S_Config2SL
	li	v0, 0x2
	jr	ra
	 sll	v0, t9

END(sys_determine_l2cache_linesize_flash)	


/************************************************************************
 *
 *                          sys_determine_l2cache_lines_flash
 *
 *  Description :
 *  -------------
 *  Determine number of L2 CACHE lines
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = number of L2 CACHE lines
 *
 ************************************************************************/
LEAF(sys_determine_l2cache_lines_flash)	

	/* RM70xx? */
	and     t9, a0, M_PRIdCoID|M_PRIdImp
	beq	t9, QED_RM70XX, l2cache_lines_qed_rm7061a
	 nop
	
	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), l2cache_lines_mips
	 nop

l2cache_lines_none:

	jr	ra
	 move	v0, zero

l2cache_lines_qed_rm7061a:

	li	v0, (QED_RM7061A_L2_CACHE_SIZE/QED_RM7061A_L2_CACHE_LSIZE)
	jr	ra
	 nop

l2cache_lines_mips:

	/* Read CONFIG1 register to check that CONFIG2 register is available */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )
	and	t9, M_Config1M
	beqz	t9, l2cache_lines_none
	 nop
	
	/* Read CONFIG2 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config2, R_C0_SelConfig2 )

	/* S-cache lines
	 * Calculated as associativity * sets per way
	 */
	and	t8, t9, M_Config2SA
	srl	t8, S_Config2SA
	addiu	t8, 1				/* t8 = associativity	*/

	and	t9, M_Config2SS
	srl	t9, S_Config2SS
	li	t7, 0x40
	sll	t7, t9				/* t7 = sets per way	*/

	multu	t8, t7

	jr	ra
	 mflo    v0

END(sys_determine_l2cache_lines_flash)


/************************************************************************
 *
 *                          sys_determine_l2cache_assoc_flash
 *
 *  Description :
 *  -------------
 *  Determine L2 CACHE associativity
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = L2 CACHE associativity
 *
 ************************************************************************/
LEAF(sys_determine_l2cache_assoc_flash)	

	/* RM70xx? */
	and     t9, a0, M_PRIdCoID|M_PRIdImp
	beq	t9, QED_RM70XX, l2cache_assoc_qed_rm7061a
	 nop

	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), l2cache_assoc_mips
	 nop
	
l2cache_assoc_none:
	
	jr	ra
	 move	v0, zero

l2cache_assoc_qed_rm7061a:
	
	li	v0, QED_RM7061A_L2_CACHE_ASSOC
	jr	ra
	 nop

l2cache_assoc_mips:

	/* Read CONFIG1 register to check that CONFIG2 register is available */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )
	and	t9, M_Config1M
	beqz	t9, l2cache_assoc_none
	 nop

	/* Read CONFIG2 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config2, R_C0_SelConfig2 )

	/* L2-cache associativity */
	and	v0, t9, M_Config2SA
	srl	v0, S_Config2SA
	jr	ra
	 addiu	v0, 1

END(sys_determine_l2cache_assoc_flash)


/************************************************************************
 *
 *                          sys_determine_l2cache_avail_flash
 *
 *  Description :
 *  -------------
 *  Determine whether L2 CACHE is available and enabled
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = 1 if L2 CACHE is available, otherwise 0.
 *  v1 = 1 if L2 CACHE is enabled,   otherwise 0.
 *
 ************************************************************************/
LEAF(sys_determine_l2cache_avail_flash)	

	/* RM70xx? */
	and     t9, a0, M_PRIdCoID|M_PRIdImp
	beq	t9, QED_RM70XX, l2cache_avail_qed_rm7061a
	 nop

	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), l2cache_avail_mips
	 nop
	
l2cache_avail_none:
	
	move	v0, zero
	jr	ra
	 move	v1, zero

l2cache_avail_qed_rm7061a:

	li	v0, 1		/* L2 available        */
	jr	ra
	 move	v1, zero	/* Disabled by default */

l2cache_avail_mips:

	/* Read CONFIG1 register to check that CONFIG2 register is available */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )
	and	t9, M_Config1M
	beqz	t9, l2cache_avail_none
	 nop

	/* Read CONFIG2 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config2, R_C0_SelConfig2 )

	/* L2 cache line size */
	and	t9, M_Config2SL
	beqz	t9, 1f
	 move	v0, zero
	li	v0, 1
1:	
	jr	ra
	 move	v1, v0		/* L2 can't be disabled */

END(sys_determine_l2cache_avail_flash)	


/************************************************************************
 *
 *                          sys_determine_icache_linesize_flash
 *  Note :
 *  ------
 *  This routine is called also from syscon.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Determine ICACHE linesize
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = ICACHE linesize in bytes
 *
 ************************************************************************/
LEAF(sys_determine_icache_linesize_flash)	

	/* RM52xx?? */
	and     t9, a0, (M_PRIdCoID|M_PRIdImp)
	beq	t9, QED_RM52XX, icache_linesize_qed_rm5261
	 nop
	/* RM70xx? */
	beq	t9, QED_RM70XX, icache_linesize_qed_rm7061a
	 nop
	
	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), icache_linesize_mips
	 nop
	
	/* Unknown CPU */
icache_linesize_zero:
	jr	ra
	 move	v0, zero

icache_linesize_mips:
	
	/* Read CONFIG1 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )

	/* I-cache line size */
	and	t9, M_Config1IL
	beqz	t9, icache_linesize_zero
	 nop

	srl	t9, S_Config1IL
	li	v0, 0x2
	jr	ra
	 sll	v0, t9

icache_linesize_qed_rm5261:
	li	v0, QED_RM5261_ICACHE_LSIZE
	jr	ra
	 nop

icache_linesize_qed_rm7061a:
	li	v0, QED_RM7061A_ICACHE_LSIZE
	jr	ra
	 nop

END(sys_determine_icache_linesize_flash)	
	


/************************************************************************
 *
 *                          sys_determine_icache_lines_flash
 *  Note :
 *  ------
 *  This routine is called also from syscon.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Determine number of ICACHE lines
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = number of ICACHE lines
 *
 ************************************************************************/
LEAF(sys_determine_icache_lines_flash)	

	/* RM52xx? */
	and     t9, a0, (M_PRIdCoID|M_PRIdImp)
	beq	t9, QED_RM52XX, icache_lines_qed_rm5261
	 nop
	/* RM70xx? */
	beq	t9, QED_RM70XX, icache_lines_qed_rm7061a
	 nop
	
	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), icache_lines_mips
	 nop
	
	/* Unknown CPU */
icache_lines_zero:
	jr	ra
	 move	v0, zero

icache_lines_mips:
	
	/* Read CONFIG1 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )

	/* I-cache lines
	 * Calculated as associativity * sets per way
	 */
	and	t8, t9, M_Config1IA
	srl	t8, S_Config1IA
	addiu	t8, 1				/* t8 = associativity	*/

	and	t9, M_Config1IS
	srl	t9, S_Config1IS

	/* check for 32 ways */
	beq	t9, 0x7, 1f
	li	t7, 32				/* delay slot */

	li	t7, 0x40
	sll	t7, t9				/* t7 = sets per way	*/
1:

	multu	t8, t7

	jr	ra
	 mflo    v0

icache_lines_qed_rm5261:
	li	v0, (QED_RM5261_ICACHE_SIZE / QED_RM5261_ICACHE_LSIZE)
	jr	ra
	 nop

icache_lines_qed_rm7061a:
	li	v0, (QED_RM7061A_ICACHE_SIZE / QED_RM7061A_ICACHE_LSIZE)
	jr	ra
	 nop

END(sys_determine_icache_lines_flash)


/************************************************************************
 *
 *                          sys_determine_icache_assoc_flash
 *  Note :
 *  ------
 *  This routine is called also from syscon.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Determine ICACHE associativity
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = ICACHE associativity
 *
 ************************************************************************/
LEAF(sys_determine_icache_assoc_flash)	

	/* RM50xx? */
	and     t9, a0, M_PRIdCoID|M_PRIdImp
	beq	t9, QED_RM52XX, icache_assoc_qed_rm5261
	 nop
	/* RM70xx? */
	beq	t9, QED_RM70XX, icache_assoc_qed_rm7061a
	 nop
	
	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), icache_assoc_mips
	 nop
	
	/* Unknown CPU */
icache_assoc_zero:
	jr	ra
	 move	v0, zero

icache_assoc_mips:
	
	/* Read CONFIG1 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )

	/* I-cache associativity */
	and	v0, t9, M_Config1IA
	srl	v0, S_Config1IA
	jr	ra
	 addiu	v0, 1

icache_assoc_qed_rm5261:
	li	v0, QED_RM5261_ICACHE_ASSOC
	jr	ra
	nop

icache_assoc_qed_rm7061a:
	li	v0, QED_RM7061A_ICACHE_ASSOC
	jr	ra
	nop

END(sys_determine_icache_assoc_flash)


/************************************************************************
 *
 *                          sys_determine_dcache_linesize_flash
 *  Note :
 *  ------
 *  This routine is called also from syscon.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Determine DCACHE linesize
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = DCACHE linesize in bytes
 *
 ************************************************************************/
LEAF(sys_determine_dcache_linesize_flash)	

	/* RM52xx?? */
	and     t9, a0, (M_PRIdCoID|M_PRIdImp)
	beq	t9, QED_RM52XX, dcache_linesize_qed_rm5261
	 nop
	/* RM70xx? */
	beq	t9, QED_RM70XX, dcache_linesize_qed_rm7061a
	 nop

	/* Check if it is a MIPS32/64 processor */
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), dcache_linesize_mips
	 nop
	
	/* Unknown CPU */
dcache_linesize_zero:
	jr	ra
	 move	v0, zero

dcache_linesize_mips:
	
	/* Read CONFIG1 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )

	/* D-cache line size */
	and	t9, M_Config1DL
	beqz	t9, dcache_linesize_zero
	 nop

	srl	t9, S_Config1DL
	li	v0, 0x2
	jr	ra
	 sll	v0, t9

dcache_linesize_qed_rm5261:
	li	v0, QED_RM5261_DCACHE_LSIZE
	jr	ra
	 nop

dcache_linesize_qed_rm7061a:
	li	v0, QED_RM7061A_DCACHE_LSIZE
	jr	ra
	 nop

END(sys_determine_dcache_linesize_flash)	


/************************************************************************
 *
 *                          sys_determine_dcache_lines_flash
 *  Note :
 *  ------
 *  This routine is called also from syscon.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Determine number of DCACHE lines
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = number of DCACHE lines
 *
 ************************************************************************/
LEAF(sys_determine_dcache_lines_flash)	

	/* RM52xx?? */
	and     t9, a0, (M_PRIdCoID|M_PRIdImp)
	beq	t9, QED_RM52XX, dcache_lines_qed_rm5261
	 nop
	/* RM70xx? */
	beq	t9, QED_RM70XX, dcache_lines_qed_rm7061a
	 nop
	
	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), dcache_lines_mips
	 nop
	
	/* Unknown CPU */
dcache_lines_zero:
	jr	ra
	 move	v0, zero

dcache_lines_mips:
	
	/* Read CONFIG1 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )

	/* D-cache lines
	 * Calculated as associativity * sets per way
	 */
	and	t8, t9, M_Config1DA
	srl	t8, S_Config1DA
	addiu	t8, 1				/* t8 = associativity	*/

	and	t9, M_Config1DS
	srl	t9, S_Config1DS

	/* check for 32 ways */
	beq	t9, 0x7, 1f
	li	t7, 32				/* delay slot */

	li	t7, 0x40
	sll	t7, t9				/* t7 = sets per way	*/
1:

	multu	t8, t7

	jr	ra
	 mflo    v0

dcache_lines_qed_rm5261:
	li	v0, (QED_RM5261_DCACHE_SIZE / QED_RM5261_DCACHE_LSIZE)
	jr	ra
	 nop

dcache_lines_qed_rm7061a:
	li	v0, (QED_RM7061A_DCACHE_SIZE / QED_RM7061A_DCACHE_LSIZE)
	jr	ra
	 nop

END(sys_determine_dcache_lines_flash)


	
/************************************************************************
 *
 *                          sys_determine_dcache_assoc_flash
 *  Note :
 *  ------
 *  This routine is called also from syscon.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Determine DCACHE associativity
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = DCACHE associativity
 *
 ************************************************************************/
LEAF(sys_determine_dcache_assoc_flash)	

	/* RM52xx?? */
	and     t9, a0, (M_PRIdCoID|M_PRIdImp)
	beq	t9, QED_RM52XX, dcache_assoc_qed_rm5261
	 nop
	/* RM70xx? */
	beq	t9, QED_RM70XX, dcache_assoc_qed_rm7061a
	 nop
	
	/* Check if it is a MIPS32/64 processor */	
	and     t9, a0, M_PRIdCoID
	bne	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), dcache_assoc_mips
	 nop
	
	/* Unknown CPU */
dcache_assoc_zero:
	jr	ra
	 move	v0, zero

dcache_assoc_mips:
	
	/* Read CONFIG1 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config1, R_C0_SelConfig1 )

	/* D-cache associativity */
	and	v0, t9, M_Config1DA
	srl	v0, S_Config1DA
	jr	ra
	 addiu	v0, 1

dcache_assoc_qed_rm5261:
	li	v0, QED_RM5261_DCACHE_ASSOC
	jr	ra
	 nop

dcache_assoc_qed_rm7061a:
	li	v0, QED_RM7061A_DCACHE_ASSOC
	jr	ra
	 nop

END(sys_determine_dcache_assoc_flash)

/************************************************************************
 *
 *                          sys_determine_dcache_antialias_flash
 *  Note :
 *  ------
 *  This routine is called also from syscon.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Determine whether CPU has hardware anitialising on Dcache
 *
 *  input : a0 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  v0 = TRUE/FALSE
 *
 ************************************************************************/
LEAF(sys_determine_dcache_antialias_flash)
	
	/* Only some MIPS32/64 processors support dcache HW antialiasing */
	and     t9, a0, M_PRIdCoID
	beq	t9, (C0_PRID_COMP_NOT_MIPS32_64<<S_PRIdCoID), dcache_antialias_found
	 move	v0, zero

	/* Read CONFIG7 register, which holds implementation data */
	MFC0_SEL_OPCODE( R_t9, R_C0_Config7, R_C0_SelConfig7 )
	and	v0, t9, M_Config7AR
	srl	v0, S_Config7AR

dcache_antialias_found:
	jr	ra
	 nop

END(sys_determine_dcache_antialias_flash)

/*
 * Tag register usage for MIPS32/64 processors
 *
 * Proc		Dcache			Icache			L2cache
 * 4K*		taglo(28/0)		taglo(28/0)		-
 * 5K		taglo/hi(28/0)		taglo/hi(29/0)		-
 * 20K		dtaglo/hi(28/2,29/2)	itaglo/hi(28/0,29/0)	-
 * 24K		dtaglo(28/2)		itaglo(28/0)		l23taglo(28/4)
 * 24KE		dtaglo(28/2)		itaglo(28/0)		l23taglo(28/4)
 * 25K		dtaglo/hi(28/2,29/2)	itaglo/hi(28/0,29/0)	l23taglo/hi(28/4,29/4)
 * 34K		dtaglo(28/2)		itaglo(28/0)		l23taglo(28/4)
 * 74K		dtaglo/hi(28/2,29/2)	itaglo/hi(28/0,29/0)	l23taglo(28/4)
 * 1074K	dtaglo/hi(28/2,29/2)	itaglo/hi(28/0,29/0)	l23taglo(28/4)
 * 1004K	dtaglo(28/2)		itaglo(28/0)		l23taglo(28/4)
 */
	
/************************************************************************
 *
 *                          sys_init_l2cache_flash
 *
 *  Description :
 *  -------------
 *  Invalidate entire L2 CACHE
 *
 *  Inputs : a0 = cache size (bytes)
 *           a1 = line size  (bytes)
 *	     a2 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  None
 *
 ************************************************************************/
LEAF( sys_init_l2cache_flash )

	beq   a0, zero, 9f		 /* just in case cache size = 0 */
	 nop

	/*
	 * Mask and shift the processor id so that we can
	 * use xori in the tests
	 */
	and	a2, M_PRIdCoID|M_PRIdImp
	srl	a2, S_PRIdImp
	
	/*
	 * L2 cache support is not defined for all of these processors
	 * so the register selection is base on the registers used for the
	 * primary cache
	 * Hopefully we will never get here unless an L2 cache is present
	 */
#define X(this, last, type) \
	xori	a2, ((MIPS_##last)>>S_PRIdImp)^((MIPS_##this)>>S_PRIdImp); \
	beqz	a2, init_l2cache_##type
#define MIPS_0	0
	X(4Kc,		0,		taglo)
	X(4Kmp,		4Kc,		taglo)
	X(4KEc,		4Kmp,		taglo)
	X(4KEc_R2,	4KEc,		taglo)
	X(4KEmp,	4KEc_R2,	taglo)
	X(4KEmp_R2,	4KEmp,		taglo)
	X(4KSc,		4KEmp_R2,	taglo)
	X(4KSd,		4KSc,		taglo)
	X(5K,		4KSd,		taghilo)
	X(20Kc,		5K,		l23taghilo)
	X(M4K,		20Kc,		taglo)
	X(25Kf,		M4K,		l23taghilo)
	X(5KE,		25Kf,		taghilo)
	X(24K,		5KE,		l23taglo)
	X(24KE,		24K,		l23taglo)
	X(34K,		24KE,		l23taglo)
	X(74K,		34K,		l23taglo)
	X(1004K,	74K,		l23taglo)
	X(1074K,	1004K,		l23taglo)
	 nop
#undef MIPS_0
#undef X

	/* default: Clear TagHI/TagLo */
init_l2cache_taghilo:
	MTC0(   zero, C0_TagHi )
init_l2cache_taglo:	
	MTC0(   zero, C0_TagLo )
	b	0f
	 nop

init_l2cache_l23taghilo:
	MTC0_SEL_OPCODE( R_zero, R_C0_L23TagHi, R_C0_SelL23TagHi )
init_l2cache_l23taglo:
	MTC0_SEL_OPCODE( R_zero, R_C0_L23TagLo, R_C0_SelL23TagLo )

0:
	/* Calc an address that will correspond to the first cache line */
	li	t0, KSEG0BASE
	addu	t1, t0, a0
	subu	t1, a1		/* t1 = last line */

	/* Loop through all lines, invalidating each of them */
1:	
SET_MIPS3()
	cache	SCACHE_INDEX_STORE_TAG, 0(t0)	/* clear tag */
SET_MIPS0()
	bne	t0, t1, 1b
	 addu	t0, a1
9:		
	jr	ra
	 nop
	
END( sys_init_l2cache_flash )

	
/************************************************************************
 *
 *                          sys_init_icache
 *  Note :
 *  ------
 *  This routine is called also from sys_cpu.c and syscon_cpu.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Invalidate entire ICACHE
 *
 *  Inputs : a0 = cache size (bytes)
 *           a1 = line size  (bytes)
 *	
 *  Return values :
 *  ---------------
 *  None
 *
 ************************************************************************/
LEAF( sys_init_icache )
	
#ifdef NO_CACHE

	jr	ra
	nop

#else
		
	beq   a0, zero, 9f		 /* just in case cache size = 0 */
	 nop

	/*
	 * Mask and shift the processor id so that we can
	 * use xori in the tests
	 */
	and	a2, M_PRIdCoID|M_PRIdImp
	srl	a2, S_PRIdImp
	
#define X(this, last, type) \
	xori	a2, ((MIPS_##last)>>S_PRIdImp)^((MIPS_##this)>>S_PRIdImp); \
	beqz	a2, init_icache_##type
#define MIPS_0	0
	X(4Kc,		0,		taglo)
	X(4Kmp,		4Kc,		taglo)
	X(4KEc,		4Kmp,		taglo)
	X(4KEc_R2,	4KEc,		taglo)
	X(4KEmp,	4KEc_R2,	taglo)
	X(4KEmp_R2,	4KEmp,		taglo)
	X(4KSc,		4KEmp_R2,	taglo)
	X(4KSd,		4KSc,		taglo)
	X(5K,		4KSd,		taghilo)
	X(20Kc,		5K,		itaghilo)
	X(M4K,		20Kc,		taglo)
	X(25Kf,		M4K,		itaghilo)
	X(5KE,		25Kf,		taghilo)
	X(24K,		5KE,		itaglo)
	X(24KE,		24K,		itaglo)
	X(34K,		24KE,		itaglo)
	X(74K,		34K,		itaghilo)
	X(1004K,	74K,		itaglo)
	X(1074K,	1004K,		itaghilo)
	 nop
#undef MIPS_0
#undef X
	
	/* default: Clear TagHi/TagLo */

	/* Note: ITagHi/Lo == TagHi/Lo, so we can use common code */
init_icache_itaghilo:
init_icache_taghilo:
	MTC0( zero, C0_TagHi )
init_icache_itaglo:
init_icache_taglo:
	MTC0( zero, C0_TagLo )

0:
	/* Calc an address that will correspond to the first cache line */
	li	a2, KSEG0BASE

	/* Calc an address that will correspond to the last cache line  */
	addu	a3, a2, a0
	subu    a3, a1

	/* Loop through all lines, invalidating each of them */
1:	
SET_MIPS3()
	cache	ICACHE_INDEX_STORE_TAG, 0(a2)	/* clear tag */
SET_MIPS0()
	bne	a2, a3, 1b
	 addu	a2, a1

9:
	jr	ra
	nop

#endif
		
END( sys_init_icache )
		

/************************************************************************
 *
 *                          sys_init_dcache
 *  Note :
 *  ------
 *  This routine is called also from sys_cpu.c
 *  and must obey c calling conventions - and cannot use k0/k1
 *
 *  Description :
 *  -------------
 *  Invalidate entire DCACHE
 *
 *  Inputs : a0 = cache size (bytes)
 *           a1 = line size  (bytes)
 *           a2 = processor ID
 *	
 *  Return values :
 *  ---------------
 *  None
 *
 ************************************************************************/
LEAF( sys_init_dcache )

#ifdef NO_CACHE

	jr	ra
	 nop

#else
		
	/*
	 * Mask and shift the processor id so that we can
	 * use xori in the tests
	 */
	and	a2, M_PRIdCoID|M_PRIdImp
	srl	a2, S_PRIdImp
	
#define X(this, last, type) \
	xori	a2, ((MIPS_##last)>>S_PRIdImp)^((MIPS_##this)>>S_PRIdImp); \
	beqz	a2, init_dcache_##type
#define MIPS_0	0
	X(4Kc,		0,		taglo)
	X(4Kmp,		4Kc,		taglo)
	X(4KEc,		4Kmp,		taglo)
	X(4KEc_R2,	4KEc,		taglo)
	X(4KEmp,	4KEc_R2,	taglo)
	X(4KEmp_R2,	4KEmp,		taglo)
	X(4KSc,		4KEmp_R2,	taglo)
	X(4KSd,		4KSc,		taglo)
	X(5K,		4KSd,		taghilo)
	X(20Kc,		5K,		dtaghilo)
	X(M4K,		20Kc,		taglo)
	X(25Kf,		M4K,		dtaghilo)
	X(5KE,		25Kf,		taghilo)
	X(24K,		5KE,		dtaglo)
	X(24KE,		24K,		dtaglo)
	X(34K,		24KE,		dtaglo)
	X(74K,		34K,		dtaghilo)
	X(1004K,	74K,		dtaglo)
	X(1074K,	1004K,		dtaghilo)
	 nop
#undef MIPS_0
#undef X
	
	/* default: Clear TagHi/TagLo */
init_dcache_taghilo:
	MTC0( zero, C0_TagHi )
init_dcache_taglo:
	MTC0( zero, C0_TagLo )
	b	0f
	 nop
	
init_dcache_dtaghilo:
	MTC0_SEL_OPCODE( R_zero, R_C0_DTagHi, R_C0_SelDTagHi )
init_dcache_dtaglo:
	MTC0_SEL_OPCODE( R_zero, R_C0_DTagLo, R_C0_SelDTagLo )

0:	
	/* Calc an address that will correspond to the first cache line */
	li	a2, KSEG0BASE

	/* Calc an address that will correspond to the last cache line  */
	addu	a3, a2, a0
	subu    a3, a1

	/* Loop through all lines, invalidating each of them */
1:	
SET_MIPS3()
	cache	DCACHE_INDEX_STORE_TAG, 0(a2)	/* clear tag */
SET_MIPS0()
	bne	a2, a3, 1b
	 addu	a2, a1

9:
	jr	ra
	 nop

#endif
		
END( sys_init_dcache )
		
	

/************************************************************************
 *  Implementation : Static functions
 ************************************************************************/

	
/* Messages */

	.text
	
//MSG( msg_cache,        "CACHE"   )
//MSG( msg_init_l2cache, "L2CACHE" )
//MSG( msg_init_icache,  "ICACHE"  )
//MSG( msg_init_dcache,  "DCACHE"  )		

