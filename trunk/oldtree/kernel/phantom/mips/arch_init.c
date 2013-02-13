/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * MIPS machdep cpu/system init
 *
**/

#define DEBUG_MSG_PREFIX "arch.init"
#include <debug_ext.h>
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/init.h>
#include <kernel/board.h>
#include <mips/cp0_regs.h>
#include <mips/interrupt.h>
#include <device.h>
#include <endian.h>
#include <assert.h>
#include <hal.h>


static u_int32_t    isa_read32(u_int32_t addr)                      { return *((u_int32_t*)(BOARD_ISA_IO|addr)); }
static void         isa_write32(u_int32_t addr, u_int32_t value)    { *((u_int32_t*)(BOARD_ISA_IO|addr)) = value; }

static u_int16_t    isa_read16(u_int32_t addr)                      { return *((u_int16_t*)(BOARD_ISA_IO|addr)); }
static void         isa_write16(u_int32_t addr, u_int16_t value)    { *((u_int16_t*)(BOARD_ISA_IO|addr)) = value; }

static u_int8_t     isa_read8(u_int32_t addr)                       { return *((u_int8_t*)(BOARD_ISA_IO|addr)); }
static void         isa_write8(u_int32_t addr, u_int8_t value)      { *((u_int8_t*)(BOARD_ISA_IO|addr)) = value; }

// Set from asm code
u_int32_t mips_config_0, mips_config_1, mips_config_2, mips_config_3;


void  mips_read_config_registers(void);

void arch_init_early(void)
{
    isa_bus.read32      = isa_read32;
    isa_bus.write32     = isa_write32;

    isa_bus.read16      = isa_read16;
    isa_bus.write16     = isa_write16;

    isa_bus.read8       = isa_read8;
    isa_bus.write8      = isa_write8;

    unsigned cfg = mips_read_cp0_config();

#if BYTE_ORDER == BIG_ENDIAN
    if( !(cfg&CONF_BE) )
        SHOW_ERROR0( 0, "Big endian kernel on little endian CPU?" );
#else
    if( cfg&CONF_BE )
        SHOW_ERROR0( 0, "Little endian kernel on big endian CPU?" );
#endif

    mips_read_config_registers();

    int arch_revision = (mips_config_0 >> 10) & 0x7;
    printf("Arch rev = %d\n", arch_revision );
    //assert( arch_revision >= 2 );
    if( arch_revision <2 )
        printf("CPU arch rev < 2?\n");

    int mmu_tlb_size = 1+ ( (mips_config_1 >> 25) & 0x3F );

    int mmu_type = (mips_config_0 >> 7) & 0x7;
    printf("MMU type = %d, TLB size = %d \n", mmu_type, mmu_tlb_size );
    assert( mmu_type == 1 );

    int vi_l1_i_cache = mips_config_0 & 0x40;
    printf("I Cache tagged %s\n", vi_l1_i_cache ? "VIRTUAL! :(" : "physical");

    int has_mt = mips_config_3 & 0x4;
    if( has_mt ) printf("Has multithreading support\n");

}

static void timer_interrupt( void *a );
//static int timer_compare_delta = 1000; // Default value of 2000 instructions
static int timer_compare_delta = 400000;
static int timer_compare_value = 0;

static int usec_per_tick = 100000; // 100 Hz = HZ*1000


void board_init_kernel_timer(void)
{
    // dies
#if 1
    int timer_irq = 7;

    // On-chip timer init
    assert(!hal_irq_alloc( timer_irq, timer_interrupt, 0, HAL_IRQ_SHAREABLE ));

    timer_compare_value += timer_compare_delta;
    mips_write_cp0_compare( timer_compare_value );
    mips_write_cp0_count( 0 );
#endif
}


static void timer_interrupt( void *a )
{
    (void) a;

    timer_compare_value += timer_compare_delta;
    mips_write_cp0_compare( timer_compare_value );

    // BUG Dumb mode - just reset both - time will slip this way!
    mips_write_cp0_compare( timer_compare_delta );
    mips_write_cp0_count( 0 );

    hal_time_tick(usec_per_tick);
}


// -----------------------------------------------------------------------
// Usually defined in board, but seems to be arch-wide
// -----------------------------------------------------------------------

int phantom_dev_keyboard_getc(void)
{
    return debug_console_getc();
}

//int phantom_scan_console_getc(void)
int board_boot_console_getc(void)
{
    return debug_console_getc();
}




static void sched_soft_interrupt( void *a )
{
    (void) a;
    // Just do nothing - scheduler will be called from
    // interrupt dispatcher in no-intr state, and req is
    // reset there as well.
}

static int took_zero_intr = 0;

void board_sched_cause_soft_irq(void)
{
    if( !took_zero_intr )
    {
        //board_interrupt_enable(0);
        assert(!hal_irq_alloc( 0, sched_soft_interrupt, 0, HAL_IRQ_SHAREABLE ));
        took_zero_intr = 1;
    }

    // We need to reenable it for thread switch swaps mask too
    board_interrupt_enable(0);

    int ie = hal_save_cli();

    unsigned int cause = mips_read_cp0_cause();
    //#warning set soft int bit 0
    cause |= 1 << 8; // Lower bit - software irq 0
    mips_write_cp0_cause( cause );

    SHOW_FLOW( 8, "softirq cause now %x", cause );

    hal_sti();

    //__asm__("wait"); // Now wait for IRQ
    // sw irq must happen right after sti, right?

    if(ie) hal_sti(); else hal_cli();
}






void arch_interrupt_enable(int irq)
{
    assert_interrupts_disabled();
    assert(irq < MIPS_ONCPU_INTERRUPTS);

    // irq mask in 15:8
    unsigned mask = mips_read_cp0_status();
    mask |= 1 << (irq+8);
    mips_write_cp0_status( mask );
}


void arch_interrupt_disable(int irq)
{
    assert_interrupts_disabled();
    assert(irq < MIPS_ONCPU_INTERRUPTS);

    // irq mask in 15:8
    unsigned mask = mips_read_cp0_status();
    mask &= ~(1 << (irq+8));
    mips_write_cp0_status( mask );
}


void arch_interrupts_disable_all(void)
{
    assert_interrupts_disabled();

    unsigned mask = mips_read_cp0_status();
    mask &= ~(0xFF << 8);
    mips_write_cp0_status( mask );
}


void arch_threads_init()
{
}



//! Stop interrupts, timer, seconary CPUs...
void board_panic_stop_world(void)
{
    board_interrupts_disable_all();
}



//! Wait for a key press on default console - don't use interrupts, assume interrupts disabled
void board_panic_wait_keypress(void)
{
    debug_console_getc();
}





