/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS low level tests. 
 *
**/

//#include <arm/psr.h>
#include <mips/asm.h>

// NB - 32 bits
#define STK_SIZE (64*4)

.set noat

LEAF(mips_test_interrupts_integrity)
    addi        sp, sp, -STK_SIZE

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
    //sw         $29, 29*4(sp) // sp
    sw         $30, 30*4(sp)
    sw         $31, 31*4(sp)


    mfhi t0
    sw          t0, 36*4(sp) // hi
    mflo t0
    sw          t0, 37*4(sp) // lo

    la	t0, __mips_llt_sp
    sw  sp, (t0)
    la	t0, __mips_llt_ra
    sw  ra, (t0)


// -------------------------
// Clear all except for sp and ra


    addiu        $1, zero, 0
    addiu        $2, zero, 0
    addiu        $3, zero, 0
    addiu        $4, zero, 0
    addiu        $5, zero, 0
    addiu        $6, zero, 0
    addiu        $7, zero, 0
    addiu        $8, zero, 0
    addiu        $9, zero, 0
    addiu        $10, zero, 0
    addiu        $11, zero, 0
    addiu        $12, zero, 0
    addiu        $13, zero, 0
    addiu        $14, zero, 0
    addiu        $15, zero, 0
    addiu        $16, zero, 0
    addiu        $17, zero, 0
    addiu        $18, zero, 0
    addiu        $19, zero, 0
    addiu        $20, zero, 0
    addiu        $21, zero, 0
    addiu        $22, zero, 0
    addiu        $23, zero, 0
    addiu        $24, zero, 0
    addiu        $25, zero, 0
    addiu        $26, zero, 0
    addiu        $27, zero, 0
    addiu        $28, zero, 0
    //addiu        $29, zero, 0
    addiu        $30, zero, 0
    //addiu        $31, zero, 0

again:
    addiu        t0, zero, 0
    addiu        t1, zero, 0

    bne          zero, $1, fail
    bne          zero, $2, fail
    bne          zero, $3, fail
    bne          zero, $4, fail
    bne          zero, $5, fail
    bne          zero, $6, fail
    bne          zero, $7, fail
    bne          zero, $8, fail
    bne          zero, $9, fail
    bne          zero, $10, fail
    bne          zero, $11, fail
    bne          zero, $12, fail
    bne          zero, $13, fail
    bne          zero, $14, fail
    bne          zero, $15, fail
    bne          zero, $16, fail
    bne          zero, $17, fail
    bne          zero, $18, fail
    bne          zero, $19, fail
    bne          zero, $20, fail
    bne          zero, $21, fail
    bne          zero, $22, fail
    bne          zero, $23, fail
    bne          zero, $24, fail
    bne          zero, $25, fail
    bne          zero, $26, fail
    bne          zero, $27, fail
    bne          zero, $28, fail
    //bne          zero, $29, fail
    bne          zero, $30, fail
    //bne          zero, $31, fail

    la           t0, __mips_llt_sp
    lw           t1, (t0)
    bne          t1, $29, fail

    la           t0, __mips_llt_ra
    lw           t1, (t0)
    bne          t1, $31, fail




loop:
    la           t0, __mips_llt_count
    lw           t1, (t0)
    addiu        t1, t1, 1
    sw           t1, (t0)
    bne          zero, t1, again
    nop // delay slot


// -------------------------
// Restore and return
// -------------------------

    la	t0, __mips_llt_sp
    lw  sp, (t0)
    la	t0, __mips_llt_ra
    lw  ra, (t0)

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
    //lw         k1, 29*4(sp) // sp
    lw         $30, 30*4(sp)
    lw         $31, 31*4(sp)



    addi       sp, sp, STK_SIZE

    jr         ra
    nop // delay slot


fail:
    la	       a0, __panic_msg
    j          panic

END(mips_test_interrupts_integrity)


.data

__mips_llt_sp:
    .word      0

__mips_llt_ra:
    .word      0

__mips_llt_count:
    .word      100000

__panic_msg:
     .string "integrity test failed\r\n"

