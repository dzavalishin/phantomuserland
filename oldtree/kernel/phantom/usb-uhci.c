#if HAVE_USB

// Code for handling UHCI USB controllers.
//
// Copyright (C) 2009  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#define DEBUG_MSG_PREFIX "uhci"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <compat/seabios.h>
#include <ia32/pio.h>
#include <time.h>
#include <kernel/sem.h>

#include "usb-uhci.h" // USBLEGSUP
#include "usb.h" // struct usb_s

#include <kernel/drivers.h>


struct usb_uhci_s {
    struct usb_s usb;
    u16 iobase;
    struct uhci_qh *control_qh, *bulk_qh;
    struct uhci_framelist *framelist;
};

static void uhci_dump(struct usb_uhci_s *cntl);


/****************************************************************
 * Root hub
 ****************************************************************/

// Check if device attached to a given port
static int
uhci_hub_detect(struct usbhub_s *hub, u32 port)
{
    struct usb_uhci_s *cntl = container_of(hub->cntl, struct usb_uhci_s, usb);
    u16 ioport = cntl->iobase + USBPORTSC1 + port * 2;

    u16 status = inw(ioport);
    if (!(status & USBPORTSC_CCS))
        // No device
        return -1;

    // XXX - if just powered up, need to wait for USB_TIME_ATTDB?

    // Begin reset on port
    outw(ioport, USBPORTSC_PR);
    msleep(USB_TIME_DRSTR);
    return 0;
}

// Reset device on port
static int
uhci_hub_reset(struct usbhub_s *hub, u32 port)
{
    struct usb_uhci_s *cntl = container_of(hub->cntl, struct usb_uhci_s, usb);
    u16 ioport = cntl->iobase + USBPORTSC1 + port * 2;

    // Finish reset on port
    outw(ioport, 0);
    //udelay(6); // 64 high-speed bit times
    tenmicrosec();
    u16 status = inw(ioport);
    if (!(status & USBPORTSC_CCS))
        // No longer connected
        return -1;
    outw(ioport, USBPORTSC_PE);
    return !!(status & USBPORTSC_LSDA);
}

// Disable port
static void
uhci_hub_disconnect(struct usbhub_s *hub, u32 port)
{
    struct usb_uhci_s *cntl = container_of(hub->cntl, struct usb_uhci_s, usb);
    u16 ioport = cntl->iobase + USBPORTSC1 + port * 2;
    outw(ioport, 0);
}

static struct usbhub_op_s uhci_HubOp = {
    .detect = uhci_hub_detect,
    .reset = uhci_hub_reset,
    .disconnect = uhci_hub_disconnect,
};

// Find any devices connected to the root hub.
static int
check_uhci_ports(struct usb_uhci_s *cntl)
{
    SHOW_FLOW0( 2, "Look for UHCI ports" );
    ASSERT32FLAT();
    struct usbhub_s hub;
    memset(&hub, 0, sizeof(hub));
    hub.cntl = &cntl->usb;
    hub.portcount = 2;
    hub.op = &uhci_HubOp;
    usb_enumerate(&hub);
    return hub.devcount;
}


/****************************************************************
 * Setup
 ****************************************************************/

static int uhci_irq = -1;
static hal_sem_t intr_sem;
struct usb_uhci_s *static_cntl;

static void uhci_int_enable( int on )
{
    if( on )
        outw( static_cntl->iobase + USBINTR, USBINTR_TIMEOUT|USBINTR_RESUME|USBINTR_IOC|USBINTR_SP );
    else
        outw( static_cntl->iobase + USBINTR, 0);
}


static void
reset_uhci(struct usb_uhci_s *cntl, u16 bdf)
{
    // XXX - don't reset if not needed.

    // Reset PIRQ and SMI
    pci_config_writew(bdf, USBLEGSUP, USBLEGSUP_RWC);

    // Reset the HC
    outw(cntl->iobase + USBCMD, USBCMD_HCRESET);
    //udelay(5);
    tenmicrosec();

    // Disable interrupts and commands (just to be safe).
    outw(cntl->iobase + USBINTR, 0);
    outw(cntl->iobase + USBCMD, 0);
#if UHCI_INTERRUPT
#endif
}

static void
configure_uhci(void *data)
{
    struct usb_uhci_s *cntl = data;

    //struct uhci_framelist *fl = memalign_high(sizeof(*fl), sizeof(*fl));
    void *fl_b = malloc(sizeof(struct uhci_framelist)+4096);
    struct uhci_framelist *fl = (void *) ( (((addr_t)fl_b)+4096) & ~(4096-1) );

    SHOW_FLOW( 8, "framelist %p", fl );

    // Allocate ram for schedule storage
    addr_t term_td_raw = (addr_t)malloc_high(0x10+sizeof(struct uhci_td));
    addr_t intr_qh_raw = (addr_t)malloc_high(0x10+sizeof(struct uhci_td));
    addr_t term_qh_raw = (addr_t)malloc_high(0x10+sizeof(struct uhci_td));
    if (!term_td_raw || !fl_b || !intr_qh_raw || !term_qh_raw) {
        warn_noalloc();
        goto fail;
    }

    struct uhci_td *term_td = (void*)((0x10+term_td_raw) & ~0xF);
    struct uhci_qh *intr_qh = (void*)((0x10+intr_qh_raw) & ~0xF);
    struct uhci_qh *term_qh = (void*)((0x10+term_qh_raw) & ~0xF);

    // Work around for PIIX errata
    memset(term_td, 0, sizeof(*term_td));
    term_td->link = UHCI_PTR_TERM;
    term_td->token = (uhci_explen(0) | (0x7f << TD_TOKEN_DEVADDR_SHIFT)
                      | USB_PID_IN);
    memset(term_qh, 0, sizeof(*term_qh));
    term_qh->element = (u32)term_td;
    term_qh->link = UHCI_PTR_TERM;

    // Set schedule to point to primary intr queue head
    memset(intr_qh, 0, sizeof(*intr_qh));
    intr_qh->element = UHCI_PTR_TERM;
    intr_qh->link = (u32)term_qh | UHCI_PTR_QH;
    unsigned int i;
    for (i=0; i<ARRAY_SIZE(fl->links); i++)
        fl->links[i] = (u32)intr_qh | UHCI_PTR_QH;
    cntl->framelist = fl;
    cntl->control_qh = cntl->bulk_qh = intr_qh;
    barrier();

    // Set the frame length to the default: 1 ms exactly
    outb(cntl->iobase + USBSOF, USBSOF_DEFAULT);

    // Store the frame list base address
    outl(cntl->iobase + USBFLBASEADD, (u32)fl->links );

    // Set the current frame number
    outw(cntl->iobase + USBFRNUM, 0);

    // Mark as configured and running with a 64-byte max packet.
    outw(cntl->iobase + USBCMD, USBCMD_RS | USBCMD_CF | USBCMD_MAXP);

    // Find devices
    int count = check_uhci_ports(cntl);
    free_pipe(cntl->usb.defaultpipe);
    if (count)
        // Success
        return;

    // No devices found - shutdown and free controller.
    outw(cntl->iobase + USBCMD, 0);
fail:
    free((void *)term_td_raw);
    free(fl_b);
    free((void *)intr_qh_raw);
    free((void *)term_qh_raw);
    free(cntl);
}

void
uhci_init(u16 bdf, int busid)
{
    if (! CONFIG_USB_UHCI)        return;

    assert( 0 == hal_sem_init( &intr_sem, "uhci intr" ) );

    struct usb_uhci_s *cntl = malloc_tmphigh(sizeof(*cntl));
    memset(cntl, 0, sizeof(*cntl));
    usb_init_usb_s( &(cntl->usb) );
    cntl->usb.busid = busid;
    cntl->usb.bdf = bdf;
    cntl->usb.type = USB_TYPE_UHCI;
    cntl->iobase = (pci_config_readl(bdf, PCI_BASE_ADDRESS_4)
                    & PCI_BASE_ADDRESS_IO_MASK);

    SHOW_INFO(1, "UHCI init on dev %02x:%02x.%x (io=%x)"
            , pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf)
            , pci_bdf_to_fn(bdf), cntl->iobase);

    pci_config_maskw(bdf, PCI_COMMAND, 0, PCI_COMMAND_MASTER);

    reset_uhci(cntl, bdf);

    //n_thread(configure_uhci, cntl);
    configure_uhci( cntl );

    static_cntl = cntl;
}

static void uhci_int(void *arg)
{
    (void) arg;
    uhci_int_enable( 0 );
    hal_sem_release( &intr_sem );
}

phantom_device_t * driver_uhci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    uhci_irq = pci->interrupt;
    SHOW_INFO( 2, "irq = %d", uhci_irq );

    //assert( 0 == hal_sem_init( &intr_sem, "uhci intr" ) );

    if( hal_irq_alloc( uhci_irq, &uhci_int, 0, HAL_IRQ_SHAREABLE ) )
    {

        SHOW_ERROR( 0, "IRQ %d is busy", uhci_irq );

        return 0;
    }

    return 0; // TODO BUG FIXME redo all the init process, create dev
}



/****************************************************************
 * End point communication
 ****************************************************************/

static int
wait_qh(struct usb_uhci_s *cntl, volatile struct uhci_qh *qh)
{
    //SHOW_FLOW( 5, "count = %d", count );
    // XXX - 500ms just a guess
    //u64 end = calc_future_tsc(500);
    u64 end = calc_future_tsc(5000);
    uhci_dump(cntl);
    for (;;) {
        //printf("!%x ", qh->element);
        if (qh->element & UHCI_PTR_TERM)
            return 0;
        if (check_tsc(end)) {
            warn_timeout();
            struct uhci_td *td = (void*)(qh->element & ~UHCI_PTR_BITS);
            dprintf(1, "Timeout on wait_qh %p (td=%p s=%x c=%x/%x)"
                    , qh, td, td->status
                    , inw(cntl->iobase + USBCMD)
                    , inw(cntl->iobase + USBSTS));
            uhci_dump(cntl);
            return -1;
        }
        yield();
    }
}

// Wait for next USB frame to start - for ensuring safe memory release.
static void
uhci_waittick(struct usb_uhci_s *cntl)
{
    u16 iobase = cntl->iobase;

    SHOW_FLOW0( 6, "enter" );
    uhci_dump(cntl);

    barrier();
    u16 startframe = inw(iobase + USBFRNUM);
    u64 end = calc_future_tsc(1000 * 5);
    for (;;) {
        u16 nowframe = inw(iobase + USBFRNUM);
        if(nowframe != startframe)
            break;
        if (check_tsc(end)) {
            warn_timeout();

            uhci_dump(cntl);

            return;
        }
        yield();
    }
    SHOW_FLOW0( 6, "leave ok" );
}

struct uhci_pipe {
    struct uhci_qh qh;
    struct uhci_td *next_td;
    struct usb_pipe pipe;
    u16 iobase;
    u8 toggle;
};

void
uhci_free_pipe(struct usb_pipe *p)
{
    if (! CONFIG_USB_UHCI)
        return;
    dprintf(7, "uhci_free_pipe %p", p);
    struct uhci_pipe *pipe = container_of(p, struct uhci_pipe, pipe);
    struct usb_uhci_s *cntl = container_of(
        pipe->pipe.cntl, struct usb_uhci_s, usb);

    struct uhci_qh *pos = (void*)(cntl->framelist->links[0] & ~UHCI_PTR_BITS);
    for (;;) {
        u32 link = pos->link;
        if (link == UHCI_PTR_TERM) {
            // Not found?!  Exit without freeing.
            warn_internalerror();
            return;
        }
        struct uhci_qh *next = (void*)(link & ~UHCI_PTR_BITS);
        if (next == &pipe->qh) {
            pos->link = next->link;
            if (cntl->control_qh == next)
                cntl->control_qh = pos;
            if (cntl->bulk_qh == next)
                cntl->bulk_qh = pos;
            uhci_waittick(cntl);
            free(pipe);
            return;
        }
        pos = next;
    }
}

struct usb_pipe *
uhci_alloc_control_pipe(struct usb_pipe *dummy)
{
    if (! CONFIG_USB_UHCI)
        return NULL;
    struct usb_uhci_s *cntl = container_of(
        dummy->cntl, struct usb_uhci_s, usb);
    dprintf(7, "uhci_alloc_control_pipe usb %p", &cntl->usb);

    // Allocate a queue head.
    struct uhci_pipe *pipe = malloc_tmphigh(sizeof(*pipe));
    if (!pipe) {
        warn_noalloc();
        return NULL;
    }

    dprintf(7, "uhci_alloc_control_pipe pipe %p", pipe);

    memset(pipe, 0, sizeof(*pipe));
    memcpy(&pipe->pipe, dummy, sizeof(pipe->pipe));
    pipe->qh.element = UHCI_PTR_TERM;
    pipe->iobase = cntl->iobase;

    // Add queue head to controller list.
    struct uhci_qh *control_qh = cntl->control_qh;
    pipe->qh.link = control_qh->link;
    barrier();
    control_qh->link = (u32)&pipe->qh | UHCI_PTR_QH;
    if (cntl->bulk_qh == control_qh)
        cntl->bulk_qh = &pipe->qh;
    return &pipe->pipe;
}

int
uhci_control(struct usb_pipe *p, int dir, const void *cmd, int cmdsize
             , void *data, int datasize)
{
    ASSERT32FLAT();
    if (! CONFIG_USB_UHCI)
        return -1;
    dprintf(5, "uhci_control %p", p);
    struct uhci_pipe *pipe = container_of(p, struct uhci_pipe, pipe);
    struct usb_uhci_s *cntl = container_of(
        pipe->pipe.cntl, struct usb_uhci_s, usb);

    int maxpacket = pipe->pipe.maxpacket;
    int lowspeed = pipe->pipe.speed;
    int devaddr = pipe->pipe.devaddr | (pipe->pipe.ep << 7);

    // Setup transfer descriptors
    int count = 2 + DIV_ROUND_UP(datasize, maxpacket);

    // Poor man's memalign
    addr_t tds_raw = (addr_t)malloc_tmphigh(0x10 + sizeof(struct uhci_td) * count);
    if (!tds_raw) {
        warn_noalloc();
        return -1;
    }

    struct uhci_td *tds = (void *)((0x10+tds_raw) & ~0xF);

    SHOW_FLOW( 5, "count = %d, datasize = %d, maxpacket = %d", count, datasize, maxpacket );

    tds[0].link = (u32)&tds[1] | UHCI_PTR_DEPTH;
    tds[0].status = (uhci_maxerr(3) | (lowspeed ? TD_CTRL_LS : 0)
                     | TD_CTRL_ACTIVE);
    tds[0].token = (uhci_explen(cmdsize) | (devaddr << TD_TOKEN_DEVADDR_SHIFT)
                    | USB_PID_SETUP);
    tds[0].buffer = (void*)cmd;
    int toggle = TD_TOKEN_TOGGLE;
    int i;
    for (i=1; i<count-1; i++) {
        tds[i].link = (u32)&tds[i+1] | UHCI_PTR_DEPTH;
        tds[i].status = (uhci_maxerr(3) | (lowspeed ? TD_CTRL_LS : 0)
                         | TD_CTRL_ACTIVE);
        int len = (i == count-2 ? (datasize - (i-1)*maxpacket) : maxpacket);
        tds[i].token = (uhci_explen(len) | toggle
                        | (devaddr << TD_TOKEN_DEVADDR_SHIFT)
                        | (dir ? USB_PID_IN : USB_PID_OUT));
        tds[i].buffer = data + (i-1) * maxpacket;
        toggle ^= TD_TOKEN_TOGGLE;
    }
    tds[i].link = UHCI_PTR_TERM;
    tds[i].status = (uhci_maxerr(0) | (lowspeed ? TD_CTRL_LS : 0)
                     | TD_CTRL_ACTIVE);
    tds[i].token = (uhci_explen(0) | TD_TOKEN_TOGGLE
                    | (devaddr << TD_TOKEN_DEVADDR_SHIFT)
                    | (dir ? USB_PID_OUT : USB_PID_IN));
    tds[i].buffer = 0;

    // Transfer data
    barrier();
    pipe->qh.element = (u32)&tds[0];
    int ret = wait_qh(cntl, &pipe->qh);
    if (ret) {
        pipe->qh.element = UHCI_PTR_TERM;
        uhci_waittick(cntl);
    }
    free((void *)tds_raw);
    return ret;
}

struct usb_pipe *
uhci_alloc_bulk_pipe(struct usb_pipe *dummy)
{
    if (! CONFIG_USB_UHCI)
        return NULL;
    struct usb_uhci_s *cntl = container_of(
        dummy->cntl, struct usb_uhci_s, usb);
    dprintf(7, "uhci_alloc_bulk_pipe %p", &cntl->usb);

    // Allocate a queue head.
    struct uhci_pipe *pipe = malloc_low(sizeof(*pipe));
    if (!pipe) {
        warn_noalloc();
        return NULL;
    }
    memset(pipe, 0, sizeof(*pipe));
    memcpy(&pipe->pipe, dummy, sizeof(pipe->pipe));
    pipe->qh.element = UHCI_PTR_TERM;
    pipe->iobase = cntl->iobase;

    // Add queue head to controller list.
    struct uhci_qh *bulk_qh = cntl->bulk_qh;
    pipe->qh.link = bulk_qh->link;
    barrier();
    bulk_qh->link = (u32)&pipe->qh | UHCI_PTR_QH;

    return &pipe->pipe;
}

static int
wait_td(struct uhci_td *td)
{
    u64 end = calc_future_tsc(5000); // XXX - lookup real time.
    u32 status;
    for (;;) {
        status = td->status;
        if (!(status & TD_CTRL_ACTIVE))
            break;
        if (check_tsc(end)) {
            warn_timeout();
            return -1;
        }
#if UHCI_INTERRUPT
        hal_sem_zero( &intr_sem );
        uhci_int_enable( 1 );
        hal_sem_acquire_etc( &intr_sem, 1, SEM_FLAG_TIMEOUT, 5000 );
#else
        yield();
#endif
    }

    if (status & TD_CTRL_ANY_ERROR) {
        dprintf(1, "wait_td error - status=%x", status);
        return -2;
    }

    return 0;
}

#define STACKTDS 4
#define TDALIGN 16

int
uhci_send_bulk(struct usb_pipe *p, int dir, void *data, int datasize)
{
    if (! CONFIG_USB_UHCI)
        return -1;
    struct uhci_pipe *pipe = container_of(p, struct uhci_pipe, pipe);
    dprintf(7, "uhci_send_bulk qh=%p dir=%d data=%p size=%d"
            , &pipe->qh, dir, data, datasize);
    int maxpacket = GET_FLATPTR(pipe->pipe.maxpacket);
    int lowspeed = GET_FLATPTR(pipe->pipe.speed);
    int devaddr = (GET_FLATPTR(pipe->pipe.devaddr)
                   | (GET_FLATPTR(pipe->pipe.ep) << 7));
    int toggle = GET_FLATPTR(pipe->toggle) ? TD_TOKEN_TOGGLE : 0;

    // Allocate 4 tds on stack (16byte aligned)
    u8 tdsbuf[sizeof(struct uhci_td) * STACKTDS + TDALIGN - 1];
    struct uhci_td *tds = (void*)ALIGN((u32)tdsbuf, TDALIGN);
    memset(tds, 0, sizeof(*tds) * STACKTDS);

    // Enable tds
    barrier();
    SET_FLATPTR(pipe->qh.element, (u32)MAKE_FLATPTR(GET_SEG(SS), tds));

    int tdpos = 0;
    while (datasize) {
        struct uhci_td *td = &tds[tdpos++ % STACKTDS];
        int ret = wait_td(td);
        if (ret)
            goto fail;

        int transfer = datasize;
        if (transfer > maxpacket)
            transfer = maxpacket;
        struct uhci_td *nexttd_fl = MAKE_FLATPTR(GET_SEG(SS)
                                                 , &tds[tdpos % STACKTDS]);
        td->link = (transfer==datasize ? UHCI_PTR_TERM : (u32)nexttd_fl);
        td->token = (uhci_explen(transfer) | toggle
                     | (devaddr << TD_TOKEN_DEVADDR_SHIFT)
                     | (dir ? USB_PID_IN : USB_PID_OUT));
        td->buffer = data;
        barrier();
        td->status = (uhci_maxerr(3) | (lowspeed ? TD_CTRL_LS : 0)
                      | TD_CTRL_ACTIVE);
        toggle ^= TD_TOKEN_TOGGLE;

        data += transfer;
        datasize -= transfer;
    }
    int i;
    for (i=0; i<STACKTDS; i++) {
        struct uhci_td *td = &tds[tdpos++ % STACKTDS];
        int ret = wait_td(td);
        if (ret)
            goto fail;
    }

    SET_FLATPTR(pipe->toggle, !!toggle);
    return 0;
fail:
    SHOW_FLOW0(1, "uhci_send_bulk failed");
    SET_FLATPTR(pipe->qh.element, UHCI_PTR_TERM);
    struct usb_uhci_s *cntl = container_of(
        p->cntl, struct usb_uhci_s, usb);
    uhci_waittick(GET_FLATPTR(cntl));
    return -1;
}

struct usb_pipe *
uhci_alloc_intr_pipe(struct usb_pipe *dummy, int frameexp)
{
    if (! CONFIG_USB_UHCI)
        return NULL;
    struct usb_uhci_s *cntl = container_of(
        dummy->cntl, struct usb_uhci_s, usb);
    dprintf(7, "uhci_alloc_intr_pipe %p %d", &cntl->usb, frameexp);

    if (frameexp > 10)
        frameexp = 10;
    int maxpacket = dummy->maxpacket;
    int lowspeed = dummy->speed;
    int devaddr = dummy->devaddr | (dummy->ep << 7);
    // Determine number of entries needed for 2 timer ticks.
    int ms = 1<<frameexp;

    //unsigned int count = DIV_ROUND_UP(PIT_TICK_INTERVAL * 1000 * 2, PIT_TICK_RATE * ms);
    unsigned int count = usb_intr_pipe_count(ms);

    count = ALIGN(count, 2);
    struct uhci_pipe *pipe = malloc_low(sizeof(*pipe));
    struct uhci_td *tds = malloc_low(sizeof(*tds) * count);
    void *data = malloc_low(maxpacket * count);
    if (!pipe || !tds || !data) {
        warn_noalloc();
        goto fail;
    }
    memset(pipe, 0, sizeof(*pipe));
    memcpy(&pipe->pipe, dummy, sizeof(pipe->pipe));
    pipe->qh.element = (u32)tds;
    pipe->next_td = &tds[0];
    pipe->iobase = cntl->iobase;

    int toggle = 0;
    unsigned int i;
    for (i=0; i<count; i++) {
        tds[i].link = (i==count-1 ? (u32)&tds[0] : (u32)&tds[i+1]);
        tds[i].status = (uhci_maxerr(3) | (lowspeed ? TD_CTRL_LS : 0)
                         | TD_CTRL_ACTIVE);
        tds[i].token = (uhci_explen(maxpacket) | toggle
                        | (devaddr << TD_TOKEN_DEVADDR_SHIFT)
                        | USB_PID_IN);
        tds[i].buffer = data + maxpacket * i;
        toggle ^= TD_TOKEN_TOGGLE;
    }

    // Add to interrupt schedule.
    struct uhci_framelist *fl = cntl->framelist;
    if (frameexp == 0) {
        // Add to existing interrupt entry.
        struct uhci_qh *intr_qh = (void*)(fl->links[0] & ~UHCI_PTR_BITS);
        pipe->qh.link = intr_qh->link;
        barrier();
        intr_qh->link = (u32)&pipe->qh | UHCI_PTR_QH;
        if (cntl->control_qh == intr_qh)
            cntl->control_qh = &pipe->qh;
        if (cntl->bulk_qh == intr_qh)
            cntl->bulk_qh = &pipe->qh;
    } else {
        int startpos = 1<<(frameexp-1);
        pipe->qh.link = fl->links[startpos];
        barrier();
        for (i=startpos; i<ARRAY_SIZE(fl->links); i+=ms)
            fl->links[i] = (u32)&pipe->qh | UHCI_PTR_QH;
    }

    return &pipe->pipe;
fail:
    free(pipe);
    free(tds);
    free(data);
    return NULL;
}

int
uhci_poll_intr(struct usb_pipe *p, void *data)
{
    //ASSERT16();
    if (! CONFIG_USB_UHCI)
        return -1;

    struct uhci_pipe *pipe = container_of(p, struct uhci_pipe, pipe);
    struct uhci_td *td = GET_FLATPTR(pipe->next_td);
    u32 status = GET_FLATPTR(td->status);
    u32 token = GET_FLATPTR(td->token);
    if (status & TD_CTRL_ACTIVE)
        // No intrs found.
        return -1;
    // XXX - check for errors.

    // Copy data.
    void *tddata = GET_FLATPTR(td->buffer);

    memcpy_far(GET_SEG(SS), data
               , FLATPTR_TO_SEG(tddata), (void*)FLATPTR_TO_OFFSET(tddata)
               , uhci_expected_length(token));

    // Reenable this td.
    struct uhci_td *next = (void*)(GET_FLATPTR(td->link) & ~UHCI_PTR_BITS);
    SET_FLATPTR(pipe->next_td, next);
    barrier();
    SET_FLATPTR(td->status, (uhci_maxerr(0) | (status & TD_CTRL_LS)
                             | TD_CTRL_ACTIVE));

    return 0;
}

static void uhci_dump(struct usb_uhci_s *cntl)
{
    u32 membase = inl(cntl->iobase + USBFLBASEADD);
    u16 startframe = inw(cntl->iobase + USBFRNUM);
    u8 sof = inb(cntl->iobase + USBSOF);
    SHOW_INFO( 1, "iobase %x, membase %x, frame %d, SOF %d", cntl->iobase, membase, startframe, sof );

    {
    u16 cmd = inw(cntl->iobase + USBCMD);
    SHOW_INFO( 1, "cmd %b", cmd, "\020\1Run\2Reset\3GReset\4GSuspend\5FrcGResume\6SWDebug\7Conf\10MaxPkt64" );
    }
    {
    u16 sts = inw(cntl->iobase + USBSTS);
    SHOW_INFO( 1, "sts %b", sts, "\020\1IntrIOC\2IntrErr\3ResumeDetect\4PciErr\5SchedErr\6Halt" );
    }
    {
    u16 intr = inw(cntl->iobase + USBINTR);
    SHOW_INFO( 1, "irq %b", intr, "\020\1Timeout\2Resume\3IOComplete\4ShortPkt" );
    }

    //uhci_framelist *fl = cntl->framelist;


    struct uhci_qh *pos = (void*)(cntl->framelist->links[0] & ~UHCI_PTR_BITS);
    for (;;)
    {
        SHOW_INFO( 1, "td %x link %x elem %x", pos, pos->link, pos->element );
        u32 link = pos->link;

        if (link == UHCI_PTR_TERM)
            break;

        struct uhci_qh *next = (void*)(link & ~UHCI_PTR_BITS);
        pos = next;
    }

}



#endif // HAVE_USB
