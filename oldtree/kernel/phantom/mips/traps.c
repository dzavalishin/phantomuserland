
#include <kernel/trap.h>
#include <phantom_assert.h>
#include <stdio.h>
#include <signal.h>
#include <threads.h>



static char *trap_type[ARCH_N_TRAPS] =
{
    "HW interrupt",
    "TLB modification",
    "TLB load",
    "TLB store",
    "Address error on data load or inst fetch",
    "Address error on data store",
    "Bus error accessing instruction",
    "Bus error accessing data",
    "Syscall",
    "Breakpoint",
    "Reserved opcode",
    "No coprocessor",
    "Arithmetic overflow",
    "Trap",
    "?14",
    "Floating point exception",
    "?16",
    "?17",
    "?18",
    "?19",
    "?20",
    "?21",
    "?22",

    "Watcpoint",
    "Machine check",

    "?25",
    "?26",
    "?27",
    "?28",
    "?29",
    "?30",
    "?31",

    "Reset",
    "Cache error",
    "TLB miss",
    "xTLB miss",
    "?",
};

#define TRAP_TYPES (sizeof(trap_type)/sizeof(trap_type[0]))

const char *trap_name(unsigned int trapnum)
{
    return trapnum < TRAP_TYPES ? trap_type[trapnum] : "(unknown)";
}


extern int panic_reenter;

void dump_ss(struct trap_state *st)
{
    if(panic_reenter > 1)
        return;

    //int from_user = (st->cs & 3) || (st->eflags & EFL_VM);

    printf("Dump of mips state:\n" );

    printf("Z = (0)      AT= %08x V0= %08x V1= %08x\n", st->r1, st->r2, st->r3);
    printf("A0= %08x A1= %08x A2= %08x A3= %08x\n", st->r4, st->r5, st->r6, st->r7);
    printf("T0= %08x T1= %08x T2= %08x T3= %08x\n", st->r8, st->r9, st->r10, st->r11);
    printf("T4= %08x T5= %08x T6= %08x T7= %08x\n", st->r12, st->r13, st->r14, st->r15);
    // TODO rest 16
    printf("S0= %08x S1= %08x S2= %08x S3= %08x\n", st->r16, st->r17, st->r18, st->r19);
    printf("S4= %08x S5= %08x S6= %08x S7= %08x\n", st->r20, st->r21, st->r22, st->r23);
    printf("T8= %08x T9= %08x K0= %08x K1= %08x\n", st->r24, st->r25, st->r26, st->r27);
    printf("GP= %08x SP= %08x FP= %08x RA= %08x\n", st->r28, st->r29, st->r30, st->r31);

    printf("HI= %08x LO= %08x PC= %08x\n",           st->hi, st->lo, st->pc );

    printf("trapno %d: %s, intno %08x\n",
           st->trapno, trap_name(st->trapno),
           st->intno);

    //if(st->trapno == T_PAGE_FAULT) printf("page fault linear address %08x\n", st->cr2);

    if(phantom_symtab_getname)
    {
        printf("PC %6p: %s\n", st->pc, phantom_symtab_getname((void *)st->pc) );
    }

    stack_dump_from((void *)st->r11);
}



int trap2signo( struct trap_state *ts )
{
    int		sig_no = 0;

    switch (ts->trapno)
    {

    case T_INTERRUPT:
        sig_no = SIGINT;  break;	// hardware interrupt

    case T_ADDR_LOAD:
    case T_ADDR_SAVE:
    case T_CODE_BUS_ERR:
    case T_DATA_BUS_ERR:
        sig_no = SIGBUS; break;

    case T_SYSCALL:
        sig_no = SIGSYS; break;

    case T_BREAK:
    case T_TRAP:
        sig_no = SIGTRAP; break;

    case T_NO_INSTR:
    case T_NO_CP:
        sig_no = SIGILL; break;

    case T_OVERFLOW:
    case T_FPE:
        sig_no = SIGFPE; break;

    case T_CACHE_ERR:
        sig_no = SIGBUS; break;

    case T_TLB:
    case T_XTLB:
        sig_no = SIGSEGV; break;

    default:
        panic("No signal mapping for trap number: %d", ts->trapno);
    }

    return sig_no;
}


int mips_irq_dispatch(struct trap_state *ts, u_int32_t pending);


// Called from asm wrap
void hal_MIPS_exception_dispatcher(struct trap_state *ts, int cause)
{
    (void) cause;
    (void) ts;

    int trapno = (cause >> 2) & 0x1F; // 5 bits
    ts->trapno = trapno;
    ts->intno = 0;

    switch( trapno )
    {
    case T_INTERRUPT:
        {
            int ipending = (cause >> 8) & 0xFF; // mask of pending interrupts

            mips_irq_dispatch(ts, ipending);

/*
            // Use softirq 0 for direct thread switch
            if( ipending & 0x01 )
            {
                phantom_scheduler_soft_interrupt();
                return;
            }
#warning here we must call boards specific interrup dispatch code
            panic("interrupt %x", ipending);
            */
            break;
        }

    case T_SYSCALL:
        syscall_sw(ts);
        return;

    case T_TLB_LOAD:
    case T_TLB_STORE:
        tlb_refill_exception(ts);
        return;

#if 0
    case T_TLB_MOD:
    case T_ADDR_LOAD:
    case T_ADDR_SAVE:
    case T_CODE_BUS_ERR:
    case T_DATA_BUS_ERR:
    case T_BREAK:
    case T_NO_INSTR:
    case T_NO_CP:
    case T_OVERFLOW:
    case T_TRAP:
    case T_FPE:
#endif

    default:
        phantom_kernel_trap( ts );
        return;
    }

}

/*
// Called from asm wrap
void hal_MIPS_interrupt_dispatcher(struct trap_state *ts, int cause)
{
    //(void) cause;
    //(void) ts;
    //panic("interrupt");
    hal_MIPS_exception_dispatcher(ts, cause); // the same
}
*/





















