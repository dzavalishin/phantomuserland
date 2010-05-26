#define DEBUG_MSG_PREFIX "lpt"
#include "debug_ext.h"
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include "driver_map.h"

#include <i386/pio.h>
#include <phantom_libc.h>

#include "hal.h"

/* machine independent definitions, it shall only depend on the ppbus
 * parallel port model */

					/* PIN */
#define	LPS_NERR		0x08	/* 15  printer no error */
#define	LPS_SEL			0x10	/* 13  printer selected */
#define	LPS_OUT			0x20	/* 12  printer out of paper */
#define	LPS_NACK		0x40	/* 10  printer no ack of data */
#define	LPS_NBSY		0x80	/* 11  printer busy */

#define	LPC_STB			0x01	/*  1  strobe data to printer */
#define	LPC_AUTOL		0x02	/* 14  automatic linefeed */
#define	LPC_NINIT		0x04	/* 16  initialize printer */
#define	LPC_SEL			0x08	/* 17  printer selected */
#define	LPC_ENA			0x10	/*  -  enable IRQ */



static int lpt_addr = 0;
//static int sc_control = LPC_SEL|LPC_NINIT;

/*
 * Internal routine to lptprobe to do port tests of one byte value
 */
static int
lpt_port_test(int addr, u_char data, u_char mask)
{
    int	temp;
    int timeout;

    data = data & mask;

    SHOW_FLOW(1, "probe LPT @0x%X", addr);
    outb(addr, data);

    timeout = 10000;
    do {
        //DELAY(10);
        temp = inb(addr) & mask;
    }
    while (temp != data && --timeout);
    //lprintf(("out=%x\tin=%x\ttout=%d\n", data, temp, timeout));
    return (temp == data);
}


static int
lpt_detect(int port)
{
    static u_char	testbyte[18] = {
        0x55,			/* alternating zeros */
        0xaa,			/* alternating ones */
        0xfe, 0xfd, 0xfb, 0xf7,
        0xef, 0xdf, 0xbf, 0x7f,	/* walking zero */
        0x01, 0x02, 0x04, 0x08,
        0x10, 0x20, 0x40, 0x80	/* walking one */
    };
    int		i, status;

    status = 1;				/* assume success */

    for (i = 0; i < 18 && status; i++)
        if (!lpt_port_test(port, testbyte[i], 0xff)) {
            status = 0;
            goto end_probe;
        }

end_probe:
    /* write 0's to control and data ports */
    outb(port, 0);
    outb(port+2, 0);

    return (status);
}


// TODO this is to be moved to driver_map.c and be kept in driver tables
static int seq_number = 0;

phantom_device_t * driver_isa_lpt_probe( int port, int irq, int stage )
{
    (void) irq;
    (void) stage;
    //if(lpt_addr)        return 0; // just one instance yet!

    if( !lpt_detect(port) )
        return 0;

    lpt_addr = port;

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));
    dev->name = "LPT";
    dev->seq_number = seq_number++;

    return dev;
}


#define	LPS_INVERT	(LPS_NBSY | LPS_NACK |           LPS_SEL | LPS_NERR)
#define	LPS_MASK	(LPS_NBSY | LPS_NACK | LPS_OUT | LPS_SEL | LPS_NERR)
#define	NOT_READY(ppbus) ((ppb_rstr(ppbus)^LPS_INVERT)&LPS_MASK)

/*
 * lpt_pushbytes()
 *	Workhorse for actually spinning and writing bytes to printer
 *	Derived from lpa.c
 *	Originally by ?
 *
 *	This code is only used when we are polling the port
 * /
static int
lpt_pushbyte(char ch)
{
    int spin, err, tic;

    //
    // Wait for printer ready.
    // Loop 20 usecs testing BUSY bit, then sleep
    // for exponentially increasing timeout. (vak)
    //
    for (spin = 0; NOT_READY(ppbus) && spin < MAX_SPIN; ++spin)
        DELAY(1);	// XXX delay is NOT this accurate!
    if (spin >= MAX_SPIN) {
        tic = 0;
        while (NOT_READY(ppbus)) {
            // Now sleep, every cycle a little longer ..
            tic = tic + tic + 1;
            // But no more than 10 seconds. (vak)
            if (tic > MAX_SLEEP)
                tic = MAX_SLEEP;
            err = tsleep(dev, LPPRI,
                         LPT_NAME "poll", tic);
            if (err != EWOULDBLOCK) {
                return (err);
            }
        }
    }

    // output data
    ppb_wdtr(ppbus, ch);
    // strobe
    ppb_wctr(ppbus, sc_control|LPC_STB);
    ppb_wctr(ppbus, sc_control);


    return(0);
}
*/



/*

The port BASE+0 (Data port) controls the data signals of the port (D0 to D7 for bits 0 to 7, respectively; states: 0 = low (0 V), 1 = high (5 V)). A write to this port latches the data on the pins. A read returns the data last written in standard or extended write mode, or the data in the pins from another device in extended read mode.

The port BASE+1 (Status port) is read-only, and returns the state of the following input signals:

* Bits 0 and 1 are reserved.
* Bit 2 IRQ status (not a pin, I don't know how this works)
* Bit 3 ERROR (1=high)
* Bit 4 SLCT (1=high)
* Bit 5 PE (1=high)
* Bit 6 ACK (1=high)
* Bit 7 -BUSY (0=high)

The port BASE+2 (Control port) is write-only (a read returns the data last written), and controls the following status signals:

* Bit 0 -STROBE (0=high)
* Bit 1 -AUTO_FD_XT (0=high)
* Bit 2 INIT (1=high)
* Bit 3 -SLCT_IN (0=high)
* Bit 4 enables the parallel port IRQ (which occurs on the low-to-high transition of ACK) when set to 1.
* Bit 5 controls the extended mode direction (0 = write, 1 = read), and is completely write-only (a read returns nothing useful for this bit).
* Bits 6 and 7 are reserved.


*/










#if 0

#define DATA PORTADDRESS+0
#define STATUS PORTADDRESS+1
#define CONTROL PORTADDRESS+2


int interflag; /* Interrupt Flag */


void interrupt parisr()  /* Interrupt Service Routine (ISR) */
{
    interflag = 1;
}

void main(void)
{
    int c;
    int intno;    /* Interrupt Vector Number */

    outportb(CONTROL, inportb(CONTROL) & 0xDF); /* Make sure port is in Forward Direction */
    outportb(DATA,0xFF);

    setvect(intno, parisr);      /* Set New Interrupt Vector Entry */

    outportb(picaddr+1,inportb(picaddr+1) & (0xFF - picmask)); /* Un-Mask Pic */
    outportb(CONTROL, inportb(CONTROL) | 0x10); /* Enable Parallel Port IRQ's */

    clrscr();
    printf("Parallel Port Interrupt Polarity Tester\n");
    printf("IRQ %d : INTNO %02X : PIC Addr 0x%X : Mask 0x%02X\n",IRQ,intno,picaddr,picmask);
    interflag = 0; /* Reset Interrupt Flag */
    delay(10);
    outportb(DATA,0x00); /* High to Low Transition */
    delay(10);           /* Wait */
    if (interflag == 1) printf("Interrupts Occur on High to Low Transition of ACK.\n");
    else
    {
        outportb(DATA,0xFF); /* Low to High Transition */
        delay(10);           /* wait */
        if (interflag == 1) printf("Interrupts Occur on Low to High Transition of ACK.\n");
        else printf("No Interrupt Activity Occurred. \nCheck IRQ Number, Port Address and Wiring.");
    }

    outportb(CONTROL, inportb(CONTROL) & 0xEF); /* Disable Parallel Port IRQ's */
    outportb(picaddr+1,inportb(picaddr+1) | picmask); /* Mask Pic */
    setvect(intno, oldhandler); /* Restore old Interrupt Vector Before Exit */
}

#endif


