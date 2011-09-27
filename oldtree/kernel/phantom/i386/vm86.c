/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * VM86 support
 *
**/

#define DEBUG_MSG_PREFIX "vm86"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/init.h>
#include <kernel/trap.h>
#include <kernel/page.h>

#include <setjmp.h>

#include <phantom_types.h>
#include <phantom_libc.h>

#include <hal.h>
#include <threads.h>

#include <ia32/vm86.h>

#include "../misc.h"
#include <ia32/private.h>

// Against warnings
#if 1

#define inb_W(p) __inb(p)
#define inw_W(p) __inw(p)
#define inl_W(p) __inl(p)

#define outb_W(p,v) __outb(p,v)
#define outw_W(p,v) __outw(p,v)
#define outl_W(p,v) __outl(p,v)

#else

#define inb_W(p) inb(p)
#define inw_W(p) inw(p)
#define inl_W(p) inl(p)

#define outb_W(p,v) outb(p,v)
#define outw_W(p,v) outw(p,v)
#define outl_W(p,v) outl(p,v)

#endif


static int vm86_ready = 0;


struct vm86_settings  vm86_setup;

void phantom_ret_from_vm86();

//#warning base_trap_handlers
//extern int (*base_trap_handlers[])(struct trap_state *ts);


static void check_ua( u_int32_t addr )
{
    // BIOS data area is accessible
    if( addr < 0x400u )        return;
    // ROM area is ok too
    if( addr > 0xA0000u && addr < 0x100000u )        return;

    if( addr < (unsigned int)vm86_setup.code || addr > (unsigned int)vm86_setup.stackHi )
        panic("Invalid VM86 address 0x%X (%X-%X)",
              addr, vm86_setup.code, vm86_setup.stackHi
             );
}





static void suword32( u_int32_t addr, u_int32_t data )
{
    check_ua( addr );
    *((u_int32_t*)addr) = data;
}

static void suword16( u_int32_t addr, u_int16_t data )
{
    check_ua( addr );
    *((u_int16_t*)addr) = data;
}

static u_int32_t fuword32( u_int32_t addr )
{
    check_ua( addr );
    return *((u_int32_t*)addr);
}

static u_int16_t fuword16( u_int32_t addr )
{
    check_ua( addr );
    return *((u_int16_t*)addr);
}

static u_int8_t fubyte( u_int32_t addr )
{
    check_ua( addr );
    return *((u_int8_t*)addr);
}








#define USE_VME 0



#define	HLT	0xf4
#define	CLI	0xfa
#define	STI	0xfb
#define	PUSHF	0x9c
#define	POPF	0x9d
#define	INTn	0xcd
#define	IRET	0xcf
#define	CALLm	0xff
#define OPERAND_SIZE_PREFIX	0x66
#define ADDRESS_SIZE_PREFIX	0x67
#define PUSH_MASK	~(EFL_VM | EFL_RF | EFL_IF)
#define POP_MASK	~(EFL_VIP | EFL_VIF | EFL_VM | EFL_RF | EFL_IOPL_USER)

static inline u_int32_t
MAKE_ADDR(u_short sel, u_short off)
{
    return ((u_int32_t)((sel << 4) + off));
}

static inline void
GET_VEC(u_int vec, u_int16_t *sel, u_int16_t *off)
{
    *sel = vec >> 16;
    *off = vec & 0xffffu;
}

static inline u_int
MAKE_VEC(u_int16_t sel, u_int16_t off)
{
    return ((sel << 16) | off);
}

static inline void
PUSH(u_short x, struct trap_state *ts)
{
    ts->esp -= 2;
    suword16(MAKE_ADDR(ts->ss, ts->esp), x);
}

static inline void
PUSHL(u_int32_t x, struct trap_state *ts)
{
    ts->esp -= 4;
    suword32(MAKE_ADDR(ts->ss, ts->esp), x);
}

static inline u_int16_t
POP(struct trap_state *ts)
{
    u_int16_t x = fuword16(MAKE_ADDR(ts->ss, ts->esp));

    ts->esp += 2;
    return (x);
}

static inline u_int32_t
POPL(struct trap_state *ts)
{
    u_int32_t x = fuword32(MAKE_ADDR(ts->ss, ts->esp));

    ts->esp += 4;
    return (x);
}

int
vm86_emulate(struct trap_state *ts)
{
    u_int32_t addr;
    u_char i_byte;
    u_int temp_flags;
    int inc_ip = 1;
    int retcode = 0;

    // has trace - just kill him
    if (ts->eflags & EFL_TF)
    {
        printf("VM86 killed by trace flag");
        return -1;
    }

    addr = MAKE_ADDR(ts->cs, ts->eip);
    i_byte = fubyte(addr);
    if (i_byte == ADDRESS_SIZE_PREFIX) {
        i_byte = fubyte(++addr);
        inc_ip++;
    }

#if USE_VME
    if (vm86->vm86_has_vme) {
        switch (i_byte) {
        case OPERAND_SIZE_PREFIX:
            i_byte = fubyte(++addr);
            inc_ip++;
            switch (i_byte) {
            case PUSHF:
                if (ts->eflags & EFL_VIF)
                    PUSHL((ts->eflags & PUSH_MASK)
                          | EFL_IOPL_USER | EFL_IF, vmf);
                else
                    PUSHL((ts->eflags & PUSH_MASK)
                          | EFL_IOPL_USER, vmf);
                ts->eip += inc_ip;
                return 0;

            case POPF:
                temp_flags = POPL(ts) & POP_MASK;
                ts->eflags = (ts->eflags & ~POP_MASK)
                    | temp_flags | EFL_VM | EFL_IF;
                ts->eip += inc_ip;
                if (temp_flags & EFL_IF) {
                    ts->eflags |= EFL_VIF;
                    if (ts->eflags & EFL_VIP)
                        break;
                } else {
                    ts->eflags &= ~EFL_VIF;
                }
                return 0;
            }
            break;

            /* VME faults here if VIP is set, but does not set VIF. */
        case STI:
            ts->eflags |= EFL_VIF;
            ts->eip += inc_ip;
            if ((ts->eflags & EFL_VIP) == 0) {
                uprintf("fatal sti\n");
                return -1;
            }
            break;

            /* VME if no redirection support */
        case INTn:
            break;

            /* VME if trying to set EFL_TF, or EFL_IF when VIP is set */
        case POPF:
            temp_flags = POP(ts) & POP_MASK;
            ts->eflags = (ts->eflags & ~POP_MASK)
                | temp_flags | EFL_VM | EFL_IF;
            ts->eip += inc_ip;
            if (temp_flags & EFL_IF) {
                ts->eflags |= EFL_VIF;
                if (ts->eflags & EFL_VIP)
                    break;
            } else {
                ts->eflags &= ~EFL_VIF;
            }
            return (retcode);

            /* VME if trying to set EFL_TF, or EFL_IF when VIP is set */
        case IRET:
            ts->eip = POP(ts);
            vmf->vmf_cs = POP(ts);
            temp_flags = POP(ts) & POP_MASK;
            ts->eflags = (ts->eflags & ~POP_MASK)
                | temp_flags | EFL_VM | EFL_IF;
            if (temp_flags & EFL_IF) {
                ts->eflags |= EFL_VIF;
                if (ts->eflags & EFL_VIP)
                    break;
            } else {
                ts->eflags &= ~EFL_VIF;
            }
            return (retcode);

        }
        return (SIGBUS);
    }
#endif

    switch (i_byte) {
    case OPERAND_SIZE_PREFIX:
        i_byte = fubyte(++addr);
        inc_ip++;


        switch (i_byte)
        {
        case PUSHF:
            if (ts->eflags & EFL_VIF)
                PUSHL((ts->eflags & PUSH_MASK) | EFL_IOPL_USER | EFL_IF, ts);
            else
                PUSHL((ts->eflags & PUSH_MASK) | EFL_IOPL_USER, ts);
            ts->eip += inc_ip;
            return (retcode);

        case POPF:
            temp_flags = POPL(ts) & POP_MASK;
            ts->eflags = (ts->eflags & ~POP_MASK)
                | temp_flags | EFL_VM | EFL_IF;
            ts->eip += inc_ip;
            if (temp_flags & EFL_IF) {
                ts->eflags |= EFL_VIF;
                if (ts->eflags & EFL_VIP)
                    break;
            } else {
                ts->eflags &= ~EFL_VIF;
            }
            return (retcode);

        case 0xEF: /* outl (%dx), eax */
            outl( ts->edx & 0xFFFFu, ts->eax );
            goto retok;

        case 0xE7: /* outl port, eax */
            {
                u_int8_t port = fubyte(++addr);
                inc_ip++;
                outl_W( port, ts->eax );
                goto retok;
            }
        case 0xED: /* inl eax, (%dx) */
            {
                u_int16_t port = ts->edx & 0xFFFFu;
                ts->eax = inl(port);
                goto retok;
            }
        case 0xE5: /* inl eax, port */
            {
                u_int8_t port = fubyte(++addr);
                inc_ip++;
                ts->eax = inl_W( port );
                goto retok;
            }


        }
        printf("VM86 unknown prefixed op 0x%X\n", i_byte );

        return -1;

    case CLI:
        ts->eflags &= ~EFL_VIF;
        ts->eip += inc_ip;
        return (retcode);

    case STI:
        /* if there is a pending interrupt, go to the emulator */
        ts->eflags |= EFL_VIF;
        ts->eip += inc_ip;
        if (ts->eflags & EFL_VIP)
            break;
        return (retcode);

    case PUSHF:
        if (ts->eflags & EFL_VIF)
            PUSH((ts->eflags & PUSH_MASK) | EFL_IOPL_USER | EFL_IF, ts);
        else
            PUSH((ts->eflags & PUSH_MASK) | EFL_IOPL_USER, ts);
        ts->eip += inc_ip;
        return (retcode);

    case INTn:
        i_byte = fubyte(addr + 1);
        //if ((vm86->vm86_intmap[i_byte >> 3] & (1 << (i_byte & 7))) != 0)
        //break;

        // no way we are going to do DOS there :)
        if(i_byte == 0x21)
        {
            // TODO pass success somehow?
            //printf("VM86 finished with int21\n");
            phantom_ret_from_vm86();
        }

        if (ts->eflags & EFL_VIF)
            PUSH((ts->eflags & PUSH_MASK) | EFL_IOPL_USER | EFL_IF, ts);
        else
            PUSH((ts->eflags & PUSH_MASK) | EFL_IOPL_USER, ts);
        PUSH(ts->cs, ts);
        PUSH(ts->eip + inc_ip + 1, ts);	/* increment IP */
        {
            u_int16_t tmp_ip;
            u_int16_t tmp_cs;
            GET_VEC(fuword32(i_byte * 4), &tmp_cs, &tmp_ip);
            ts->eip = tmp_ip;
            ts->cs = tmp_cs;
        }
        ts->eflags &= ~EFL_TF;
        ts->eflags &= ~EFL_VIF;
        return (retcode);

    case IRET:
        ts->eip = POP(ts);
        ts->cs = POP(ts);
        temp_flags = POP(ts) & POP_MASK;
        ts->eflags = (ts->eflags & ~POP_MASK) | temp_flags | EFL_VM | EFL_IF;
        if (temp_flags & EFL_IF) {
            ts->eflags |= EFL_VIF;
            if (ts->eflags & EFL_VIP)
                break;
        } else {
            ts->eflags &= ~EFL_VIF;
        }
        return (retcode);

    case POPF:
        temp_flags = POP(ts) & POP_MASK;
        ts->eflags = (ts->eflags & ~POP_MASK) | temp_flags | EFL_VM | EFL_IF;
        ts->eip += inc_ip;
        if( temp_flags & EFL_IF ) {
            ts->eflags |= EFL_VIF;
            if (ts->eflags & EFL_VIP)
                break;
        } else {
            ts->eflags &= ~EFL_VIF;
        }
        return (retcode);

    case 0xEE: /* outb (%dx), al */
        outb( ts->edx & 0xFFFFu, ts->eax & 0xFFu );
        goto retok;

    case 0xEF: /* outw (%dx), ax */
        outw( ts->edx & 0xFFFFu, ts->eax & 0xFFFFu );
        goto retok;

    case 0xE6: /* outb port, al */
        {
        u_int8_t port = fubyte(++addr);
        inc_ip++;
        outb_W( port, ts->eax & 0xFFu );
        goto retok;
        }

    case 0xE7: /* outw port, ax */
        {
        u_int8_t port = fubyte(++addr);
        inc_ip++;
        outw_W( port, ts->eax & 0xFFFFu );
        goto retok;
        }

    case 0xE4: /* inb al, port */
        {
            u_int8_t port = fubyte(++addr);
            inc_ip++;
            ts->eax = (ts->eax & ~0xFFu) | (inb_W(port) & 0xFFu);
            goto retok;
        }
    case 0xE5: /* inw ax, port */
        {
            u_int8_t port = fubyte(++addr);
            inc_ip++;
            ts->eax = (ts->eax & ~0xFFFFu) | (inw_W(port) & 0xFFFFu);
            goto retok;
        }
    case 0xEC: /* inb al, (%dx) */
        {
            u_int16_t port = ts->edx & 0xFFFFu;
            ts->eax = (ts->eax & ~0xFFu) | (inb(port) & 0xFFu);
            goto retok;
        }
    case 0xED: /* inw ax, (%dx) */
        {
            u_int16_t port = ts->edx & 0xFFFFu;
            ts->eax = (ts->eax & ~0xFFFFu) | (inw(port) & 0xFFFFu);
            goto retok;
        }
    }

    printf("Unknown opcode 0x%X", i_byte );
    return -1;

retok:
    ts->eip += inc_ip;
    return 0;

}




static void *saved_trap_handler = 0;


int vm86_gpf_handler(struct trap_state *ts)
{
    if( vm86_emulate(ts) )
    {
        phantom_ret_from_vm86();
        /*NOTREACHED*/
    }

    return 0;
}

static void *		exe;

void phantom_v86_run(void *code, size_t size);

// asm code
//extern char vesa_setup_end[], vesa_setup_start[];
extern char bios_int_10_end[], bios_int_10[];

static hal_mutex_t      realmode_mutex;

//#warning i am non-reentrant, kill me
void phantom_bios_int_10()
{
    hal_mutex_lock(&realmode_mutex);
    tss_vm86.tss.ebp = 0; // Trying to prevent stackdump from going nuts
    phantom_v86_run( bios_int_10, bios_int_10_end-bios_int_10);
    hal_mutex_unlock(&realmode_mutex);
}

void phantom_bios_int_10_args(RM_REGS *regs)
{
    hal_mutex_lock(&realmode_mutex);

    tss_vm86.tss.eax = regs->d.eax;
    tss_vm86.tss.ebx = regs->d.ebx;
    tss_vm86.tss.ecx = regs->d.ecx;
    tss_vm86.tss.edx = regs->d.edx;

    tss_vm86.tss.es = ((int)vm86_setup.data) >> 4;

    tss_vm86.tss.esi = regs->d.esi;
    tss_vm86.tss.edi = regs->d.edi;

    tss_vm86.tss.ebp = regs->d.ebp;

    phantom_v86_run( bios_int_10, bios_int_10_end-bios_int_10);

    regs->d.eax = tss_vm86.tss.eax;
    regs->d.ebx = tss_vm86.tss.ebx;
    regs->d.ecx = tss_vm86.tss.ecx;
    regs->d.edx = tss_vm86.tss.edx;

    regs->d.esi = tss_vm86.tss.esi;
    regs->d.edi = tss_vm86.tss.edi;

    regs->d.ebp = tss_vm86.tss.ebp;

    hal_mutex_unlock(&realmode_mutex);
}


// it does not work if we try to do VM86 calls late in OS init sequence
// - after base_paging_init()/base_paging_load()/start_osenv();

#define TSS_VM86_RETURN 0


// Must be called late in init sequence
void phantom_init_vm86(void)
{
    hal_mutex_init(&realmode_mutex,"VM86");

#if TSS_VM86_RETURN
    phantom_load_gdt(); // do we really need it?
    phantom_load_main_tss(); // need?
#endif

    void *	r0stack;

    assert(PAGE_ALIGNED(VM86_R0_STACK_SIZE));
    assert(PAGE_ALIGNED(VM86_EXE_SIZE));

    hal_alloc_phys_pages_low( (physaddr_t *) (&r0stack), VM86_R0_STACK_SIZE/PAGE_SIZE);
    //r0stack = calloc( VM86_R0_STACK_SIZE, 1 );
    hal_alloc_phys_pages_low( (physaddr_t *) (&exe), VM86_EXE_SIZE/PAGE_SIZE);


    if(r0stack == 0 || exe == 0 )        panic("can't allocate VM86 resources");

    if( ((int)exe) & 0xF )
        panic("VM86 memory unaligned");

    SHOW_INFO( 7, "VM86 region address is 0x%X, R0 stack is 0x%X", exe, r0stack );

    {
    // TODO BUG -sizeof(int) needed?
    void *r0_s_end = r0stack+VM86_R0_STACK_SIZE-sizeof(int);

    tss_vm86.tss.ss0 = KERNEL_DS;
    tss_vm86.tss.esp0 = (int)r0_s_end;
    }

    tss_vm86.tss.eflags = EFL_VM|EFL_IF;
    tss_vm86.tss.eflags |= EFL_IOPL_USER; // Let Vm86 to do CLI/INTn/IRET

    {
        unsigned int cr3val;
        asm("mov %%cr3, %0" : "=r" (cr3val));
        tss_vm86.tss.cr3 = cr3val;
    }

    vm86_setup.code  	= exe;
    vm86_setup.data  	= exe+1*64*1024;
    //vm86_setup.stackLow = exe+2*64*1024;
    //vm86_setup.stackHi 	= exe+3*64*1024-4;

    vm86_setup.stackLow = exe+1*64*1024;
    vm86_setup.stackHi 	= exe+2*64*1024-4;

    if( ((int)vm86_setup.stackHi) >= 0x100000 )
    {
        printf("Skipping VM86 due to wrong DOS memory allocation");
        return;
    }

    //tss_vm86.tss.eip = 0; // start of CS
    //tss_vm86.tss.esp = 0xFFF0; // 16 bytes below. why?

    tss_vm86.tss.cs = ((int)vm86_setup.code) >> 4;
    tss_vm86.tss.ds = ((int)vm86_setup.data) >> 4;
    tss_vm86.tss.ss = ((int)vm86_setup.data) >> 4;
    tss_vm86.tss.es = tss_vm86.tss.ds;
    tss_vm86.tss.fs = 0; // to access low mem easily
    tss_vm86.tss.gs = 0; // to access low mem easily

    tss_vm86.tss.ldt = MAIN_LDT;

    // do all soft interrupts in VM86 directly
    memset( tss_vm86.redir, 0, sizeof(tss_vm86.redir) );

    vm86_ready = 1;
}


static jmp_buf jb;
void phantom_v86_run(void *code, size_t size)
{
    if(!vm86_ready) 		panic("VM86 is not ready, but used");
    if( size > 0xFFFFu )        panic("VM86 code size > 64k");

    tss_vm86.tss.eip = 0; // start of CS
    tss_vm86.tss.esp = 0xFFF0; // 16 bytes below. why?

    tss_vm86.tss.cs = ((int)vm86_setup.code) >> 4;
    tss_vm86.tss.ds = ((int)vm86_setup.data) >> 4;
    //tss_vm86.tss.ss = ((int)vm86_setup.data) >> 4;
    tss_vm86.tss.ss = ((int)vm86_setup.stackLow) >> 4;

    tss_vm86.tss.eflags = EFL_VM|EFL_IF;
    tss_vm86.tss.eflags |= EFL_IOPL_USER; // Let Vm86 to do CLI/INTn/IRET


    memset( exe, 0, VM86_EXE_SIZE );

    memmove( vm86_setup.code, code, size );

    //hal_cli();
    hal_disable_preemption();

    saved_trap_handler = phantom_trap_handlers[T_GENERAL_PROTECTION];
    phantom_trap_handlers[T_GENERAL_PROTECTION] = vm86_gpf_handler;

    //hexdump(vm86_setup.data, 256, "VESA data", 0);

    //printf("Lets go VM86\n");
    //getchar();

#if USE_VME
    set_cr4( get_cr4() | CR4_VME );
#endif


    if(setjmp(jb))
    {
        // Returned via longjmp - reset main TSS
        phantom_load_gdt(); // need?
        phantom_load_main_tss(); // need? why?
        phantom_trap_handlers[T_GENERAL_PROTECTION] = saved_trap_handler;

        // let us use VM86 TSS again - in phantom_load_gdt
        //gdt[VM86_TSS / 8].access &= ~ACC_TSS_BUSY;

        hal_enable_preemption();
        hal_sti();
        return;
    }

    hal_cli();
    asm("ljmp %0, $0" : : "i" (VM86_TSS));
    phantom_trap_handlers[T_GENERAL_PROTECTION] = saved_trap_handler;

    hal_enable_preemption();
    hal_sti();

}

void phantom_ret_from_vm86()
{
    if(!vm86_ready) panic("VM86 is not ready, but used");
#if TSS_VM86_RETURN
    asm("ljmp %0, $0" : : "i" (MAIN_TSS));
#else
    longjmp(jb,1);
#endif
}
