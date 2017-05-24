/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2017 Dmitry Zavalishin, dz@dz.ru
 *
 * Elbrus 2000 traps support.
 *
**/

#include <e2k/proc_reg.h>
//#include <e2k/debug_reg.h>
//#include <e2k/eflags.h>

#include <kernel/trap.h>

#include <phantom_assert.h>
#include <phantom_libc.h>

//#include <signal.h>



/*
static char *trap_type[] = {
    "Divide error", "Debug trap",		"NMI",		"Breakpoint",
    "Overflow",	    "Bounds check",		"Invalid opcode","No coprocessor",
    "Double fault", "Coprocessor overrun",	"Invalid TSS",	"Segment not present",
    "Stack bounds", "General protection",	"Page fault",	"(reserved)",
    "Coprocessor error"
};
#define TRAP_TYPES (sizeof(trap_type)/sizeof(trap_type[0]))
*/
const char *trap_name(unsigned int trapnum)
{
    //return trapnum < TRAP_TYPES ? trap_type[trapnum] : "(unknown)";
    return "(unknown)";
}







extern int panic_reenter;



void dump_ss(struct trap_state *st)
{
    if(panic_reenter > 1)
        return;
/*
    int from_user = (st->cs & 3) || (st->eflags & EFL_VM);

    printf("Dump of i386 state:\n");
    printf("EAX %08x EBX %08x ECX %08x EDX %08x\n",
           st->eax, st->ebx, st->ecx, st->edx);
    printf("ESI %08x EDI %08x EBP %08x ESP %08x\n",
           st->esi, st->edi, st->ebp,
           from_user ? st->esp : (unsigned)&st->esp
          );
    printf("CS %04x SS %04x DS %04x ES %04x FS %04x GS %04x\n",
           st->cs & 0xffff, from_user ? st->ss & 0xffff : get_ss(),
           st->ds & 0xffff, st->es & 0xffff,
           st->fs & 0xffff, st->gs & 0xffff);
    if (st->eflags & EFL_VM)
    {
        printf("v86:            DS %04x ES %04x FS %04x GS %04x\n",
               st->v86_ds & 0xffff, st->v86_es & 0xffff,
               st->v86_gs & 0xffff, st->v86_gs & 0xffff);
    }
    printf("EIP %08x EFLAGS %08x\n", st->eip, st->eflags);
    printf("trapno %d: %s, error %08x\n",
           st->trapno, trap_name(st->trapno),
           st->err);
    if (st->trapno == T_PAGE_FAULT)
        printf("page fault linear address %08x\n", st->cr2);

    if(phantom_symtab_getname)
    {
        printf("EIP %6p: %s\n", (void *)st->eip, phantom_symtab_getname((void *)st->eip) ); // TODO 64 bit machdep
    }


    //if(!from_user)
        stack_dump_from((void *)st->ebp);
*/
}







static int trap_ignore(struct trap_state *ts)
{
    (void)ts;
    return 0;
}


//#define NO_HANDLER trap_panic
#define NO_HANDLER 0

/**
 *
 * If trap handler returns nonzero, panic will be called.
 *
**/

int (*phantom_trap_handlers[ARCH_N_TRAPS])(struct trap_state *ts) = {
	NO_HANDLER, /* 0 */
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER, /* 7 */
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	trap_ignore,  // 15 spurios traps, not really used
//	NO_HANDLER,  // 15 spurios traps, not really used
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER, /* 23 */
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER,
	NO_HANDLER  /* 31 */
};




int trap2signo( struct trap_state *ts )
{
#if 0
    int		sig_no = 0;

    /*
     * It probably makes no sense to pass all these signals to the
     * application program.
     */
    switch (ts->trapno)
    {

    case -1: 	sig_no = SIGINT;  break;	/* hardware interrupt */
    case 0: 	sig_no = SIGFPE;  break;	/* divide by zero */
    case 1: 	sig_no = SIGTRAP; break;	/* debug exception */
    case 3: 	sig_no = SIGTRAP; break;	/* breakpoint */
    case 4: 	sig_no = SIGFPE;  break;	/* overflow */
    case 5: 	sig_no = SIGFPE;  break;	/* bound instruction */
    case 6: 	sig_no = SIGILL;  break;	/* Invalid opcode */
    case 7: 	sig_no = SIGFPE;  break;	/* coprocessor not available */
    case 9: 	sig_no = SIGBUS;  break;	/* coproc segment overrun*/
    case 10: 	sig_no = SIGBUS;  break;	/* Invalid TSS */
    case 11: 	sig_no = SIGSEGV; break;	/* Segment not present */
    case 12: 	sig_no = SIGSEGV; break;	/* stack exception */
    case 13: 	sig_no = SIGSEGV; break;	/* general protection */
    case 14: 	sig_no = SIGSEGV; break;	/* page fault */
    case 16: 	sig_no = SIGFPE;  break;	/* coprocessor error */

    default:
        panic("No signal mapping for trap number: %d", ts->trapno);
    }

    /*
     * Look for null pointer exceptions ...
     */
    if(ts->trapno == 1)
    {
        unsigned dr6 = rdr6();

        if (dr6 & (DR6_B0|DR6_B1))
            sig_no = SIGSEGV;
        else
            sig_no = SIGTRAP;
    }

    return sig_no;
#else
    panic("No signal mapping for trap numbers");
#endif
}














/*********************************************************************/

// Use system call entry SCALL 12 as a kernel jumpstart.

#ifdef HAVE_SMP
static atomic_t __initdata boot_bss_cleaning_finished = ATOMIC_INIT(0);
#endif

void  notrace __ttable_entry12__ ttable_entry12(
                                                int n,
                                                bootblock_struct_t *bootblock
                                               )
{
    boot_info_t	*boot_info = NULL;
    u16     	signature;
#ifndef	CONFIG_E2K_MACHINE
    e2k_upsr_t	upsr;
    int		simul_flag;
    int		iohub_flag;
    int		mach_id = 0;
    int		virt_mach_id = 0;
    unsigned char	cpu_type;
#endif /* ! CONFIG_E2K_MACHINE */

    /* CPU will stall if we have unfinished memory operations.
     * This shows bootloader problems if they present */
    E2K_WAIT_ALL;

    /* Set current pointers to 0 to indicate that
     * current_thread_info() is not ready yet */
    E2K_SET_DGREG_NV(16, 0);
    E2K_SET_DGREG_NV(17, 0);
    /* percpu shift for the BSP processor is 0 */
    E2K_SET_DGREG_NV(18, 0);
    /* Initial CPU number for the BSP is 0 */
    E2K_SET_DGREG_NV(19, 0);

    /* Clear BSS ASAP */

    if (IS_BOOT_STRAP_CPU()) {
        void *bss_p = boot_vp_to_pp(__bss_start);
        unsigned long size = (unsigned long) __bss_stop -
            (unsigned long) __bss_start;
        do_boot_printk("Kernel BSS segment will be cleared from physical address 0x%p size 0x%lx\n",
                           bss_p, size);
            recovery_memset_8(bss_p, 0, 0, size,
                              LDST_DWORD_FMT << LDST_REC_OPC_FMT_SHIFT |
                              MAS_STORE_PA << LDST_REC_OPC_MAS_SHIFT);
#ifdef HAVE_SMP
            boot_set_event(&boot_bss_cleaning_finished);
        } else {
            boot_wait_for_event(&boot_bss_cleaning_finished);
#endif
        }


    EARLY_BOOT_TRACEPOINT("Phantom kernel entered");

    if (IS_BOOT_STRAP_CPU())
        EARLY_BOOT_TRACEPOINT("kernel boot-time init started");

    if( IS_BOOT_STRAP_CPU() ) {
#ifdef	CONFIG_E2K_MACHINE
#if defined(CONFIG_E2K_E3M_SIM)
        boot_e3m_lms_setup_arch();
#elif defined(CONFIG_E2K_E3M)
        boot_e3m_setup_arch();
#elif defined(CONFIG_E2K_E3M_IOHUB_SIM)
        boot_e3m_iohub_lms_setup_arch();
#elif defined(CONFIG_E2K_E3M_IOHUB)
        boot_e3m_iohub_setup_arch();
#elif defined(CONFIG_E2K_E3S_SIM)
        boot_e3s_lms_setup_arch();
#elif defined(CONFIG_E2K_E3S)
        boot_e3s_setup_arch();
#elif defined(CONFIG_E2K_ES2_DSP_SIM) || defined(CONFIG_E2K_ES2_RU_SIM)
        boot_es2_lms_setup_arch();
#elif defined(CONFIG_E2K_ES2_DSP) || defined(CONFIG_E2K_ES2_RU)
        boot_es2_setup_arch();
#elif defined(CONFIG_E2K_E2S_SIM)
        boot_e2s_lms_setup_arch();
#elif defined(CONFIG_E2K_E2S)
        boot_e2s_setup_arch();
#elif defined(CONFIG_E2K_E8C_SIM)
        boot_e8c_lms_setup_arch();
#elif defined(CONFIG_E2K_E8C)
        boot_e8c_setup_arch();
#elif defined(CONFIG_E2K_E1CP_SIM)
        boot_e1cp_lms_setup_arch();
#elif defined(CONFIG_E2K_E1CP)
        boot_e1cp_setup_arch();
#elif defined(CONFIG_E2K_E8C2_SIM)
        boot_e8c2_lms_setup_arch();
#elif defined(CONFIG_E2K_E8C2)
        boot_e8c2_setup_arch();
#else
#    error "E2K MACHINE type is not defined"
#endif
#else	/* ! CONFIG_E2K_MACHINE */
        bool e3m;

        simul_flag = bootblock->info.mach_flags & SIMULATOR_MACH_FLAG;
        iohub_flag = bootblock->info.mach_flags & IOHUB_MACH_FLAG;
        cpu_type = bootblock->info.bios.cpu_type;
        if (simul_flag)
            mach_id |= MACHINE_ID_SIMUL;
        if (iohub_flag)
            mach_id |= MACHINE_ID_E2K_IOHUB;

        upsr = read_UPSR_reg();
        if (AS_WORD(upsr) & (UPSR_FSM | UPSR_IMPT | UPSR_IUC)) {
            /* Only E3S, ES2, E2S E8C E1C+ E8C2 CPUs have these */
            /* bits */
            e3m = false;
        } else {
            WRITE_UPSR_REG_VALUE(AS_WORD(upsr) | UPSR_FSM);
            if (READ_UPSR_REG_VALUE() & (UPSR_FSM)) {
                /* Only E3S, ES2, E2S E8C E1C+ E8C2 CPUs have */
                /* these bits */
                write_UPSR_reg(upsr);
                e3m = false;
            } else {
                e3m = true;
            }
        }

        if (e3m) {
            mach_id |= MACHINE_ID_E3M;
            boot_machine_id = mach_id;
            if (mach_id == MACHINE_ID_E3M_LMS) {
                boot_e3m_lms_setup_arch();
            } else if (mach_id == MACHINE_ID_E3M) {
                boot_e3m_setup_arch();
            } else if (mach_id == MACHINE_ID_E3M_IOHUB_LMS) {
                boot_e3m_iohub_lms_setup_arch();
            } else {
                boot_e3m_iohub_setup_arch();
            }
        } else {
            mach_id |= boot_get_e2k_machine_id();
            boot_machine_id = mach_id;
            if (mach_id == MACHINE_ID_E3S_LMS) {
                boot_e3s_lms_setup_arch();
            } else if (mach_id == MACHINE_ID_E3S) {
                boot_e3s_setup_arch();
            } else if (mach_id == MACHINE_ID_ES2_DSP_LMS ||
                       mach_id == MACHINE_ID_ES2_RU_LMS) {
                boot_es2_lms_setup_arch();
            } else if (mach_id == MACHINE_ID_ES2_DSP ||
                       mach_id == MACHINE_ID_ES2_RU) {
                boot_es2_setup_arch();
            } else if (mach_id == MACHINE_ID_E2S_LMS) {
                boot_e2s_lms_setup_arch();
            } else if (mach_id == MACHINE_ID_E2S) {
                boot_e2s_setup_arch();
            } else if (mach_id == MACHINE_ID_E8C_LMS) {
                boot_e8c_lms_setup_arch();
            } else if (mach_id == MACHINE_ID_E8C) {
                boot_e8c_setup_arch();
            } else if (mach_id == MACHINE_ID_E1CP_LMS) {
                boot_e1cp_lms_setup_arch();
            } else if (mach_id == MACHINE_ID_E1CP) {
                boot_e1cp_setup_arch();
            } else if (mach_id == MACHINE_ID_E8C2_LMS) {
                boot_e8c2_lms_setup_arch();
            } else if (mach_id == MACHINE_ID_E8C2) {
                boot_e8c2_setup_arch();
            } else {
                mach_id |= MACHINE_ID_E3M;
                boot_machine_id = mach_id;
                if (mach_id == MACHINE_ID_E3M_LMS) {
                    boot_e3m_lms_setup_arch();
                } else if (mach_id == MACHINE_ID_E3M) {
                    boot_e3m_setup_arch();
                } else if (mach_id ==
                           MACHINE_ID_E3M_IOHUB_LMS) {
                    boot_e3m_iohub_lms_setup_arch();
                } else {
                    boot_e3m_iohub_setup_arch();
                }
            }
        }

        if (IS_CPU_TYPE_VIRT(cpu_type)) {
            virt_mach_id = GET_VIRT_MACHINE_ID(cpu_type);
        } else {
            virt_mach_id = mach_id;
        }
        boot_machine_id = mach_id;
        boot_virt_machine_id = virt_mach_id;
#endif /* CONFIG_E2K_MACHINE */
        boot_machine.id = boot_machine_id;
        boot_machine.virt_id = boot_virt_machine_id;

        /* Initialize this as early as possible (but after
         * setting cpu id and revision) */
        boot_setup_cpu_features(&boot_machine);
    }

    /*
     * An early parse of cmd line.
     */
#ifdef	HAVE_SMP
    if (IS_BOOT_STRAP_CPU()) {
#endif	/* HAVE_SMP */
        boot_parse_param(bootblock);
#ifdef	HAVE_SMP
    }
#endif	/* HAVE_SMP */

#if defined(CONFIG_SERIAL_BOOT_PRINTK)
    if (!recovery || cnt_points)
        boot_setup_serial_console(&bootblock->info);
#endif

#if defined(DEBUG_BOOT_INFO) && DEBUG_BOOT_INFO
    if (IS_BOOT_STRAP_CPU()) {
        /*
         * Set boot strap CPU id to enable erly boot print with
         * nodes and CPUs numbers
         */
        int cpu_id = READ_APIC_ID();
        boot_smp_set_processor_id(cpu_id);
        do_boot_printk("bootblock 0x%x, flags 0x%x\n",
                       bootblock, bootblock->boot_flags);
        print_bootblock(bootblock);
    }
#endif

    /*
     * BIOS/x86 loader has following incompatibilities with kernel
     * boot process assumption:
     *	1. Not set USBR register to C stack high address
     *	2. Set PSP register size to full procedure stack memory
     *	   when this size should be without last page (last page
     *	   used as guard to preserve stack overflow)
     *	3. Set PCSP register size to full procedure chain stack memory
     *	   when this size should be without last page (last page
     *	   used as guard to preserve stack overflow)
     */
    boot_info = &bootblock->info;
    signature = boot_info->signature;

    if (signature == X86BOOT_SIGNATURE) {
        usbr_struct_t	USBR = {{0}};
        usd_struct_t	USD;
        psp_struct_t	PSP;
        pcsp_struct_t	PCSP;

            read_USD_reg(&USD);
            USBR.USBR_base = PAGE_ALIGN_DOWN(USD.USD_base);
            write_USBR_reg(USBR);

            PSP = RAW_READ_PSP_REG();
            PSP.PSP_size -= PAGE_SIZE;
            RAW_WRITE_PSP_REG(PSP.PSP_hi_struct, PSP.PSP_lo_struct);

            PCSP = RAW_READ_PCSP_REG();
            PCSP.PCSP_size -= PAGE_SIZE;
            RAW_WRITE_PCSP_REG(PCSP.PCSP_hi_struct,
                               PCSP.PCSP_lo_struct);
    }

    INIT_G_REGS();

    /*
     * Set UPSR register in the initial state (where interrupts
     * are disabled).
     * Switch control from PSR register to UPSR if it needs
     */
    SET_KERNEL_UPSR(1);

    boot_init(bootblock);

}















