/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 * HISTORY
 * $Log:	proc_reg.h,v $
 * Revision 2.4  92/01/03  20:08:42  dbg
 * 	Add macros to get and set various privileged registers.
 * 	[91/10/20            dbg]
 * 
 * Revision 2.3  91/05/14  16:15:32  mrt
 * 	Correcting copyright
 * 
 * Revision 2.2  91/05/08  12:41:37  dbg
 * 	Created.
 * 	[91/03/21            dbg]
 * 
 */

/*
 * Processor registers for i386 and i486.
 */
#ifndef	_I386_PROC_REG_H_
#define	_I386_PROC_REG_H_

#ifndef ARCH_ia32
#warning Intel32 code! Wrong arch?
#endif


#ifndef ASSEMBLER

#include <phantom_types.h>


/*
 * region descriptors, used to load gdt/idt tables before segments yet exist.
 */
struct region_descriptor {
	//unsigned rd_limit:16;		/* segment extent */
	u_int16_t 	rd_limit;		/* segment extent */
	unsigned 	rd_base:32 __attribute__ ((packed));	/* base address  */
};

#endif // ASSEMBLER



/*
 * CR0
 */
#define	CR0_PG	0x80000000		/*	 enable paging */
#define	CR0_CD	0x40000000		/* i486: cache disable */
#define	CR0_NW	0x20000000		/* i486: no write-through */
#define	CR0_AM	0x00040000		/* i486: alignment check mask */
#define	CR0_WP	0x00010000		/* i486: write-protect kernel access */
#define	CR0_NE	0x00000020		/* i486: handle numeric exceptions */
#define	CR0_ET	0x00000010		/*	 extension type is 80387 */
					/*	 (not official) */
#define	CR0_TS	0x00000008		/*	 task switch */
#define	CR0_EM	0x00000004		/*	 emulate coprocessor */
#define	CR0_MP	0x00000002		/*	 monitor coprocessor */
#define	CR0_PE	0x00000001		/*	 enable protected mode */


/*
 * Bits in PPro special registers
 */
#define	CR4_VME	0x00000001	/* Virtual 8086 mode extensions */
#define	CR4_PVI	0x00000002	/* Protected-mode virtual interrupts */
#define	CR4_TSD	0x00000004	/* Time stamp disable */
#define	CR4_DE	0x00000008	/* Debugging extensions */
#define	CR4_PSE	0x00000010	/* Page size extensions */
#define	CR4_PAE	0x00000020	/* Physical address extension */
#define	CR4_MCE	0x00000040	/* Machine check enable */
#define	CR4_PGE	0x00000080	/* Page global enable */
#define	CR4_PCE	0x00000100	/* Performance monitoring counter enable */
#define	CR4_FXSR 0x00000200	/* Fast FPU save/restore used by OS */
#define	CR4_XMM	0x00000400	/* enable SIMD/MMX2 to use except 16 */

/*
 * Bits in AMD64 special registers.  EFER is 64 bits wide.
 */
#define	EFER_NXE 0x000000800	/* PTE No-Execute bit enable (R/W) */


#ifndef	ASSEMBLER
#ifdef	__GNUC__

#define	get_cr0() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr0, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	set_cr0(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr0" : : "r" (_temp__)); \
     })







static __inline unsigned int
read_eflags(void)
{
	unsigned int	ef;

	__asm __volatile("pushfl; popl %0" : "=r" (ef));
	return (ef);
}


static __inline void
write_eflags(unsigned int ef)
{
	__asm __volatile("pushl %0; popfl" : : "r" (ef));
}



static __inline void
disable_intr(void)
{
	__asm __volatile("cli" : : : "memory");
}



static __inline unsigned int
get_esp(void)
{
	unsigned int sel;
	__asm __volatile("movl %%esp,%0" : "=rm" (sel));
	return (sel);
}
/*
static __inline unsigned int
get_ebp(void)
{
	unsigned int sel;
	__asm __volatile("movl %%ebp,%0" : "=rm" (sel));
	return (sel);
}
*/


static __inline unsigned int
get_ss(void)
{
	unsigned int sel;
	__asm __volatile("movw %%ss,%w0" : "=rm" (sel));
	return (sel);
}



static __inline u_int64_t
rdmsr(unsigned int msr)
{
	u_int64_t rv;

	__asm __volatile("rdmsr" : "=A" (rv) : "c" (msr));
	return (rv);
}

static __inline void
wrmsr(unsigned int msr, u_int64_t newval)
{
	__asm __volatile("wrmsr" : : "A" (newval), "c" (msr));
}



static __inline u_int16_t
rdldt(void)
{
	u_int16_t ldtr;
	__asm __volatile("sldt %0" : "=g" (ldtr));
	return (ldtr);
}

static __inline void
wrldt(u_int16_t sel)
{
	__asm __volatile("lldt %0" : : "r" (sel));
}

static __inline void
halt(void)
{
	__asm __volatile("hlt");
}


static __inline void
ia32_pause(void)
{
	__asm __volatile("pause");
}

/*
 * Global TLB flush (except for thise for pages marked PG_G)
 */
static __inline void
invltlb(void)
{

	//load_cr3(rcr3());
    //set_cr3(get_cr3));

    register unsigned int _temp__;
    asm("mov %%cr3, %0" : "=r" (_temp__));
    asm volatile("mov %0, %%cr3" : : "r" (_temp__));
}

/*
 * TLB flush for an individual page (even if it has PG_G).
 * Only works on 486+ CPUs (i386 does not have PG_G).
 */
static __inline void
invlpg(unsigned int addr)
{

	__asm __volatile("invlpg %0" : : "m" (*(char *)addr) : "memory");
}










static __inline unsigned int
rdr6(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr6,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr6(unsigned int dr6)
{
	__asm __volatile("movl %0,%%dr6" : : "r" (dr6));
}


#define	get_cr4() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr4, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	set_cr4(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr4" : : "r" (_temp__)); \
     })


static __inline u_int64_t
rdtsc(void)
{
	u_int64_t rv;

	__asm __volatile("rdtsc" : "=A" (rv));
	return (rv);
}






#if 0

#define	get_ldt() \
    ({ \
	unsigned short _seg__; \
	asm volatile("sldt %0" : "=rm" (_seg__) ); \
	_seg__; \
    })

#define	set_ldt(seg) \
	asm volatile("lldt %0" : : "rm" ((unsigned short)(seg)) )


#define	get_cr2() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr2, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	get_cr3() \
    ({ \
	register unsigned int _temp__; \
	asm("mov %%cr3, %0" : "=r" (_temp__)); \
	_temp__; \
    })

#define	set_cr3(value) \
    ({ \
	register unsigned int _temp__ = (value); \
	asm volatile("mov %0, %%cr3" : : "r" (_temp__)); \
     })


#define	set_ts() \
	set_cr0(get_cr0() | CR0_TS)

#define	clear_ts() \
	asm volatile("clts")

#define	get_tr() \
    ({ \
	unsigned short _seg__; \
	asm volatile("str %0" : "=rm" (_seg__) ); \
	_seg__; \
    })

#define	set_tr(seg) \
	asm volatile("ltr %0" : : "rm" ((unsigned short)(seg)) )





static __inline void
breakpoint(void)
{
	__asm __volatile("int $3");
}

static __inline unsigned int
bsfl(unsigned int mask)
{
	unsigned int	result;

	__asm __volatile("bsfl %1,%0" : "=r" (result) : "rm" (mask));
	return (result);
}

static __inline unsigned int
bsrl(unsigned int mask)
{
	unsigned int	result;

	__asm __volatile("bsrl %1,%0" : "=r" (result) : "rm" (mask));
	return (result);
}


static __inline void
do_cpuid(unsigned int ax, unsigned int *p)
{
	__asm __volatile("cpuid"
			 : "=a" (p[0]), "=b" (p[1]), "=c" (p[2]), "=d" (p[3])
			 :  "0" (ax));
}

static __inline void
cpuid_count(unsigned int ax, unsigned int cx, unsigned int *p)
{
	__asm __volatile("cpuid"
			 : "=a" (p[0]), "=b" (p[1]), "=c" (p[2]), "=d" (p[3])
			 :  "0" (ax), "c" (cx));
}

static __inline void
enable_intr(void)
{
	__asm __volatile("sti");
}




static __inline u_int64_t
rdpmc(unsigned int pmc)
{
	u_int64_t rv;

	__asm __volatile("rdpmc" : "=A" (rv) : "c" (pmc));
	return (rv);
}


static __inline void
wbinvd(void)
{
	__asm __volatile("wbinvd");
}




static __inline unsigned int
rfs(void)
{
	unsigned int sel;
	__asm __volatile("movl %%fs,%0" : "=rm" (sel));
	return (sel);
}



static __inline u_int64_t
rgdt(void)
{
	u_int64_t gdtr;
	__asm __volatile("sgdt %0" : "=m" (gdtr));
	return (gdtr);
}

static __inline unsigned int
rgs(void)
{
	unsigned int sel;
	__asm __volatile("movl %%gs,%0" : "=rm" (sel));
	return (sel);
}

static __inline u_int64_t
ridt(void)
{
	u_int64_t idtr;
	__asm __volatile("sidt %0" : "=m" (idtr));
	return (idtr);
}



static __inline u_int16_t
rtr(void)
{
	u_int16_t tr;
	__asm __volatile("str %0" : "=g" (tr));
	return (tr);
}


#if 0
static __inline void
load_fs(unsigned int sel)
{
	__asm __volatile("movl %0,%%fs" : : "rm" (sel));
}

static __inline void
load_gs(unsigned int sel)
{
	__asm __volatile("movl %0,%%gs" : : "rm" (sel));
}
#endif


static __inline void
lidt(struct region_descriptor *addr)
{
	__asm __volatile("lidt (%0)" : : "r" (addr));
}


static __inline void
ltr(u_int16_t sel)
{
	__asm __volatile("ltr %0" : : "r" (sel));
}

static __inline unsigned int
rdr0(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr0,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr0(unsigned int dr0)
{
	__asm __volatile("movl %0,%%dr0" : : "r" (dr0));
}

//#endif

static __inline unsigned int
rdr1(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr1,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr1(unsigned int dr1)
{
	__asm __volatile("movl %0,%%dr1" : : "r" (dr1));
}

static __inline unsigned int
rdr2(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr2,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr2(unsigned int dr2)
{
	__asm __volatile("movl %0,%%dr2" : : "r" (dr2));
}

static __inline unsigned int
rdr3(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr3,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr3(unsigned int dr3)
{
	__asm __volatile("movl %0,%%dr3" : : "r" (dr3));
}

static __inline unsigned int
rdr4(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr4,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr4(unsigned int dr4)
{
	__asm __volatile("movl %0,%%dr4" : : "r" (dr4));
}

static __inline unsigned int
rdr5(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr5,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr5(unsigned int dr5)
{
	__asm __volatile("movl %0,%%dr5" : : "r" (dr5));
}


static __inline unsigned int
rdr7(void)
{
	unsigned int	data;
	__asm __volatile("movl %%dr7,%0" : "=r" (data));
	return (data);
}

static __inline void
load_dr7(unsigned int dr7)
{
	__asm __volatile("movl %0,%%dr7" : : "r" (dr7));
}

static __inline register_t
intr_disable(void)
{
	register_t eflags;

	eflags = read_eflags();
	disable_intr();
	return (eflags);
}

static __inline void
intr_restore(register_t eflags)
{
	write_eflags(eflags);
}



/*
static __inline void
set_esp(unsigned int espv)
{
	__asm __volatile("movl %0, %%esp" : "=rm" (espv));
}
*/



#define set_esp(espv) __asm __volatile("movl %0, %%esp" : "=rm" (espv))

#endif

#endif	/* __GNUC__ */
#endif	/* ASSEMBLER */

#endif	/* _I386_PROC_REG_H_ */
