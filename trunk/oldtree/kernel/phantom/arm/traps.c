
#include <kernel/trap.h>
#include <phantom_assert.h>
#include <stdio.h>
#include <signal.h>
#include <threads.h>

#include <arm/private.h>

/*

STMFD, STMDB, LDMEA, LDMDB:
Starts from highest reg number. Highest reg number in highest mem addr.

LDMIA, LDMFD, STMIA, STMEA:
Startts from lowest reg number. Highest reg number in highest mem addr.

*/




static char *trap_type[8] =
{
    "reset",
    "inv opcode",
    "soft int",
    "prefetch abort",
    "data abort",
    "?",
    "IRQ",
    "FIQ",
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

    printf("Dump of arm state:\n" );

    printf("R 0=%08x R 1=%08x R 2=%08x R 3=%08x\n", st->r0, st->r1, st->r2, st->r3);
    printf("R 4=%08x R 5=%08x R 6=%08x R 7=%08x\n", st->r4, st->r5, st->r6, st->r7);
    printf("R 8=%08x R 9=%08x R10=%08x !R12=%08x\n", st->r8, st->r9, st->r10, st->r12);

    printf("FP(R11)=%08x SP(R13)=%08x LR(R14)=%08x PC(R15)=%08x\n",
           st->r11, st->usr_sp, st->usr_lr, st->pc
          );

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

    /*
     * It probably makes no sense to pass all these signals to the
     * application program.
     */
    switch (ts->trapno)
    {

    case -1: 	sig_no = SIGINT;  break;	// hardware interrupt 
    case 0: 	sig_no = SIGSEGV; break;	// Reset? Impossible.
    case 1: 	sig_no = SIGILL;  break;	// Invalid opcode
    case 2: 	sig_no = SIGEMT;  break;	// soft int
    case 3: 	sig_no = SIGSEGV; break;	// prefetch abort
    case 4: 	sig_no = SIGSEGV; break;	// data abort
    case 5: 	sig_no = SIGBUS;  break;	// ?
    case 6: 	sig_no = SIGBUS;  break;	// IRQ
    case 7: 	sig_no = SIGBUS;  break;	// FIQ

    default:
        panic("No signal mapping for trap number: %d", ts->trapno);
    }


    return sig_no;
}


int handle_swi(struct trap_state *ts)
{
    //printf("r0=%d, r1=%d, r2=%d, R3=%d, r12=%d\n", ts->r0, ts->r1, ts->r2, ts->r3, ts->r12 );

    int swino = ts->intno & 0xFFFFFF;

    // If no real angel exist, return -1 to caller
    if( swino == 0x123456 )
    {
        ts->r0 = -1;
        ts->r1 = -1;
        return 0;
    }


    // TODO magic number! define! used to request scheduler softint
    if( swino == 0xFFF )
    {
        phantom_scheduler_soft_interrupt();
        //phantom_scheduler_schedule_soft_irq();
    }
    return 0;
}

void arm_init_swi(void)
{
    phantom_trap_handlers[T_SOFT_INT] = handle_swi;
    //asm volatile("mov r0, #10; mov r1, #11; mov r2, #12; mov r3, #13; mov r12, #22; swi 0x34");
}
