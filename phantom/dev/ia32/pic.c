/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * PC legacy (8259) interrupt controller
 *
**/


//#include <platforms.h>

#include <phantom_types.h>
#include <phantom_assert.h>
#include <phantom_libc.h>

//#include <i386/ipl.h>
#include <i386/isa/pic_regs.h>
#include <i386/isa/pic.h>

#include <i386/pio.h>

#include <hal.h>

#define MASKOFF 1

//spl_t	curr_ipl;
//int	pic_mask[NSPL];
//int	curr_pic_mask;

//int	iunit[NINTR] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

int 	nintr = NINTR;
int	npics = NPICS;

//static char	*master_icw, *master_ocw, *slaves_icw, *slaves_ocw;
static int master_icw, master_ocw, slaves_icw, slaves_ocw;

u_short PICM_ICW1, PICS_ICW1; //, PICM_OCW1, PICS_OCW1 ;
u_short PICM_OCW2, PICS_OCW2 ; // PICM_ICW2, PICS_ICW2,
u_short PICM_ICW3, PICM_OCW3, PICS_ICW3, PICS_OCW3 ;
u_short PICM_ICW4, PICS_ICW4 ;

/*
** picinit() - This routine
**		* Establishes a table of interrupt vectors
**		* Establishes a table of interrupt priority levels
**		* Establishes a table of interrupt masks to be put
**			in the PICs.
**		* Establishes location of PICs in the system
**		* Initialises them
**
**	At this stage the interrupt functionality of this system should be
**	coplete.
**
*/


/*
** Initialise the PICs , master first, then the slave.
** All the register field definitions are described in pic.h, also
** the settings of these fields for the various registers are selected.
**
*/

void
phantom_pic_init(unsigned char master_base, unsigned char slave_base)
{

    //u_short i;

    //asm("cli");
    int ie = hal_save_cli();


    /*
     ** 2. Generate addresses to each PIC port.
     */

    master_icw = PIC_MASTER_ICW;
    master_ocw = PIC_MASTER_OCW;
    slaves_icw = PIC_SLAVE_ICW;
    slaves_ocw = PIC_SLAVE_OCW;

    /*
     ** 3. Select options for each ICW and each OCW for each PIC.
     */

    PICM_ICW1 =
        (ICW_TEMPLATE | EDGE_TRIGGER | ADDR_INTRVL8 | CASCADE_MODE | ICW4__NEEDED);

    PICS_ICW1 =
        (ICW_TEMPLATE | EDGE_TRIGGER | ADDR_INTRVL8 | CASCADE_MODE | ICW4__NEEDED);

    //PICM_ICW2 = PICM_VECTBASE;
    //PICS_ICW2 = PICS_VECTBASE;

    PICM_ICW3 = ( SLAVE_ON_IR2 );
    PICS_ICW3 = ( I_AM_SLAVE_2 );

    PICM_ICW4 =
        (SNF_MODE_DIS | NONBUFD_MODE | NRML_EOI_MOD | I8086_EMM_MOD);
    PICS_ICW4 =
        (SNF_MODE_DIS | NONBUFD_MODE | NRML_EOI_MOD | I8086_EMM_MOD);


    //PICM_OCW1 = (curr_pic_mask & 0x00FF);
    //PICS_OCW1 = ((curr_pic_mask & 0xFF00)>>8);

    PICM_OCW2 = NON_SPEC_EOI;
    PICS_OCW2 = NON_SPEC_EOI;

    PICM_OCW3 = (OCW_TEMPLATE | READ_NEXT_RD | READ_IR_ONRD );
    PICS_OCW3 = (OCW_TEMPLATE | READ_NEXT_RD | READ_IR_ONRD );


    /*
     ** 4.	Initialise master - send commands to master PIC
     */

    outb ( master_icw, PICM_ICW1 );
    //outb ( master_ocw, PICM_ICW2 );
    outb ( master_ocw, master_base );
    outb ( master_ocw, PICM_ICW3 );
    outb ( master_ocw, PICM_ICW4 );

#if MASKOFF
    outb ( master_ocw, 0xFF ); // Mask off all
#endif
    //outb ( master_icw, PICM_OCW3 ); // setup to read IR

    /*
     ** 5.	Initialise slave - send commands to slave PIC
     */

    outb ( slaves_icw, PICS_ICW1 );
    //outb ( slaves_ocw, PICS_ICW2 );
    outb ( slaves_ocw, slave_base );
    outb ( slaves_ocw, PICS_ICW3 );
    outb ( slaves_ocw, PICS_ICW4 );


#if MASKOFF
    outb ( slaves_ocw, 0xFF ); // Mask off all
#endif
    //outb ( slaves_icw, PICS_OCW3 ); // setup to read IR

    /*
     ** 6. Initialise interrupts
     */
    //outb ( master_ocw, PICM_OCW1 );


    // give out ack so that if there were any spurious interrupt,
    // we cleaned it up.
    outb ( master_icw, NON_SPEC_EOI);
    outb ( slaves_icw, NON_SPEC_EOI);


    //asm("sti");
    if(ie) hal_sti();

}


/*
 intnull(unit_dev)
 {
 printf("intnull(%d)\n", unit_dev);
 }

 int prtnull_count = 0;
 prtnull(unit)
{
++prtnull_count;
}

*/


void phantom_interrupt_ack(unsigned int n)
{
    if(n < 16) {
        // 8239 controlled interrupt
        if(n > 7)
            //out8(0x20, 0xa0);	// EOI to pic 2
            outb ( slaves_icw, NON_SPEC_EOI);

        //out8(0x20, 0x20);	// EOI to pic 1
        outb ( master_icw, NON_SPEC_EOI);
    }
}


//! \brief Disable an IRQ line.
//! \param irq_no The IRQ line to disable.
void phantom_pic_disable_irq(unsigned int irq_no)
{
    u_int8_t mask;

    assert( irq_no < NINTR );

    int ie = hal_save_cli();
    if (irq_no < 8)
    {
        // Master
        mask = inb(PIC_MASTER_OCW);
        mask |= 1 << irq_no;
        outb(PIC_MASTER_OCW, mask);
    }
    else
    {
        // Slave
        mask = inb(PIC_SLAVE_OCW);
        mask |= 1 << (irq_no-8);
        outb(PIC_SLAVE_OCW, mask);
    }
    if(ie) hal_sti();
}



//! \brief Enable an IRQ line.
//! \param irq_no The IRQ line to enable.
void phantom_pic_enable_irq(unsigned int irq_no)
{
    u_int8_t mask;

    assert( irq_no < NINTR );

    int ie = hal_save_cli();
    if (irq_no < 8)
    {
        // Master
        mask = inb(PIC_MASTER_OCW);
        mask &= ~(1 << irq_no);
        outb(PIC_MASTER_OCW, mask);
    }
    else
    {
        // Cascade (slave) input on master
        outb(PIC_MASTER_OCW, inb(PIC_MASTER_OCW) & ~(1 << 2));
        // Slave
        mask = inb(PIC_SLAVE_OCW);
        mask &= ~(1 << (irq_no-8));
        outb(PIC_SLAVE_OCW, mask);
    }
    if(ie) hal_sti();

#if 0
    {
        int m = phantom_pic_get_irqmask();
        printf("eirq mask = %X\n", m );
    }
#endif
}



//! \brief Check if an IRQ is pending.
//! \param irq_no The IRQ to check.
int phantom_pic_is_irq_pending(unsigned int irq_no)
{
    assert( irq_no < NINTR );

    if (irq_no < 8)
    {
        // Master
        return inb(PIC_MASTER_ICW) & (1 << irq_no);
    }
    else
    {
        // Slave
        return inb(PIC_SLAVE_ICW) & (1 << (irq_no-8));
    }
}




int
phantom_pic_get_irqmask(void)
{
    return (inb(PIC_SLAVE_OCW) << 8) | inb(PIC_MASTER_OCW);
}

void
phantom_pic_set_irqmask(int mask)
{
    unsigned int mmask, smask;

    mmask = mask & 0xff;
    smask = (mask >> 8) & 0xff;

    // Allways enable slave (cascade) in
    mmask &= ~(1 << 2);

    int ie = hal_save_cli();

    outb(PIC_MASTER_OCW, mmask);
    outb(PIC_SLAVE_OCW, smask);

    if(ie) hal_sti();
}


void
phantom_pic_disable_all(void)
{
    phantom_pic_set_irqmask(0xFFFF);
}


/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
Copyright (c) 1988,1989 Prime Computer, Inc.  Natick, MA 01760
All Rights Reserved.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and
without fee is hereby granted, provided that the above
copyright notice appears in all copies and that both the
copyright notice and this permission notice appear in
supporting documentation, and that the name of Prime
Computer, Inc. not be used in advertising or publicity
pertaining to distribution of the software without
specific, written prior permission.

THIS SOFTWARE IS PROVIDED "AS IS", AND PRIME COMPUTER,
INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
NO EVENT SHALL PRIME COMPUTER, INC.  BE LIABLE FOR ANY
SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN ACTION OF CONTRACT, NEGLIGENCE, OR
OTHER TORTIOUS ACTION, ARISING OUR OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

