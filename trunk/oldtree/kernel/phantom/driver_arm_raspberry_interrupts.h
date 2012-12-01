/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2013 Dmitry Zavalishin, dz@dz.ru
 *
 * ARM raspberry PI (bcm2835) interrupts controller.
 *
**/

#ifndef ARM_RASPBERRY_PI_INERRUPT
#define ARM_RASPBERRY_PI_INERRUPT


//#warning 0x7e... is remapped on raspberry?
//#define BCM2835_INTR_REG_BASE    0x7E00B000
#define BCM2835_INTR_REG_BASE    (0x20000000+0xB000)

#define BCM2835_INTR_IRQ_PENDING_BASIC	(BCM2835_INTR_REG_BASE+0x200)
#define BCM2835_INTR_IRQ_PENDING_1	(BCM2835_INTR_REG_BASE+0x204)
#define BCM2835_INTR_IRQ_PENDING_2	(BCM2835_INTR_REG_BASE+0x208)

#define BCM2835_INTR_IRQ_FIQ		(BCM2835_INTR_REG_BASE+0x20C)

#define BCM2835_INTR_IRQ_ENABLE_BASIC	(BCM2835_INTR_REG_BASE+0x218)
#define BCM2835_INTR_IRQ_ENABLE_1	(BCM2835_INTR_REG_BASE+0x210)
#define BCM2835_INTR_IRQ_ENABLE_2	(BCM2835_INTR_REG_BASE+0x214)

#define BCM2835_INTR_IRQ_DISABLE_BASIC	(BCM2835_INTR_REG_BASE+0x224)
#define BCM2835_INTR_IRQ_DISABLE_1	(BCM2835_INTR_REG_BASE+0x21C)
#define BCM2835_INTR_IRQ_DISABLE_2	(BCM2835_INTR_REG_BASE+0x220)


/**
 *
 * Interrupt map:
 *
 * 0-63  - GPU interrupts
 * 64    - ARM Timer interrupt
 * 65    - ARM Mailbox interrupt
 * 66    - ARM Doorbell 0 interrupt
 * 67    - ARM Doorbell 1 interrupt
 * 68    - GPU0 Halted interrupt (Or GPU1)
 * 69    - GPU1 Halted interrupt
 * 70    - Illegal access type-1 interrupt
 * 71    - Illegal access type-0 interrupt
 *
**/

#define BCM2835_GPU_INT_START 0
#define BCM2835_ARM_INT_START 64

#define ARM_INT_TIMER           BCM2835_ARM_INT_START+0
#define ARM_INT_MAILBOX         BCM2835_ARM_INT_START+1

#define ARM_INT_DOORBELL_0      BCM2835_ARM_INT_START+2
#define ARM_INT_DOORBELL_1      BCM2835_ARM_INT_START+3

#define ARM_INT_GPU_HALT_0      BCM2835_ARM_INT_START+4
#define ARM_INT_GPU_HALT_1      BCM2835_ARM_INT_START+5

#define ARM_INT_ILLEGAL_ACC_1   BCM2835_ARM_INT_START+6
#define ARM_INT_ILLEGAL_ACC_0   BCM2835_ARM_INT_START+7


#endif // ARM_RASPBERRY_PI_INERRUPT


