#warning unused
#if 0

#if HAVE_NET


#define FIXME 0

#define  outbreverse(data,port) outb(port,data)
#define  outbreverse_p(data,port) outb_p(port,data)

#define  outwreverse(data,port) outw(port,data)


/* $Id: //depot/blt/srv/ne2000/ne2000.c#2 $
 **
 ** Copyright 1998 Brian J. Swetland
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions
 ** are met:
 ** 1. Redistributions of source code must retain the above copyright
 **    notice, this list of conditions, and the following disclaimer.
 ** 2. Redistributions in binary form must reproduce the above copyright
 **    notice, this list of conditions, and the following disclaimer in the
 **    documentation and/or other materials provided with the distribution.
 ** 3. The name of the author may not be used to endorse or promote products
 **    derived from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 ** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 ** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 ** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 ** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 ** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 ** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file contains a network driver core for the NE2000 card.
 // Use at your own risk!    (C) Copyright 1997,
 // Douglas Armstrong	site@xnet.com	drarmstr@uiuc.edu	10/11/97
 // ...
 // derived from National Semiconductor datasheets and application notes
 // as well as the Linux driver by Donald Becker */

/* History Log:
 10-25-97	Setup under CVS					drarmstr
 10-27-97	Added alloc_buffer() and free_buffer()		drarmstr
 12-08-97	Finished scatter gather				drarmstr
 03-03-98	Minor bug / Output cleanup			drarmstr
 03-04-98	snic->tx_buf[]  packet_buf -> packet_buf*	drarmstr
 03-04-98	added passback pointer to notify and snic	drarmstr
 */


#include <phantom_libc.h>
#include "hal.h"


#include <ia32/pio.h>
#include "ne2000.h"
#include "ne2k.h"
#include "ne2000err.h"

#define kprintf printf

//extern void kprintf(char *, ...);	/* Make sure your OS has these...*/
//extern void idle();
//extern unsigned long ticks;
/* replace with a better timeout method, like wait() */

#ifdef XXX
static char *BLARGH = 0xB8000 + 160*23 + 152;
#define t(a) { BLARGH[0] = #a[0]; }
#define T(a) { BLARGH[2] = #a[0]; }
#else
#define t(a) {}
#define T(a) {}
#endif

/* we may have to change inb and outb to inb_p and outb_p.
 // Note, none of this code is gauranteed to be reentrant.

 // Note, don't create two nics to the same card and then try to use both
 // of them.  The results will be unpredictable.

 // This violates IO resource manegment if your OS handles that, but it
 // works for now.  Make sure that the driver has privlige access to the
 // mapped I/O space and the IRQ.*/
static unsigned int default_ports[] = { 0x300, 0x280, 0x320, 0x340, 0x360, 0 };

/* internal procedure, don't call this directly.*/
int nic_probe(int addr) {
    uint regd;
    uint state=inb(addr);		/* save command state */

    if(inb(addr==0xff)) return ERRNOTFOUND;

    outbreverse(NIC_DMA_DISABLE | NIC_PAGE1 | NIC_STOP, addr);
    regd=inb(addr + 0x0d);
    outbreverse(0xff, addr + 0x0d);
    outbreverse(NIC_DMA_DISABLE | NIC_PAGE0, addr);
    inb(addr + FAE_TALLY);	/* reading CNTR0 resets it.*/
    if(inb(addr + FAE_TALLY)) {	/* counter didn't clear so probe fails*/
        outbreverse(state,addr);	/* restore command state*/
        outbreverse(regd,addr + 0x0d);
        return ERRNOTFOUND; }

    return addr;	/* network card detected at io addr;*/
}

/* Detects for presence of NE2000 card.  Will check given io addr that
 // is passed to it as well as the default_ports array.  Returns ERRNOTFOUND
 // if no card is found, i/o address otherwise.  This does conduct an ISA
 // probe, so it's not always a good idea to run it.  If you already have
 // the information, then you can just use that with nic_init().*/
int nic_detect(int given) {
    int found;
    int f;

    kprintf("NE2000: detecting card...");

    if(given) if((found=nic_probe(given))!=ERRNOTFOUND)
    {
        kprintf("found at given:0x%x\n",given);
        return found;
    }

    /* probe for PCI clones here....  or not...*/

    for(f=0;default_ports[f]!='\0';f++)
    {
        // perform resource manegment here
        if((found=nic_probe(default_ports[f]))!=ERRNOTFOUND)
        {
            kprintf("found at default:0x%x\n",default_ports[f]);
#if 0
            snic nic;
            unsigned char prom[64*1024];

            nic_init( &nic, default_ports[f], prom, 0 );
#endif

            return found;
        }
    }

    kprintf("none found.\n");
    return ERRNOTFOUND;
}


#if 0

void nic_isr(snic *nic);

/* This initializes the NE2000 card.  If it turns out the card is not
 // really a NE2000 after all then it will return ERRNOTFOUND, else NOERR
 // It also dumps the prom into buffer prom for the upper layers..
 // Pass it a nic with a null iobase and it will initialize the structure for
 // you, otherwise it will just reinitialize it. */
int nic_init(snic* nic, int addr, unsigned char *prom, unsigned char *manual)
{
    uint f;
    kprintf("NE2000: reseting NIC card...");
    if(!nic->iobase) {
        nic->iobase=addr;
        nic_stat_clear(&nic->stat);
        nic->pstart=0; nic->pstop=0; nic->wordlength=0;
        nic->current_page=0;
        nic->notify=NULL;
        for(f=0;f<MAX_TX;f++) {
            nic->tx_packet[f]= NULL;
            /*			nic->tx_packet[f].count=0;
             nic->tx_packet[f].buf=NULL;
             nic->tx_packet[f].page=0; */
        }
        nic->last_tx=NULL;
        nic->busy=0;
        nic->sending=0;

        hal_irq_alloc( 5, nic_isr, nic, HAL_IRQ_SHAREABLE );


    } else {
        if(!nic->iobase || nic->iobase!=addr) return ERR;
    }

    {
        char reset_v = inb(addr + NE_RESET);
        outbreverse(reset_v, addr + NE_RESET);	/* reset the NE2000*/
    }

    while(!(inb_p(addr+INTERRUPTSTATUS) & ISR_RST)) {
        /* TODO insert timeout code here.*/
        hal_sleep_msec(1);
    }
    //kprintf("done.\n");
    outbreverse_p(0xff,addr + INTERRUPTSTATUS);	/* clear all pending ints*/

    // Initialize registers
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_STOP, addr);	/* enter page 0*/
    outbreverse_p(DCR_DEFAULT, addr + DATACONFIGURATION);
    outbreverse_p(0x00, addr + REMOTEBYTECOUNT0);
    outbreverse_p(0x00, addr + REMOTEBYTECOUNT1);
    outbreverse_p(0x00, addr + INTERRUPTMASK);	/* mask off all irq's*/
    outbreverse_p(0xff, addr + INTERRUPTSTATUS);	/* clear pending ints*/
    outbreverse_p(RCR_MON, RECEIVECONFIGURATION);	/* enter monitor mode*/
    outbreverse_p(TCR_INTERNAL_LOOPBACK, TRANSMITCONFIGURATION); /* internal loopback*/

    nic->wordlength=nic_dump_prom(nic,prom);
    if(prom[14]!=0x57 || prom[15]!=0x57) {
        kprintf("NE2000: PROM signature does not match NE2000 0x57.\n");
        return ERRNOTFOUND;
    }
    kprintf("NE2000: PROM signature matches NE2000 0x57.\n");

    /* if the wordlength for the NE2000 card is 2 bytes, then
     // we have to setup the DP8390 chipset to be the same or
     // else all hell will break loose.*/
    if(nic->wordlength==2) {
        outbreverse_p(DCR_DEFAULT_WORD, addr + DATACONFIGURATION);
    }
    nic->pstart=(nic->wordlength==2) ? PSTARTW : PSTART;
    nic->pstop=(nic->wordlength==2) ? PSTOPW : PSTOP;

    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_STOP, addr);
    outbreverse_p(nic->pstart, addr + TRANSMITPAGE);	/* setup local buffer*/
    outbreverse_p(nic->pstart + TXPAGES, addr + PAGESTART);
    outbreverse_p(nic->pstop - 1, addr + BOUNDARY);
    outbreverse_p(nic->pstop, addr + PAGESTOP);
    outbreverse_p(0x00, addr + INTERRUPTMASK);	/* mask off all irq's*/
    outbreverse_p(0xff, addr + INTERRUPTSTATUS);	/* clear pending ints*/
    nic->current_page=nic->pstart + TXPAGES;

    /* put physical address in the registers */
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE1 | NIC_STOP, addr);  /* switch to page 1 */
    if(manual) for(f=0;f<6;f++) outbreverse_p(manual[f], addr + PHYSICAL + f);
    else for(f=0;f<LEN_ADDR;f++) outbreverse_p(prom[f], addr + PHYSICAL + f);
    kprintf("NE2000: Physical Address- ");
    kprintf("%X:%X:%X:%X:%X:%X\n",
            inb(addr+PAR0),inb(addr+PAR1),inb(addr+PAR2),
            inb(addr+PAR3),inb(addr+PAR4),inb(addr+PAR5));

    /* setup multicast filter to accept all packets*/
    for(f=0;f<8;f++) outbreverse_p(0xFF, addr + MULTICAST + f);

    outbreverse_p(nic->pstart+TXPAGES, addr + CURRENT);
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_STOP, addr); /* switch to page 0 */

    return NOERR;
}





/* dumps the prom into a 16 byte buffer and returns the wordlength of
 // the card.
 // You should be able to make this procedure a wrapper of nic_block_input(). */
int nic_dump_prom(snic *nic, unsigned char *prom) {
    uint f;
    int iobase=nic->iobase;
    char wordlength=2;		/* default wordlength of 2 */
    unsigned char dump[32];

    outbreverse_p(32, iobase + REMOTEBYTECOUNT0);	  /* read 32 bytes from DMA->IO */
    outbreverse_p(0x00, iobase + REMOTEBYTECOUNT1);  /*  this is for the PROM dump */
    outbreverse_p(0x00, iobase + REMOTESTARTADDRESS0); /* configure DMA for 0x0000 */
    outbreverse_p(0x00, iobase + REMOTESTARTADDRESS1);
    outbreverse_p(NIC_REM_READ | NIC_START, iobase);

    for(f=0;f<32;f+=2) {
        dump[f]=inb_p(iobase + NE_DATA);
        dump[f+1]=inb_p(iobase + NE_DATA);
        if(dump[f]!=dump[f+1]) wordlength=1;
    }

    /* if wordlength is 2 bytes, then collapse prom to 16 bytes */
    for(f=0;f<LEN_PROM;f++) prom[f]=dump[f+((wordlength==2)?f:0)];
    /*	kprintf("NE2000: prom dump - ");
     for(f=0;f<LEN_PROM;f++) kprintf("%X",prom[f]);
     kprintf("\n");	*/

    return wordlength;
}

void nic_stat_clear(nic_stat *that)
{
    that->rx_packets=0;
    that->tx_buffered=0;		that->tx_packets=0;
    that->errors.frame_alignment=0;	that->errors.crc=0;
    that->errors.missed_packets=0;	that->errors.rx=0;
    that->errors.rx_size=0;		that->errors.rx_dropped=0;
    that->errors.rx_fifo=0;		that->errors.rx_overruns=0;
    that->errors.tx_collisions=0;
}




void nic_isr(snic *nic) {
    uint isr;	/* Illinois Sreet Residence Hall */
    uint overload;
    if(!nic || !nic->iobase) return;    /* make sure card was initialized */
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0, nic->iobase);
    overload=MAX_LOAD+1;
    while((isr=inb_p(nic->iobase+INTERRUPTSTATUS))) {
        if((--overload)<=0) break;
        if(isr & ISR_OVW)			nic_overrun(nic);
        else if(isr & (ISR_PRX | ISR_RXE))	nic_rx(nic);
        if(isr & ISR_PTX)			nic_tx(nic);
        else if(isr & ISR_TXE)			nic_tx_err(nic);
        if(isr & ISR_CNT)			nic_counters(nic);
        if(isr & ISR_RDC) outbreverse_p(ISR_RDC, nic->iobase+ INTERRUPTSTATUS);
        outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, nic->iobase);
    }
    if(isr) {
        outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, nic->iobase);
        if(!overload) {
            kprintf("NE2000: Too many requests in ISR.  ISR:%X  MaxLoad:%X\n",
                    isr, MAX_LOAD);
            outbreverse_p(ISR_ALL, nic->iobase + INTERRUPTSTATUS); // clear
        } else {
            kprintf("NE2000: Unhandeled interrupt, ISR:%X\n",isr);
            outbreverse_p(0xff, nic->iobase + INTERRUPTSTATUS);
            // Ack it anyway
        }
    }
}


void nic_counters(snic *nic) {
    nic->stat.errors.frame_alignment+=inb_p(nic->iobase+FAE_TALLY);
    nic->stat.errors.crc+=inb_p(nic->iobase+CRC_TALLY);
    nic->stat.errors.missed_packets+=inb_p(nic->iobase+MISS_PKT_TALLY);
    /* reading the counters on the DC8390 clears them. */
    outbreverse_p(ISR_CNT, nic->iobase + INTERRUPTSTATUS); /* ackkowledge int */
}

void nic_rx(snic *nic) {
    uint packets=0;	uint frame;	uint rx_page;	uint rx_offset;
    uint len;	uint next_pkt;	uint numpages;
    int iobase=nic->iobase;
    buffer_header header;
    if(!nic || !nic->iobase) return;
    while(packets<MAX_RX) {
        outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE1, iobase); /*curr is on page 1 */
        rx_page=inb_p(iobase + CURRENT);	/* get current page */
        outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0, iobase);
        frame=inb(iobase + BOUNDARY)+1;
        /* we add one becuase boundary is a page behind
         // as pre NS notes to help in overflow problems */
        if(frame>=nic->pstop) frame=nic->pstart+TXPAGES;
        /* circual buffer */

        if(frame != nic->current_page) {
            kprintf("NE2000: ERROR: mismatched read page pointers!\n");
            kprintf("NE2000: NIC-Boundary:%x  dev-current_page:%x\n",
                    frame, nic->current_page); }

        /*		kprintf("Boundary-%x  Current-%x\n",frame-1,rx_page); */
        if(frame==rx_page) break;	/* all frames read */

        rx_offset=frame << 8;  /* current ptr in bytes(not pages) */

        nic_get_header(nic,frame,&header);
        len=header.count - sizeof(buffer_header);
        /* length of packet */
        next_pkt=frame + 1 + ((len+4)>>8); /* next packet frame */

        numpages=nic->pstop-(nic->pstart+TXPAGES);
        if(	   (header.next!=next_pkt)
                   && (header.next!=next_pkt + 1)
                   && (header.next!=next_pkt - numpages)
                   && (header.next != next_pkt +1 - numpages)){
            kprintf("NE2000: ERROR: Index mismatch.   header.next:%X  next_pkt:%X frame:%X\n",
                    header.next,next_pkt,frame);
            nic->current_page=frame;
            outbreverse(nic->current_page-1, iobase + BOUNDARY);
            nic->stat.errors.rx++;
            continue;
        }

        if(len<60 || len>1518) {
            kprintf("NE2000: invalid packet size:%d\n",len);
            nic->stat.errors.rx_size++;
        } else if((header.status & 0x0f) == RSR_PRX) {
            /* We have a good packet, so let's recieve it! */

            packet_data *newpacket=alloc_buffer_data(len);
            if(!newpacket) {
                kprintf("NE2000: ERROR: out of memory!\n");
                nic->stat.errors.rx_dropped++;
                nic_block_input(nic,NULL,len,rx_offset+
                                sizeof(buffer_header));
            } else {
                nic_block_input(nic,newpacket->ptr,newpacket->len,
                                rx_offset+sizeof(buffer_header));
                /* read it */

                if(nic->notify) nic->notify(nic->kore,newpacket);
                else free_buffer_data(newpacket);
                /* NOTE: you are responsible for deleting this buffer. */

                nic->stat.rx_packets++;
            }
        } else {
            kprintf("NE2000: ERROR: bad packet.  header-> status:%X next:%X len:%x.\n",
                    header.status,header.next,header.count);
            if(header.status & RSR_FO) nic->stat.errors.rx_fifo++;
        }
        /*		kprintf("frame:%x  header.next:%x  next_pkt:%x\n",
         frame,header.next,next_pkt); */
        next_pkt=header.next;

        if(next_pkt >= nic->pstop) {
            kprintf("NE2000: ERROR: next frame beyond local buffer!  next:%x.\n",
                    next_pkt);
            next_pkt=nic->pstart+TXPAGES;
        }

        nic->current_page=next_pkt;
        outbreverse_p(next_pkt-1, iobase + BOUNDARY);
    }
    outbreverse_p(ISR_PRX | ISR_RXE, iobase + INTERRUPTSTATUS);	/* ack int */
}




















//#if 0


/* This registers the function that will be called when a new packet
 // comes in. */
void nic_register_notify(snic *nic,
                         void (*newnotify)(void*,packet_data*), void *passback){
    nic->kore= passback;
    nic->notify= newnotify;
}

/* start the NIC so it can actually recieve or transmit packets */
void nic_start(snic *nic, int promiscuous) {
    int iobase;
    kprintf("NE2000: Starting NIC.\n");
    if(!nic || !nic->iobase) {
        kprintf("NE2000: can't start a non-initialized card.\n");
        return; }
    iobase=nic->iobase;
    outbreverse(0xff, iobase + INTERRUPTSTATUS);
    outbreverse(IMR_DEFAULT, iobase + INTERRUPTMASK);
    outbreverse(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, iobase);
    outbreverse(TCR_DEFAULT, iobase + TRANSMITCONFIGURATION);
    if(promiscuous)
        outbreverse(RCR_PRO | RCR_AM, iobase + RECEIVECONFIGURATION);
    else
        outbreverse(RCR_DEFAULT, iobase + RECEIVECONFIGURATION);

    /* The following is debugging code! */
#ifdef SHIT
    kprintf("NE2000: Trying to fire off an IRQ.\n");
    outbreverse_p(0x50,iobase+INTERRUPTMASK);
    outbreverse_p(0x00,iobase+REMOTEBYTECOUNT0);
    outbreverse_p(0x00,iobase+REMOTEBYTECOUNT1);
    outbreverse_p(NIC_REM_READ | NIC_START,iobase);   /* this should fire off */
    outbreverse_p(IMR_DEFAULT,iobase+INTERRUPTMASK);  /* an interrupt... */
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, iobase);
    outbreverse_p(0xff,iobase+INTERRUPTSTATUS);
#endif
    /* End debugging code */
}

/* stops the NIC */
void nic_stop(snic *nic) {
    unsigned char tmp_buffer[16];
    if(!nic || !nic->iobase) return;    /* make sure card was initialized */
    nic_init(nic,nic->iobase,tmp_buffer,NULL);
}


/* You should call this before you just read the stats directlly from the
 // snic struct.  This procedure updates the counters */
nic_stat nic_get_stats(snic *nic) {
    nic->stat.errors.frame_alignment+=inb_p(nic->iobase + FAE_TALLY);
    nic->stat.errors.crc+=inb_p(nic->iobase + CRC_TALLY);
    nic->stat.errors.missed_packets+=inb_p(nic->iobase + MISS_PKT_TALLY);
    return nic->stat;
}


/* Since this could be called by more than one other device, it should
 // be properly protected for reentrancy.  We should put in a proper
 // semaphore or something, look into this.
 // NOTE: It is your responsibility to clean up the packet_buffer when you
 // are done calling this. */
/* TODO- Have it check how lonk a transmission is taking and attempt to
 restart the card if it's too long. */
int nic_send_packet(snic *nic, packet_buffer *buffer) {
    uint timeout;	uint f;	int iobase;
    /*	kprintf("nic_send_packet()\n"); */
    if(!buffer) return ERRARG;
    if(!buffer->count || !buffer->buf) return ERRARG;
    if(!nic || !nic->iobase) return ERR;
    iobase=nic->iobase;

    t(A);

    buffer->len=0;
    for(f=0;f<buffer->count;f++) buffer->len+=buffer->buf[f].len;
    if(buffer->len>MAX_LENGTH) return ERRTOOBIG;

    t(B);

    /* the following wait for anyother tasks that are calling
     // nic_send_packet() right now.  Note that this doesn't use
     // an atomic semaphore, so something MAY leak through. */
#if FIXME
    /*	timeout=ticks+10;	wait 10 ticks */
    timeout=ticks+100;
    while(nic->busy && ticks<=timeout) idle();
    /* Replace this with a proper timeout thing that doesn't use the
     // ticks method which will be diffrent on each OS. */
#else
    hal_sleep_msec(2);
#endif
    t(C);
    if(nic->busy) {
        kprintf("NE2000: ERROR: Card stalled, timeout.\n");
        return ERRTIMEOUT;
    }
    nic->busy=1;	/* mark as busy, replace with semaphore */

    t(D);

    outbreverse_p(0x00, iobase + INTERRUPTMASK);	/* mask ints for now */
    t(E);

    //timeout=ticks+TIMEOUT_TX;
    //while(idle(), ticks<=timeout)
    int tries = 10;
    while(tries-->0)
    {
        for(f=0;f<MAX_TX;f++) {
            if(!nic->tx_packet[f]) {
                t(F);

                /*			nic->tx_packet[f]=*buffer;*/
                nic->tx_packet[f]=buffer;
                nic->tx_packet[f]->page=nic->pstart + (f * MAX_PAGESPERPACKET);
                /*output page */

                /*			kprintf("NE2000: sending packet with count:%x on page:%X with buffer:%x\n",
                 buffer->count,buffer->page,buffer->buf); */
                t(>);

                nic_block_output(nic,nic->tx_packet[f]);

                t(<);

                if(!nic->sending) {
                    nic->send=f;	nic->sending=1;
                    /* now let's actually trigger the transmitter */
                    t(I);

                    if(nic_send(nic,f)<0) {
                        nic->sending=0;
                        break; }

                    /* note, the nic_tx() interrupt will mark this
                     // tx_packet buffer as free again once it
                     // confirms that the packet was sent. */
                }
                t(J);

                nic->stat.tx_buffered++;
                outbreverse_p(IMR_DEFAULT, iobase + INTERRUPTMASK); /* unmask */
                nic->busy=0;	/* V() */
                t(K);

                return NOERR;
            }
        }

        hal_sleep_msec( 1 );

    }

    t(L);

    outbreverse_p(IMR_DEFAULT, iobase + INTERRUPTMASK);	/* unmask */
    nic->busy=0;	/* V() */
    kprintf("NE2000: ERROR: Transmitter stalled, card timeout.\n");
    if(f==MAX_TX) {
        kprintf("NE2000: ERROR: There are no transmit buffers available.  TX stalled.\n");
        return ERRTIMEOUT;
    }
    return ERR;
}


void nic_overrun(snic *nic) {
    uint tx_status;	int iobase=nic->iobase;
    long starttime;	uint resend=0;
    kprintf("NE2000: Receive packet ring overrun!\n");
    if(!nic || !nic->iobase) return;
    tx_status=inb_p(iobase) & NIC_TRANSMIT;
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_STOP, iobase);
    nic->stat.errors.rx_overruns++;

    kprintf("BEFORE\n");

#if FIXME
    starttime=ticks;
    while(ticks-starttime<=10/*ticks to wait*/) idle();
    /* Arrgh!  TODO: Replace this whole crappy code with a decent
     // wait method.  We need to wait at least 1.6ms as per National
     // Semiconductor datasheets, but we should probablly wait a
     // little more to be safe. */
#else
    hal_sleep_msec(2);
#endif

    kprintf("AFTER\n");

    outbreverse_p(0x00, iobase + REMOTEBYTECOUNT0);
    outbreverse_p(0x00, iobase + REMOTEBYTECOUNT1);
    if(tx_status) {
        uint tx_completed=inb_p(iobase + INTERRUPTSTATUS) &
            (ISR_PTX | ISR_TXE);
        if(!tx_completed) resend=1;
    }

    outbreverse_p(TCR_INTERNAL_LOOPBACK, iobase + TRANSMITCONFIGURATION);
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, iobase);
    nic_rx(nic);	/* cleanup RX ring */
    outbreverse_p(ISR_OVW, iobase + INTERRUPTSTATUS);	/* ACK INT */

    outbreverse_p(TCR_DEFAULT, iobase + TRANSMITCONFIGURATION);
    if(resend)
        outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START | NIC_TRANSMIT,
               iobase);
}

/* This is the procedure that markst the transmit buffer as available again */
void nic_tx(snic *nic) {
    uint f;			int iobase=nic->iobase;
    uint status;
    /*	kprintf("nic_tx()\n"); */
    if(!nic || !nic->iobase) return;
    status=inb(iobase + TRANSMITSTATUS);

    if(!nic->tx_packet[nic->send]) {
        kprintf("NE2000: ERROR: Invalid transmison packet buffer.\n");
        return;
    }
    if(!nic->sending) kprintf("NE2000: ERROR: Invalid nic->sending value.\n");

    free_buffer(nic->tx_packet[nic->send]);
    nic->tx_packet[nic->send]= NULL;
    /*	nic->tx_packet[nic->send].count=0;	 mark buffer as available */
    /*	nic->tx_packet[nic->send].len=0;
     nic->tx_packet[nic->send].page=0; */

    for(f=0;f<MAX_TX;f++) {
        if(nic->tx_packet[f]) {
            kprintf("NE2000: DEBUG: transmitting secondary buffer:%X\n",f);
            nic->stat.tx_buffered++;
            nic->send=f;	nic->sending=1;
            nic_send(nic,f);	/* send a back-to-back buffer */
            break;
        }
    }
    if(f==MAX_TX) nic->sending=0;

    if(status & TSR_COL) nic->stat.errors.tx_collisions++;
    if(status & TSR_PTX) nic->stat.tx_packets++;
    else {
        if(status & TSR_ABT) {
            nic->stat.errors.tx_aborts++;
            nic->stat.errors.tx_collisions+=16; }
        if(status & TSR_CRS) nic->stat.errors.tx_carrier++;
        if(status & TSR_FU) nic->stat.errors.tx_fifo++;
        if(status & TSR_CDH) nic->stat.errors.tx_heartbeat++;
        if(status & TSR_OWC) nic->stat.errors.tx_window++;
    }

    outbreverse_p(ISR_PTX, iobase + INTERRUPTSTATUS);	/* ack int */
}

void nic_tx_err(snic *nic) {
    unsigned char tsr;	int iobase=nic->iobase;
    if(!nic || !nic->iobase) return;
    tsr=inb_p(nic->iobase);
    kprintf("NE2000: ERROR: TX error: ");
    if(tsr & TSR_ABT) kprintf("Too many collisions.\n");
    if(tsr & TSR_ND) kprintf("Not deffered.\n");
    if(tsr & TSR_CRS) kprintf("Carrier lost.\n");
    if(tsr & TSR_FU) kprintf("FIFO underrun.\n");
    if(tsr & TSR_CDH) kprintf("Heart attack!\n");

    outbreverse_p(ISR_TXE, iobase + INTERRUPTSTATUS);
    if(tsr & (TSR_ABT | TSR_FU)) {
        kprintf("NE2000: DEBUG: Attempting to retransmit packet.\n");
        nic_tx(nic);
    }
}



/* You should be able to make this procedure a wrapper of nic_block_input */
void nic_get_header(snic *nic, uint page, buffer_header *header) {
    int iobase=nic->iobase;	uint f;
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, iobase);
    outbreverse_p(sizeof(buffer_header), iobase + REMOTEBYTECOUNT0);
    outbreverse_p(0, iobase + REMOTEBYTECOUNT1);		/* read the header */
    outbreverse_p(0, iobase + REMOTESTARTADDRESS0); 	/* page boundary */
    outbreverse_p(page, iobase + REMOTESTARTADDRESS1); 	/* from this page */
    outbreverse_p(NIC_REM_READ | NIC_START, iobase);	/* start reading */

    if(nic->wordlength==2) for(f=0;f<(sizeof(buffer_header)>>1);f++)
        ((unsigned short *)header)[f]=inw(iobase+NE_DATA);
    else for(f=0;f<sizeof(buffer_header);f++)
        ((unsigned char *)header)[f]=inb(iobase+NE_DATA);
    /* Do these need to be *_p variants??? */
    outbreverse_p(ISR_RDC, iobase + INTERRUPTSTATUS);	/* finish DMA cycle */
}

int nic_send(snic *nic, uint buf) {
    uint len= (nic->tx_packet[buf]->len<MIN_LENGTH) ?
        MIN_LENGTH : nic->tx_packet[buf]->len;
    /*	kprintf("nic_send()\n"); */
    if(!nic->tx_packet[buf]) return ERR; /* this is bad */
    outbreverse_p(NIC_DMA_DISABLE |  NIC_PAGE0, nic->iobase);
    if(inb_p(nic->iobase + STATUS) & NIC_TRANSMIT) {
        kprintf("NE2000: ERROR: Transmitor busy.\n");
        free_buffer(nic->tx_packet[buf]);
        nic->tx_packet[buf]= NULL;
        /*                nic->tx_packet[buf].count=0;  mark as free again */
        return ERRTIMEOUT; }
    outbreverse_p(len & 0xff,nic->iobase+TRANSMITBYTECOUNT0);
    outbreverse_p(len >> 8,nic->iobase+TRANSMITBYTECOUNT1);
    outbreverse_p(nic->tx_packet[buf]->page,nic->iobase+TRANSMITPAGE);
    outbreverse_p(NIC_DMA_DISABLE | NIC_TRANSMIT | NIC_START,nic->iobase);
    return NOERR;
}

void nic_block_input(snic *nic, unsigned char *buf, uint len, uint offset) {
    int iobase=nic->iobase;	uint f;
    uint xfers=len;
    uint timeout=TIMEOUT_DMAMATCH;	uint addr;
    /*	kprintf("NE2000: RX: Length:%x  Offset:%x  ",len,offset); */
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, iobase);
    outbreverse_p(len & 0xff, iobase + REMOTEBYTECOUNT0);
    outbreverse_p(len >> 8, iobase + REMOTEBYTECOUNT1);
    outbreverse_p(offset & 0xff, iobase + REMOTESTARTADDRESS0);
    outbreverse_p(offset >> 8, iobase + REMOTESTARTADDRESS1);
    outbreverse_p(NIC_REM_READ | NIC_START, iobase);

    /* allow for no buffer */
    if(buf){
        if(nic->wordlength==2) {
            for(f=0;f<(len>>1);f++)
                ((unsigned short *)buf)[f]=inw(iobase+NE_DATA);
            if(len&0x01) {
                ((unsigned char *)buf)[len-1]=inb(iobase+NE_DATA);
                xfers++;
            }
        } else {
            for(f=0;f<len;f++)
                ((unsigned char *)buf)[f]=inb(iobase+NE_DATA);
        }
    } else {
        if(nic->wordlength==2) {
            for(f=0;f<(len>>1);f++)
                inw(iobase+NE_DATA);
            if(len&0x01) {
                inb(iobase+NE_DATA);
                xfers++;
            }
        } else {
            for(f=0;f<len;f++)
                inb(iobase+NE_DATA);
        }
    }
    /* Do these need to be *_p variants??? */

    /*	for(f=0;f<15;f++) kprintf("%X",buf[f]); kprintf("\n"); */
    /* TODO: make this timeout a constant */
    for(f=0;f<timeout;f++) {
        uint high=inb_p(iobase + REMOTESTARTADDRESS1);
        uint low=inb_p(iobase + REMOTESTARTADDRESS0);
        addr=(high<<8)+low;
        if(((offset+xfers)&0xff)==low)	break;
    }
    /*
     if(f>=timeout)
     kprintf("NE2000: Remote DMA address invalid.  expected:%x - actual:%x\n",
     offset+xfers, addr); HUH WHAT? BJS*/

    outbreverse_p(ISR_RDC, iobase + INTERRUPTSTATUS);	/* finish DMA cycle */
}

/* Now supports scatter gather */
void nic_block_output(snic *nic, packet_buffer *pkt) {
    int iobase=nic->iobase;
    int timeout=TIMEOUT_DMAMATCH;	int f;	uint addr;	int tmplen;
    int bufidx, buflen; /* current packet_data in packet_buffer */
    void *bufptr;
    char skip=0; /* Ask me about this if you have questions */
    uint deleteme=0;	/* delete when done debugging */
    outbreverse_p(NIC_DMA_DISABLE | NIC_PAGE0 | NIC_START, iobase);

    T(A);

    for(f=0;f<pkt->count;f++) deleteme+=pkt->buf[f].len;
    T(B);

    if(pkt->len != deleteme) return;

    if(pkt->len > MAX_LENGTH) return;
    /* we should not have gotten this far...  Paranoia check */

    /* this next part is to supposedly fix a "read-before-write" bug... */
    outbreverse_p(0x42, iobase + REMOTEBYTECOUNT0);
    outbreverse_p(0x00, iobase + REMOTEBYTECOUNT1);
    outbreverse_p(0x42, iobase + REMOTESTARTADDRESS0);
    outbreverse_p(0x00, iobase + REMOTESTARTADDRESS1);
    outbreverse_p(NIC_REM_READ | NIC_START, iobase);
    SLOW_DOWN_IO;	SLOW_DOWN_IO;	SLOW_DOWN_IO;

    outbreverse_p(ISR_RDC, iobase + INTERRUPTSTATUS);	/* ack remote DMA int */
    T(C);

    tmplen=(pkt->len < MIN_LENGTH) ? MIN_LENGTH : pkt->len;
    outbreverse_p(tmplen & 0xff, iobase + REMOTEBYTECOUNT0);
    outbreverse_p(tmplen >> 8, iobase + REMOTEBYTECOUNT1);
    outbreverse_p(0x00, iobase + REMOTESTARTADDRESS0);
    outbreverse_p(pkt->page, iobase + REMOTESTARTADDRESS1);
    outbreverse_p(NIC_REM_WRITE | NIC_START, iobase);

    T(D);

    /* go through each buffer and transfer it's contents */
    for(bufidx=pkt->count-1; bufidx >= 0; bufidx--) {
        char odd;
        buflen=pkt->buf[bufidx].len; /* only do derefrence once */
        bufptr=pkt->buf[bufidx].ptr; /* for performance :) */

        if(nic->wordlength==2) {

            odd=(buflen & 0x01);
            buflen>>=1;		/* half the transfers */

            if(skip) ((uint)bufptr)--;   /* korewa baka desu */

            for(f=skip;f<buflen;f++) { /* do we skip? */
                outwreverse(((unsigned short *)bufptr)[f],iobase+NE_DATA);
                tmplen -= 2;
            }

            /* output 16-bits of the last odd byte */
            skip=0;		/* assume we don't skip */

            if(odd) {
                short tmp=((unsigned char *)bufptr)[buflen<<1];
                /* get the byte from the next segment */
                if((bufidx-1)>=0) {
                    tmp |= ((unsigned char *) pkt->buf[bufidx-1].ptr)[0]  << 8;
                }
                outwreverse(tmp,iobase + NE_DATA);
                tmplen -= 2;
                skip=1;
            }
        } else
            for(f=0;f<buflen;f++) {
                outbreverse(((unsigned char *)bufptr)[f],iobase+NE_DATA);
                tmplen--;
            }
    }
    T(E);

    /* If it's too short, pad with 0s */
    if(tmplen > 0) {
        T(F);
        /*	kprintf("Padding runt\n"); */
        if(nic->wordlength==2) {
            for(f=0;f<tmplen/2;f++){
                outwreverse(0x00,iobase + NE_DATA);
                tmplen -= 2;
            }
        }
    }
    if (tmplen > 0) {
        for(f=0;f<tmplen;f++) {
            outbreverse(0x00,iobase + NE_DATA);
            tmplen--;
        }
    }


    T(G);
#ifdef SHIT
    if(nic->wordlength==2) {
        for(w=0;w<(len>>1);w++)
            outwreverse(((unsigned short *)buf)[w],iobase+NE_DATA);
        if(len & 0x01) {
            short tmp=buf[len-1]; //so we can output a whole 16-bits
            // if buf were on the end of something, we would die.
            outwreverse(tmp,iobase+NE_DATA);
        }
    } else for(f=0;f<len;f++)
        outbreverse(((unsigned char *)buf)[f],iobase+NE_DATA);

#endif
    for(f=0;f<timeout;f++) {
        uint high=inb_p(iobase + REMOTESTARTADDRESS1);
        uint low=inb_p(iobase + REMOTESTARTADDRESS0);
        addr=(high<<8)+low;
        if(( (((pkt->page<<8)+(nic->wordlength==2 && (pkt->len&0x01))?
               /*				pkt->len+1:pkt->len+2))&0xff)==low)*/
               pkt->len+0:pkt->len+1))&0xff)==low)
            break;
        if( (pkt->len < MIN_LENGTH) && ( (((pkt->page<<8)+MIN_LENGTH)
                                          & 0xff) == low) )
            break;
    }
    T(H);
#ifdef SHIT
    if(f>=timeout)
        kprintf("NE2000: Remote DMA address invalid.  expected:%x - actual:%x\n",
                ( (pkt->len >= MIN_LENGTH) ?
                  ((pkt->page<<8)+
                   (nic->wordlength==2&&(pkt->len&0x01))?
                   pkt->len+0:pkt->len+1)
                  /*					pkt->len+1:pkt->len+2)*/
                  : ((pkt->page<<8)+MIN_LENGTH) ),
                addr);
#endif
    /* TODO Check ISR_RDC */

    T(I);

    outbreverse_p(ISR_RDC, iobase + INTERRUPTSTATUS);	/* finish DMA cycle */
}

#endif




#endif // HAVE_NET

#endif
