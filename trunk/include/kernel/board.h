/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * Board (specific hardware configuration) mappings.
 *
 *
**/

//! The first function called by init code at all.
//! At this point all we have is stack. Do what is
//! needed earliest to prepare for later startup
//! code. Such as enable hardware components, correct
//! memory access, etc etc.
void board_init_early(void);


//! Init what is needed to management of cpu state, such
//! as descriptor tables, etc.
void board_init_cpu_management(void);



//! This must bring alive interrupts from main OS timer
//! providing about 100HZ calls to hal_time_tick().
//! Drives timed calls, sleep_msec, scheduling and date/time.

void board_init_kernel_timer(void);



//! Init default interupts hardware
void board_init_interrupts(void);

//! Call board-specific interrupt line enable function
void board_interrupt_enable(int irq);

//! Call board-specific interrupt line disable function
void board_interrupt_disable(int irq);

//! Disable all the possible interrupt lines. Not cli!
void board_interrupts_disable_all(int irq);


//! Start other SMP processors and corresponding infrastructure
void board_start_smp(void);
