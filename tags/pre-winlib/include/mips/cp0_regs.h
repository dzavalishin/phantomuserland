/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS coprocessor 0 regs. 
 *
**/

#ifndef MIPS_CP0_REGS
#define MIPS_CP0_REGS

#define CP0_INDEX       $0
#define CP0_RANDOM      $1
#define CP0_ENTRYLO0    $2
#define CP0_ENTRYLO1    $3
#define CP0_CONTEXT     $4
#define CP0_PAGEMASK    $5
#define CP0_WIRED       $6
#define CP0_BADVADDR    $8
#define CP0_COUNT       $9
#define CP0_ENTRYHI     $10
#define CP0_COMPARE     $11
#define CP0_STATUS      $12
#define CP0_CAUSE       $13
#define CP0_EPC         $14
#define CP0_PRID        $15
#define CP0_CONFIG      $16
#define CP0_LLADDR      $17
#define CP0_WATCHLO     $18
#define CP0_WATCHHI     $19
#define CP0_XCONTEXT    $20
#define CP0_FRAMEMASK   $21
#define CP0_DIAGNOSTIC  $22
#define CP0_PERFORMANCE $25
#define CP0_ECC         $26
#define CP0_CACHEERR    $27
#define CP0_TAGLO       $28
#define CP0_TAGHI       $29
#define CP0_ERROREPC    $30

/*
 * Status register.
 */
#define ST_IE		0x00000001	/* intr enable */
#define ST_EXL		0x00000002	/* exception level */
#define ST_ERL		0x00000004	/* error level */

#define ST_SM		0x00000008	/* supervisor mode */
#define ST_UM		0x00000010	/* user mode */

#define ST_UX		0x00000020	/* user 64 bits */
#define ST_SX		0x00000040	/* supervisor 64 bits */
#define ST_KX		0x00000080	/* kernel (CPU) 64 bits */

#define ST_IM_SW0	0x00000100	/* soft irq 0 */
#define ST_IM_SW1	0x00000200	/* soft irq 1 */
#define ST_IM       0x0000FF00	/* int mask */

//#define ST_DIAG		0x01FF0000	/* diag status */

//#define ST_DE		0x00020000	/* disable ECC */
//#define ST_CE		0x00040000	/* cache ECC */
#define ST_NMI		0x00080000	/* reset cause - NMI */

//#define ST_SR		0x00100000	/* soft reset */
#define ST_TS		0x00200000	/* TLB shutdown */
//#define ST_BEV		0x00400000	/* размещение векторов: начальная загрузка */

#define ST_RE		(1<<25)     /* reverse endian */
#define ST_FR		(1<<26)     /* 32 FP regs */
#define ST_RP		(1<<27)     /* reduced power */

#define ST_CU0		0x10000000	/* разрешение сопроцессора 0 */
#define ST_CU1		0x20000000	/* разрешение сопроцессора 1 (FPU) */
#define ST_CU2		0x40000000	/* разрешение сопроцессора 2 */
#define ST_CU3		0x80000000	/* разрешение сопроцессора 3 */


#define CP0_STATUS_DEFAULT (ST_CU0|ST_CU1)

// Config register

//! kseg0 coherency algorithm
//! 2 - Uncached
//! 3 - Cacheable noncoherent (noncoherent)
//! 4 - Cacheable coherent exclusive (exclusive)
//! 5 - Cacheable coherent exclusive on write (sharable)
//! 6 - Cacheable coherent update on write (update)
#define CONF_K0_MASK	0x7

//! Update on Store Conditional
//! 0 - Store Conditional uses coherency algorithm specified by TLB
//! 1 - SC uses cacheable coherent update on write
#define CONF_CU         (1<<3)

//! Primary D-cache line size, 0 - 16 bytes, 1 - 32 bytes
#define CONF_DB         (1<<4)

//! Primary I-cache line size, 0 - 16 bytes, 1 - 32 bytes
#define CONF_IB         (1<<5)

//! Primary D-cache Size (D-cache size = 2**12+IC bytes)
#define CONF_DC_SHIFT   6
#define CONF_DC_MASK    0x7

//! Primary I-cache Size (I-cache size = 2**12+IC bytes)
#define CONF_IC_SHIFT   9
#define CONF_IC_MASK    0x7

#define CONF_EB         (1<<13)

//! ECC mode enabled
#define CONF_EM         (1<<14)

//! Kernel and memory are big endian
#define CONF_BE         (1<<15)

//! Dirty Shared coherency disabled
#define CONF_SM         (1<<16)

//! Secondary Cache present
#define CONF_SC         (1<<17)

#define CONF_EW_SHIFT   18
#define CONF_EW_MASK    0x3

//! Secondary Cache port width
#define CONF_SW         (1<<20)

//! Split secondary cache
#define CONF_SS         (1<<21)

//! Secondary Cache line size (4-8-16-32 words)
#define CONF_SB_SHIFT   22
#define CONF_SB_MASK    0x3

//! Transmit data pattern (pattern for write-back data)
#define CONF_EP_SHIFT   24
#define CONF_EP_MASK    0xF

//! System clock
#define CONF_EC_SHIFT   28
#define CONF_EC_MASK    0x7

//! Div by 2
#define CONF_EC_2       0

//! master-checker mode
#define CONF_CM         (1<<31)


#ifndef ASSEMBLER

unsigned int mips_read_cp0_status( void );
void         mips_write_cp0_status( unsigned int data );

unsigned int mips_read_cp0_cause();
void         mips_write_cp0_cause( unsigned int data );

unsigned int mips_read_cp0_config(void);

unsigned int mips_read_cp0_cpuid( void );

// Timer
unsigned int mips_read_cp0_count(void);
void         mips_write_cp0_count( unsigned int data );

unsigned int mips_read_cp0_compare();
void         mips_write_cp0_compare( unsigned int data );


#endif // ASSEMBLER



#endif // MIPS_CP0_REGS

