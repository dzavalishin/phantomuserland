/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS startup code.
 *
**/

//#if __mips_isa_rev < 2
//#warning MIPS ISA rev = __mips_isa_rev, need >= 2
//#endif

#include <mips/cp0_regs.h>
#include <mips/asm.h>
#include <mips/pmap.h>
#include <kernel/trap.h>

#ifndef BOARD_ISA_IO
#error BOARD_ISA_IO undefined
#endif

LEAF(__lowmem__)
    .set	noreorder
//    .set	mips32r2

    // Exception vectors
    //.org 0x80000000
    .org 0x000
    //j _enter_exception_tlb_refill
    j _enter_exception_other
    nop // delay slot

    .org 0x180
    j _enter_exception_other
    nop // delay slot

    .org 0x200
    //j _enter_exception_interrupt
    j _enter_exception_other
    nop // delay slot

banner:
    .string "Phantom OS, MIPS Kernel\r\n"
    .string "Copyright (C) Dmitry Zavalishin, 2011.\r\n"

    .align 8

    .globl	_start_of_kernel
_start_of_kernel:
    .globl	_start
_start:

_reset:
    /* Disable interrupts */
    mtc0	zero, CP0_STATUS

    /* Disable watch exception. */
    mtc0    zero, CP0_WATCHLO
    mtc0    zero, CP0_WATCHHI

    /* disable kernel mode cache */
    mfc0	t0, CP0_CONFIG
    and	t0, ~0x7
    ori	t0, 0x2
    mtc0	t0, CP0_CONFIG

    /* set up stack */
    la	sp, __startup_stack_top__

    // Clear BSS
    la t0, __bss_start__
    la t1, __bss_end__
    addiu t3, zero, 0
clear_bss_loop:
    sb t3, 9(t0)
    addiu t0, t0, 1
    bne t0, t1, clear_bss_loop
    nop // delay slot

    // print hello to com port - remove!
    //lui	v0,0xB400
    lui	v0, (BOARD_ISA_IO>>16)
    ori	v0,v0,0x3f8
#if 0
 	li	v1,104
 	sb	v1,0(v0)
 	li	v1,101
 	sb	v1,0(v0)
 	li	v1,108
 	sb	v1,0(v0)
 	sb	v1,0(v0)
 	li	v1,111
 	sb	v1,0(v0)
 	li	v1,33
 	sb	v1,0(v0)
 	li	v1,10
 	sb	v1,0(v0)
 	li	v1,13
 	sb	v1,0(v0)
#endif
    la  t0, banner
print_banner:
    lb  t1, 0(t0)
    addiu t0, t0, 1
    beqz t1, eprint
    nop // delay slot
 	sb	t1, 0(v0)
    j print_banner
    nop // delay slot
eprint:	

    addiu k1, zero, 0 // Init for CPU 0 - means init L2 cache too
    jal sys_init_cache
    nop // delay slot

    // NB - see TLB_PAGEMASK define, use here?
    // pagemask (cp0 r5) = 0 -> 4Kb page
    mtc0	zero, CP0_PAGEMASK

    // wired (cp0 r6) = 24 (half of TLB is wired)?
    // wired (cp0 r6) = 0 (nothing is wired)
    mtc0	zero, CP0_WIRED

    // TODO cp0 r16 config

    /* jump to main */
    jal	phantom_multiboot_main
    nop

loop:
    j	loop
    nop
END(__lowmem__)

//void debug_console_do_putc(int c);
LEAF(debug_console_do_putc)
    lui	t0, (BOARD_ISA_IO>>16)
    ori	t0,t0,0x3f8 // t0 = data reg
    addiu t1, t0, 5 // t1 = status reg

wait_tx:
    lb t2, 0(t1) // read status reg
    andi t2, 0x20 // tx empty bit
    beqz t2, wait_tx
    nop // delay slot

    sb	a0, 0(t0)
    
    jr      ra
    nop // delay slot

END(debug_console_do_putc)

#define PHANTOM_START_STACK_SIZE (64*1024)
    .space	PHANTOM_START_STACK_SIZE
__startup_stack_top__:




LEAF(hal_cli)
    di      v0
    jr.hb   ra
    nop
END(hal_cli)

LEAF(hal_save_cli)
    di      v0
    jr.hb   ra
    andi    v0, v0, ST_IE // delay slot
END(hal_save_cli)

LEAF(hal_is_sti)
    mfc0    v0, CP0_STATUS
    jr.hb   ra
    andi    v0, v0, ST_IE // delay slot
END(hal_is_sti)


LEAF(hal_sti)
    ei      v0
    jr.hb   ra
    andi    v0, v0, ST_IE // delay slot
END(hal_sti)

LEAF(hal_wait_for_interrupt)
    wait
    nop // need?
    jr      ra
    nop
END(hal_wait_for_interrupt)


LEAF(arch_get_frame_pointer)
    jr      ra
    addiu   v0, fp, 0
END(arch_get_frame_pointer)



    //int mips_read_cp0_status();
LEAF(mips_read_cp0_status)
    mfc0    v0, CP0_STATUS
    jr.hb   ra
    nop // delay slot
END(mips_read_cp0_status)

    //void mips_write_cp0_status( int data );
LEAF(mips_write_cp0_status)
    mtc0    a0, CP0_STATUS
    jr.hb   ra
    nop // delay slot
END(mips_write_cp0_status)


    //int mips_read_cp0_cause();
LEAF(mips_read_cp0_cause)
    mfc0    v0, CP0_CAUSE
    jr.hb   ra
    nop // delay slot

END(mips_read_cp0_cause)

    //void mips_write_cp0_cause( int data );
LEAF(mips_write_cp0_cause)
    mtc0    a0, CP0_CAUSE
    jr.hb   ra
    nop // delay slot
END(mips_write_cp0_cause)


// get pr id = (cp0 r15)
//int mips_read_cp0_cpuid();
LEAF(mips_read_cp0_cpuid)
    jr      ra
    mfc0    v0, CP0_PRID // delay slot
END(mips_read_cp0_cpuid)


    //void mips_write_cp0_compare( unsigned int data );
LEAF(mips_write_cp0_compare)
    mtc0    a0, CP0_COMPARE
    jr.hb   ra
    nop // delay slot
END(mips_write_cp0_compare)

    //void mips_write_cp0_count( unsigned int data );
LEAF(mips_write_cp0_count)
    mtc0    a0, CP0_COUNT
    jr.hb   ra
    nop // delay slot
END(mips_write_cp0_count)





// int atomic_add(int *val, int incr) 
LEAF(atomic_add)
_atomic_add_again:
    ll      t0, 0(a0)
    addu    t1, t0, a2
    addiu   v0, t1, 0 // ret val
    sc      t1, 0(a0)
    beqz    t1, _atomic_add_again

    jr      ra
    nop
END(atomic_add)


/**
 *
 * curr_thread_k0_stack_top points to topmost address of current
 * thread's kernel stack. We use word at this address - 4 to count
 * entries to exception handler. So we load real sp with (top-4) & ~7
 * to make sure we don't touch top count word and have sp aligned for
 * MIPS rules
 *
**/

NESTED(_enter_exception_other,0,sp)
    .set	push
    .set	noat

    // switch to kernel stack, if needed

    la k0, curr_thread_k0_stack_top
    lw k0, 0(k0) // get var value
    addiu k0, k0, -4 // step back to access counter
    lw k1, 0(k0) // get count value

    // not zero? ok, we already have good k0 stack, can proceed 
    bnez k1, on_kernel_sp
    nop // delay slot

first_exception:
    addiu k1, sp, 0 // put user sp to k1
    //andi k0, k0, 0xFFFFFFF8 // clear lower 3 bits of new sp - align for MIPS
    sra k0, k0, 3
    sll k0, k0, 3
    addiu sp, k0, 0 // set kernel stack

    j good_k1
    nop // delay slot

on_kernel_sp:
    addiu k1, sp, 0 // put sp to k1 for consistency

good_k1:
    // push user state

    addi        sp, sp, -TRAP_STATE_SIZE

    sw          $0,  0*4(sp)
    sw          $1,  1*4(sp)
    sw          $2,  2*4(sp)
    sw          $3,  3*4(sp)

    sw          $4,  4*4(sp)
    sw          $5,  5*4(sp)
    sw          $6,  6*4(sp)
    sw          $7,  7*4(sp)

    sw          $8,  8*4(sp)
    sw          $9,  9*4(sp)
    sw         $10, 10*4(sp)
    sw         $11, 11*4(sp)

    sw         $12, 12*4(sp)
    sw         $13, 13*4(sp)
    sw         $14, 14*4(sp)
    sw         $15, 15*4(sp)

    sw         $16, 16*4(sp)
    sw         $17, 17*4(sp)
    sw         $18, 18*4(sp)
    sw         $19, 19*4(sp)

    sw         $20, 20*4(sp)
    sw         $21, 21*4(sp)
    sw         $22, 22*4(sp)
    sw         $23, 23*4(sp)

    sw         $24, 24*4(sp)
    sw         $25, 25*4(sp)
    sw         $26, 26*4(sp)
    sw         $27, 27*4(sp)

    sw         $28, 28*4(sp)
    sw         k1, 29*4(sp) // saved to k1 above
    sw         $30, 30*4(sp)
    sw         $31, 31*4(sp)

    mfc0	t0, CP0_BADVADDR        // Offending virual address, if any
    sw          t0, 32*4(sp)
    mfc0	t0, CP0_STATUS          // CPU status word
    sw          t0, 33*4(sp)
    mfc0	t0, CP0_EPC             // Exception program counter
    sw          t0, 34*4(sp)
    mfc0	t0, CP0_ERROREPC        // Error program counter
    sw          t0, 35*4(sp)

    mfhi t0
    sw          t0, 36*4(sp) // hi
    mflo t0
    sw          t0, 37*4(sp) // lo

    // inc exception entry count!
    la k0, curr_thread_k0_stack_top
    lw k0, 0(k0) // get var value
    addiu k0, k0, -4 // step back to access counter
    lw k1, 0(k0) // get count value
    addiu k1, k1, 1 // inc exception count
    sw k1, 0(k0) // set count value

    // Go to kernel mode now. We'll return to exception mode on status reset below
    mfc0	t0, CP0_STATUS          // get CPU status word
    li          t1, ST_IE|ST_EXL|ST_UM  // Flags to reset in status - intr enable, in exception, user mode. TODO supervisor mode
    not         t1                      // Invert
    and         t0, t0, t1              // Reset 'em
    mtc0	t0, CP0_STATUS          // set CPU status word

    // TODO we can set k0 and k1 to some widely used values
    // such as cpu number, thread struct, etc

    //void hal_MIPS_exception_dispatcher(struct trap_state *ts, int cause)
    addiu       a0, sp, 0               // move a0 <- sp
    mfc0	a1, CP0_CAUSE           // Cause - incl interrupt num and exception type
    jal hal_MIPS_exception_dispatcher
    nop // delay

    // dec exception entry count!
    la k0, curr_thread_k0_stack_top
    lw k0, 0(k0) // get var value
    addiu k0, k0, -4 // step back to access counter
    lw k1, 0(k0) // get count value
    addiu k1, k1, -1 // inc exception count
    sw k1, 0(k0) // set count value

    // Restore all the regs

    //lw          t0, 32*4(sp)
    //mtc0	t0, CP0_BADVADDR        // Offending virual address, if any

    lw          t0, 33*4(sp)
    mtc0	t0, CP0_STATUS          // Side effect: Return to exception mode

    lw          t0, 34*4(sp)
    mtc0	t0, CP0_EPC             // Exception program counter

    lw          t0, 35*4(sp)
    mtc0	t0, CP0_ERROREPC        // Error program counter

    lw          t0, 36*4(sp) // hi
    mthi t0
    lw          t0, 37*4(sp) // lo
    mtlo t0

    lw          $0,  0*4(sp)
    lw          $1,  1*4(sp)
    lw          $2,  2*4(sp)
    lw          $3,  3*4(sp)

    lw          $4,  4*4(sp)
    lw          $5,  5*4(sp)
    lw          $6,  6*4(sp)
    lw          $7,  7*4(sp)

    lw          $8,  8*4(sp)
    lw          $9,  9*4(sp)
    lw         $10, 10*4(sp)
    lw         $11, 11*4(sp)

    lw         $12, 12*4(sp)
    lw         $13, 13*4(sp)
    lw         $14, 14*4(sp)
    lw         $15, 15*4(sp)

    lw         $16, 16*4(sp)
    lw         $17, 17*4(sp)
    lw         $18, 18*4(sp)
    lw         $19, 19*4(sp)

    lw         $20, 20*4(sp)
    lw         $21, 21*4(sp)
    lw         $22, 22*4(sp)
    lw         $23, 23*4(sp)

    lw         $24, 24*4(sp)
    lw         $25, 25*4(sp)
    lw         $26, 26*4(sp)
    lw         $27, 27*4(sp)

    lw         $28, 28*4(sp)
    //lw         k1, 29*4(sp) // saved to k1 so that we can use sp below
    lw         $30, 30*4(sp)
    lw         $31, 31*4(sp)

    lw         $29, 29*4(sp) // pop sp last

    //addi        sp, sp, TRAP_STATE_SIZE
    //addi        sp, k1, 0 // just restore old sp


    eret
    nop // no delay slot after eret?

    .set	pop
END(_enter_exception_other)










//  int mips_tlb_read( int index, struct mips_pt_entry *e );
//! returns tlbhi
LEAF(mips_tlb_read)
    mtc0    a0, CP0_INDEX

    ehb
    tlbr
    ehb

    mfc0    t0, CP0_ENTRYLO0
    mfc0    t1, CP0_ENTRYLO1
    mfc0    v0, CP0_ENTRYHI

    sw v0, MIPS_PT_ENTRY_V_OFF(a1) // mips_pt_entry.v
    sw t0, MIPS_PT_ENTRY_P0_OFF(a1) // mips_pt_entry.p0
    sw t1, MIPS_PT_ENTRY_P1_OFF(a1) // mips_pt_entry.p1
    // ignore pagemask

    jr.hb   ra
    nop // delay slot
END(mips_tlb_read)

#if 1
//  int mips_tlb_probe( struct mips_pt_entry *e );
//! Returns contents of Index reg after probe - top bit is failure to find TLB record
LEAF(mips_tlb_probe)

    // TODO check if this instruction works as expected here
    lw t0, MIPS_PT_ENTRY_P0_OFF(a0) // mips_pt_entry.p0
    lw t1, MIPS_PT_ENTRY_P1_OFF(a0) // mips_pt_entry.p1
    lw t2, MIPS_PT_ENTRY_V_OFF(a0) // mips_pt_entry.v

    mtc0    t0, CP0_ENTRYLO0
    mtc0    t1, CP0_ENTRYLO1
    mtc0    t2, CP0_ENTRYHI

    // NB - see TLB_PAGEMASK define, use here?
    // pagemask (cp0 r5) = 0 -> 4Kb page
    mtc0	zero, CP0_PAGEMASK

    ehb
    tlbp
    ehb

    mfc0    v0, CP0_INDEX

    jr.hb   ra
    nop // delay slot
END(mips_tlb_probe)
#endif

//  void mips_tlb_write_index(int index, struct mips_pt_entry *e);
LEAF(mips_tlb_write_index)

    mtc0    a0, CP0_INDEX

    lw t0, MIPS_PT_ENTRY_P0_OFF(a1) // mips_pt_entry.p0
    lw t1, MIPS_PT_ENTRY_P1_OFF(a1) // mips_pt_entry.p1
    lw t2, MIPS_PT_ENTRY_V_OFF(a1) // mips_pt_entry.v

    mtc0    t0, CP0_ENTRYLO0
    mtc0    t1, CP0_ENTRYLO1
    mtc0    t2, CP0_ENTRYHI

    // NB - see TLB_PAGEMASK define, use here?
    // pagemask (cp0 r5) = 0 -> 4Kb page
    mtc0	zero, CP0_PAGEMASK

    ehb
    tlbwi

    jr.hb   ra
    nop // delay slot
END(mips_tlb_write_index)

//  void mips_tlb_write_random( struct mips_pt_entry *e );
LEAF(mips_tlb_write_random)
    //mtc0    a0, CP0_INDEX

    lw t0, MIPS_PT_ENTRY_P0_OFF(a0) // mips_pt_entry.p0
    lw t1, MIPS_PT_ENTRY_P1_OFF(a0) // mips_pt_entry.p1
    lw t2, MIPS_PT_ENTRY_V_OFF(a0) // mips_pt_entry.v

    mtc0    t0, CP0_ENTRYLO0
    mtc0    t1, CP0_ENTRYLO1
    mtc0    t2, CP0_ENTRYHI

    // NB - see TLB_PAGEMASK define, use here?
    // pagemask (cp0 r5) = 0 -> 4Kb page
    mtc0	zero, CP0_PAGEMASK

    ehb
    tlbwr

    jr.hb   ra
    nop // delay slot
END(mips_tlb_write_random)
































