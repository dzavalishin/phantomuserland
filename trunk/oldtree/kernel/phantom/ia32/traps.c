/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * ia32 traps support.
 *
**/

#include <i386/proc_reg.h>
#include <i386/eflags.h>

#include <kernel/trap.h>

#include <phantom_assert.h>
#include <phantom_libc.h>

#include <signal.h>
#include <ia32/debug_reg.h>

static char *trap_type[] = {
    "Divide error", "Debug trap",		"NMI",		"Breakpoint",
    "Overflow",	    "Bounds check",		"Invalid opcode","No coprocessor",
    "Double fault", "Coprocessor overrun",	"Invalid TSS",	"Segment not present",
    "Stack bounds", "General protection",	"Page fault",	"(reserved)",
    "Coprocessor error"
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

    int from_user = (st->cs & 3) || (st->eflags & EFL_VM);

    printf("Dump of i386 state:\n", st);
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
        printf("EIP %6p: %s\n", st->eip, phantom_symtab_getname((void *)st->eip) ); // TODO 64 bit machdep
    }


    //if(!from_user)
        stack_dump_from((void *)st->ebp);
}







static int trap_ignore(struct trap_state *ts)
{
    (void)ts;
    return 0;
}

/*
//static
int trap_panic(struct trap_state *ts)
{
    dump_ss(ts);
    int type = ts->trapno;

    panic("Unexpected trap %s\n", trap_name(type));


    return -1;
}
*/

/**
 *
 * If trap handler returns nonzero, panic will be called.
 *
**/

int (*phantom_trap_handlers[ARCH_N_TRAPS])(struct trap_state *ts) = {
	trap_panic, /* 0 */
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic, /* 7 */
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_ignore,  // 15 spurios traps, not really used
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic, /* 23 */
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic,
	trap_panic  /* 31 */
};




int trap2signo( struct trap_state *ts )
{
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
}
