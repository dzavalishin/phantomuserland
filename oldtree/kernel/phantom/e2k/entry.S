/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Elbrus2000 startup routine.
 *
**/

//#include <multiboot2.h>
//#include <e2k/asm.h>


// modified by <dz@dz.ru> from romstartup.S, which was marked as:
//
// lintel_startup.s, written by <karbo@lab.sun.mcst.ru>
// linux_startup.s, written by <atic@soft.sun.mcst.ru> as modification
// of 'lintel_startup.s' to start up linux secondary module
//

#define WSZ		0x20 			/* 20 quadwords	*/
#define RBS		0x10 			/* 10 quadwords	*/
#define RSZ		0x10 			/* 10 quadwords	*/

#define UPSR		0x1			/* fpu enabled */

#define OSEM_LO		0x0			/* no software traps enabled */
#define OSEM_HI		0x0

.section "text"
.global start
.global loader

// TODO We are starting first steps and call loader(). See romloader.S
// TODO Static memory allocation is defined by romloader.lds
/*
    .globl	_start_of_kernel
_start_of_kernel:
	// TODO really? dz -- someone needs this from C code as _start...
    .globl	__start_of_kernel
__start_of_kernel:
*/

start:
{
	setwd	wsz=WSZ
	setbn	rbs = RBS, rsz = RSZ, rcur = 0x0
	setbp	psz = 0
}
	rws UPSR,%upsr
	disp %ctpr1,loader
 	rwd OSEM_LO,%osem
	ct %ctpr1
	.size	start, . - start


