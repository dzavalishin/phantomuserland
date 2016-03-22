#if HAVE_USB

// Code for handling OHCI USB controllers.
//
// Copyright (C) 2009  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#define DEBUG_MSG_PREFIX "ohci"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <compat/seabios.h>

#include "usb-ohci.h" // struct ohci_hcca
#include "usb.h" // struct usb_s

#include <kernel/drivers.h>


#define FIT                     (1 << 31)

struct usb_ohci_s {
    struct usb_s usb;
    struct ohci_regs *regs;
};

static void dump_ohci_ed( struct ohci_ed *ed );
static void ohci_wait_intr(void);


/****************************************************************
 * Root hub
 ****************************************************************/

// Check if device attached to port
static int
ohci_hub_detect(struct usbhub_s *hub, u32 port)
{
    struct usb_ohci_s *cntl = container_of(hub->cntl, struct usb_ohci_s, usb);
    u32 sts = readl(&cntl->regs->roothub_portstatus[port]);
    if (!(sts & RH_PS_CCS))
        // No device.
        return -1;

    // XXX - need to wait for USB_TIME_ATTDB if just powered up?
    SHOW_FLOW( 0, "have dev on hub port %d", port );

    return 0;
}

// Disable port
static void
ohci_hub_disconnect(struct usbhub_s *hub, u32 port)
{
    struct usb_ohci_s *cntl = container_of(hub->cntl, struct usb_ohci_s, usb);
    writel(&cntl->regs->roothub_portstatus[port], RH_PS_CCS|RH_PS_LSDA);
}

// Reset device on port
static int
ohci_hub_reset(struct usbhub_s *hub, u32 port)
{
    struct usb_ohci_s *cntl = container_of(hub->cntl, struct usb_ohci_s, usb);
    writel(&cntl->regs->roothub_portstatus[port], RH_PS_PRS);
    u32 sts;
    u64 end = calc_future_tsc(USB_TIME_DRSTR * 2);
    for (;;) {
        sts = readl(&cntl->regs->roothub_portstatus[port]);
        if (!(sts & RH_PS_PRS))
            // XXX - need to ensure USB_TIME_DRSTR time in reset?
            break;
        if (check_tsc(end)) {
            // Timeout.
            warn_timeout();
            ohci_hub_disconnect(hub, port);
            return -1;
        }
#if OHCI_INTERRUPT
        ohci_wait_intr();
#else
        yield();
#endif
    }

    if ((sts & (RH_PS_CCS|RH_PS_PES)) != (RH_PS_CCS|RH_PS_PES))
        // Device no longer present
        return -1;

    return !!(sts & RH_PS_LSDA);
}

static struct usbhub_op_s ohci_HubOp = {
    .detect = ohci_hub_detect,
    .reset = ohci_hub_reset,
    .disconnect = ohci_hub_disconnect,
};

// Find any devices connected to the root hub.
static int
check_ohci_ports(struct usb_ohci_s *cntl)
{
    SHOW_FLOW( 0, "%p", cntl );

    ASSERT32FLAT();
    // Turn on power for all devices on roothub.
    u32 rha = readl(&cntl->regs->roothub_a);
    rha &= ~(RH_A_PSM | RH_A_OCPM);
    writel(&cntl->regs->roothub_status, RH_HS_LPSC);
    writel(&cntl->regs->roothub_b, RH_B_PPCM);
    msleep((rha >> 24) * 2);
    // XXX - need to sleep for USB_TIME_SIGATT if just powered up?

    struct usbhub_s hub;
    memset(&hub, 0, sizeof(hub));
    hub.cntl = &cntl->usb;
    hub.portcount = rha & RH_A_NDP;
    hub.op = &ohci_HubOp;
    SHOW_FLOW( 0, "will enumerate, ctl %p", cntl );
    usb_enumerate(&hub);
    return hub.devcount;
}


/****************************************************************
 * Setup
 ****************************************************************/

static int ohci_irq = -1;
static hal_sem_t ohci_intr_sem;
struct usb_ohci_s *static_cntl;

static void ohci_int_enable( int on )
{
    if( on )
    {
        writel(&static_cntl->regs->intrenable, ~0);
        //writel(&cntl->regs->intrstatus, ~0);
    }
    else
    {
        writel(&static_cntl->regs->intrdisable, ~0);
        writel(&static_cntl->regs->intrstatus, ~0);
    }
}


static void ohci_wait_intr(void)
{
    hal_sem_zero( &ohci_intr_sem );
    ohci_int_enable( 1 );
    hal_sem_acquire_etc( &ohci_intr_sem, 1, SEM_FLAG_TIMEOUT, 5000 );
}


static int
start_ohci(struct usb_ohci_s *cntl, struct ohci_hcca *hcca)
{
    SHOW_FLOW( 0, "ctl %p", cntl );

    u32 oldfminterval = readl(&cntl->regs->fminterval);
    u32 oldrwc = readl(&cntl->regs->control) & OHCI_CTRL_RWC;

    // XXX - check if already running?

    // Do reset
    writel(&cntl->regs->control, OHCI_USB_RESET | oldrwc);
    readl(&cntl->regs->control); // flush writes
    msleep(USB_TIME_DRSTR);

    // Do software init (min 10us, max 2ms)
    u64 end = calc_future_tsc_usec(10);
    writel(&cntl->regs->cmdstatus, OHCI_HCR);
    for (;;) {
        u32 status = readl(&cntl->regs->cmdstatus);
        if (! status & OHCI_HCR)
            break;
        if (check_tsc(end)) {
            warn_timeout();
            return -1;
        }
    }

    // Init memory
    writel(&cntl->regs->ed_controlhead, 0);
    writel(&cntl->regs->ed_bulkhead, 0);
    writel(&cntl->regs->hcca, (u32)hcca);

    // Init fminterval
    u32 fi = oldfminterval & 0x3fff;
    writel(&cntl->regs->fminterval
           , (((oldfminterval & FIT) ^ FIT)
              | fi | (((6 * (fi - 210)) / 7) << 16)));
    writel(&cntl->regs->periodicstart, ((9 * fi) / 10) & 0x3fff);
    readl(&cntl->regs->control); // flush writes

    // XXX - verify that fminterval was setup correctly.

    // Go into operational state
    writel(&cntl->regs->control
           , (OHCI_CTRL_CBSR | OHCI_CTRL_CLE | OHCI_CTRL_PLE
              | OHCI_USB_OPER | oldrwc));
    readl(&cntl->regs->control); // flush writes

    SHOW_FLOW( 0, "done, cntl %p", cntl );

    return 0;
}

static void
stop_ohci(struct usb_ohci_s *cntl)
{
    u32 oldrwc = readl(&cntl->regs->control) & OHCI_CTRL_RWC;
    writel(&cntl->regs->control, oldrwc);
    readl(&cntl->regs->control); // flush writes
}

static void
configure_ohci(void *data)
{
    struct usb_ohci_s *cntl = data;

    SHOW_FLOW( 0, "config OHCI %p", data );

    // Allocate memory
    //struct ohci_hcca *hcca = memalign_high(256, sizeof(*hcca));

    // poor man's memalign
    void *hcca_b = malloc( 256 + sizeof(struct ohci_hcca) );
    struct ohci_hcca *hcca = (void *) ( ((addr_t)hcca_b) & (~0xFF) );

    struct ohci_ed *intr_ed = malloc_high(sizeof(*intr_ed));
    if (!hcca || !intr_ed) {
        warn_noalloc();
        goto free;
    }
    memset(hcca, 0, sizeof(*hcca));
    memset(intr_ed, 0, sizeof(*intr_ed));
    intr_ed->hwINFO = ED_SKIP;
    unsigned int i;
    for (i=0; i<ARRAY_SIZE(hcca->int_table); i++)
        hcca->int_table[i] = (u32)intr_ed;

    int ret = start_ohci(cntl, hcca);
    if (ret)
        goto err;

    int count = check_ohci_ports(cntl);
    free_pipe(cntl->usb.defaultpipe);
    if (! count)
        goto err;
    return;

err:
    stop_ohci(cntl);
free:
    free(hcca_b);
    free(intr_ed);
}

void
ohci_init(u16 bdf, int busid)
{
    if (! CONFIG_USB_OHCI)
        return;
    struct usb_ohci_s *cntl = malloc_tmphigh(sizeof(*cntl));
    memset(cntl, 0, sizeof(*cntl));
    usb_init_usb_s( &(cntl->usb) );
    cntl->usb.busid = busid;
    cntl->usb.bdf = bdf;
    cntl->usb.type = USB_TYPE_OHCI;

    u32 baseaddr = pci_config_readl(bdf, PCI_BASE_ADDRESS_0);
    baseaddr &= PCI_BASE_ADDRESS_MEM_MASK;


    void *mapped;
    hal_alloc_vaddress( &mapped, 1);
    hal_page_control( baseaddr, mapped, page_map_io, page_rw );

    cntl->regs = mapped;

    SHOW_FLOW(1, "OHCI init on dev %02x:%02x.%x (regs=%p)"
            , pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf)
            , pci_bdf_to_fn(bdf), cntl->regs);

    // Enable bus mastering and memory access.
    pci_config_maskw(bdf, PCI_COMMAND
                     , 0, PCI_COMMAND_MASTER|PCI_COMMAND_MEMORY);

    // XXX - check for and disable SMM control?

    // Disable interrupts
    writel(&cntl->regs->intrdisable, ~0);
    writel(&cntl->regs->intrstatus, ~0);

    static_cntl = cntl;
    assert( 0 == hal_sem_init( &ohci_intr_sem, "ohci intr" ) );

    //run_thread(configure_ohci, cntl);
    configure_ohci( cntl );
}

static void ohci_int(void *arg)
{
    (void) arg;
    ohci_int_enable( 0 );
    hal_sem_release( &ohci_intr_sem );
}

phantom_device_t * driver_ohci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;

    if( (pci->sub_class != OHCI_SUB_CLASS) || (pci->interface != OHCI_INTERFACE) )
        return 0;

    ohci_irq = pci->interrupt;
    SHOW_INFO( 2, "irq = %d", ohci_irq );

    //assert( 0 == hal_sem_init( &ohci_intr_sem, "ohci intr" ) );

    if( hal_irq_alloc( ohci_irq, &ohci_int, 0, HAL_IRQ_SHAREABLE ) )
    {

        SHOW_ERROR( 0, "IRQ %d is busy", ohci_irq );

        return 0;
    }

    return 0; // TODO BUG FIXME redo all the init process, create dev
}


/****************************************************************
 * End point communication
 ****************************************************************/

static int
wait_ed(struct ohci_ed *ed)
{
    // XXX - 500ms just a guess
    u64 end = calc_future_tsc(500);
    //u64 end = calc_future_tsc(5000);
    for (;;) {
        //if (ed->hwHeadP == ed->hwTailP) // QEMU increments it past tail
        if (ed->hwHeadP >= ed->hwTailP)
            return 0;

        SHOW_FLOW0( 9, "again" );

        if (check_tsc(end)) {
            warn_timeout();
            return -1;
        }
#if OHCI_INTERRUPT
        ohci_wait_intr();
#else
        yield();
#endif
    }
}

// Wait for next USB frame to start - for ensuring safe memory release.
static void
ohci_waittick(struct usb_ohci_s *cntl)
{
    barrier();
    struct ohci_hcca *hcca = (void*)cntl->regs->hcca;
    u32 startframe = hcca->frame_no;
    u64 end = calc_future_tsc(1000 * 5);
    for (;;) {
        if (hcca->frame_no != startframe)
            break;
        if (check_tsc(end)) {
            warn_timeout();
            return;
        }
#if OHCI_INTERRUPT
        ohci_wait_intr();
#else
        yield();
#endif
    }
}

static void
signal_freelist(struct usb_ohci_s *cntl)
{
    u32 v = readl(&cntl->regs->control);
    if (v & OHCI_CTRL_CLE) {
        writel(&cntl->regs->control, v & ~(OHCI_CTRL_CLE|OHCI_CTRL_BLE));
        ohci_waittick(cntl);
        writel(&cntl->regs->ed_controlcurrent, 0);
        writel(&cntl->regs->ed_bulkcurrent, 0);
        writel(&cntl->regs->control, v);
    } else {
        ohci_waittick(cntl);
    }
}

struct ohci_pipe {
    struct ohci_ed ed;
    struct usb_pipe pipe;
    void *data;
    int count;
    struct ohci_td *tds;
};

void
ohci_free_pipe(struct usb_pipe *p)
{
    if (! CONFIG_USB_OHCI)
        return;
    SHOW_FLOW(7, "ohci_free_pipe %p", p);
    struct ohci_pipe *pipe = container_of(p, struct ohci_pipe, pipe);
    struct usb_ohci_s *cntl = container_of(
        pipe->pipe.cntl, struct usb_ohci_s, usb);

    u32 *pos = &cntl->regs->ed_controlhead;
    for (;;) {
        struct ohci_ed *next = (void*)*pos;
        if (!next) {
            // Not found?!  Exit without freeing.
            warn_internalerror();
            return;
        }
        if (next == &pipe->ed) {
            *pos = next->hwNextED;
            signal_freelist(cntl);
            free(pipe);
            return;
        }
        pos = &next->hwNextED;
    }
}

struct usb_pipe *
ohci_alloc_control_pipe(struct usb_pipe *dummy)
{
    if (! CONFIG_USB_OHCI)
        return NULL;
    struct usb_ohci_s *cntl = container_of(
        dummy->cntl, struct usb_ohci_s, usb);
    SHOW_FLOW(7, "ohci_alloc_control_pipe %p", &cntl->usb);

    // Allocate a queue head.
    struct ohci_pipe *pipe = malloc_tmphigh(sizeof(*pipe));
    if (!pipe) {
        warn_noalloc();
        return NULL;
    }
    memset(pipe, 0, sizeof(*pipe));
    memcpy(&pipe->pipe, dummy, sizeof(pipe->pipe));
    pipe->ed.hwINFO = ED_SKIP;

    // Add queue head to controller list.
    pipe->ed.hwNextED = cntl->regs->ed_controlhead;
    barrier();
    cntl->regs->ed_controlhead = (u32)&pipe->ed;
    return &pipe->pipe;
}

int
ohci_control(struct usb_pipe *p, int dir, const void *cmd, int cmdsize
             , void *data, int datasize)
{
    if (! CONFIG_USB_OHCI)
        return -1;

    SHOW_FLOW(5, "ohci_control pipe %p", p);

    if (datasize > 4096) {
        // XXX - should support larger sizes.
        warn_noalloc();
        SHOW_ERROR( 1, "datasize > 4096 %p", p);
        return -1;
    }
    struct ohci_pipe *pipe = container_of(p, struct ohci_pipe, pipe);
    struct usb_ohci_s *cntl = container_of(
        pipe->pipe.cntl, struct usb_ohci_s, usb);
    int maxpacket = pipe->pipe.maxpacket;
    int lowspeed = pipe->pipe.speed;
    int devaddr = pipe->pipe.devaddr | (pipe->pipe.ep << 7);

    // Setup transfer descriptors
    struct ohci_td *tds = malloc_tmphigh(sizeof(*tds) * 3);
    if (!tds) {
        warn_noalloc();
        SHOW_ERROR( 1, "malloc tds fail %p", p);
        return -1;
    }
    struct ohci_td *td = tds;
    td->hwINFO = TD_DP_SETUP | TD_T_DATA0 | TD_CC;
    td->hwCBP = (u32)cmd;
    td->hwNextTD = (u32)&td[1];
    td->hwBE = (u32)cmd + cmdsize - 1;
    td++;
    if (datasize) {
        td->hwINFO = (dir ? TD_DP_IN : TD_DP_OUT) | TD_T_DATA1 | TD_CC;
        td->hwCBP = (u32)data;
        td->hwNextTD = (u32)&td[1];
        td->hwBE = (u32)data + datasize - 1;
        td++;
    }
    td->hwINFO = (dir ? TD_DP_OUT : TD_DP_IN) | TD_T_DATA1 | TD_CC;
    td->hwCBP = 0;
    td->hwNextTD = (u32)&td[1];
    td->hwBE = 0;
    td++;

    // Transfer data
    pipe->ed.hwINFO = ED_SKIP;
    barrier();
    pipe->ed.hwHeadP = (u32)tds;
    pipe->ed.hwTailP = (u32)td;
    barrier();
    pipe->ed.hwINFO = devaddr | (maxpacket << 16) | (lowspeed ? ED_LOWSPEED : 0);
    writel(&cntl->regs->cmdstatus, OHCI_CLF);

    dump_ohci_ed( &pipe->ed );

    SHOW_FLOW( 0, "will wait_ed, pipe %x, &ed = %x", pipe, &pipe->ed );
    int ret = wait_ed(&pipe->ed);
    SHOW_ERROR( 1, "wait_ed = %d", ret );

    dump_ohci_ed( &pipe->ed );

    pipe->ed.hwINFO = ED_SKIP;
    if (ret)
        ohci_waittick(cntl);
    free(tds);
    return ret;
}

struct usb_pipe *
ohci_alloc_bulk_pipe(struct usb_pipe *dummy)
{
    (void) dummy;
    if (! CONFIG_USB_OHCI)
        return NULL;
    SHOW_FLOW0(1, "OHCI Bulk transfers not supported.");
    return NULL;
}

int
ohci_send_bulk(struct usb_pipe *p, int dir, void *data, int datasize)
{
    (void) p;
    (void) dir;
    (void) data;
    (void) datasize;
    return -1;
}

struct usb_pipe *
ohci_alloc_intr_pipe(struct usb_pipe *dummy, int frameexp)
{
    if (! CONFIG_USB_OHCI)
        return NULL;
    struct usb_ohci_s *cntl = container_of(
        dummy->cntl, struct usb_ohci_s, usb);
    SHOW_FLOW(7, "ohci_alloc_intr_pipe %p %d", &cntl->usb, frameexp);

    if (frameexp > 5)
        frameexp = 5;
    int maxpacket = dummy->maxpacket;
    int lowspeed = dummy->speed;
    int devaddr = dummy->devaddr | (dummy->ep << 7);
    // Determine number of entries needed for 2 timer ticks.
    int ms = 1<<frameexp;

    //unsigned int count = DIV_ROUND_UP(PIT_TICK_INTERVAL * 1000 * 2, PIT_TICK_RATE * ms)+1;
    unsigned int count = usb_intr_pipe_count(ms)+1;

    struct ohci_pipe *pipe = malloc_low(sizeof(*pipe));
    struct ohci_td *tds = malloc_low(sizeof(*tds) * count);
    void *data = malloc_low(maxpacket * count);
    if (!pipe || !tds || !data)
        goto err;
    memset(pipe, 0, sizeof(*pipe));
    memcpy(&pipe->pipe, dummy, sizeof(pipe->pipe));
    pipe->data = data;
    pipe->count = count;
    pipe->tds = tds;

    struct ohci_ed *ed = &pipe->ed;
    ed->hwHeadP = (u32)&tds[0];
    ed->hwTailP = (u32)&tds[count-1];
    ed->hwINFO = devaddr | (maxpacket << 16) | (lowspeed ? ED_LOWSPEED : 0);

    unsigned int i;
    for (i=0; i<count-1; i++) {
        tds[i].hwINFO = TD_DP_IN | TD_T_TOGGLE | TD_CC;
        tds[i].hwCBP = (u32)data + maxpacket * i;
        tds[i].hwNextTD = (u32)&tds[i+1];
        tds[i].hwBE = tds[i].hwCBP + maxpacket - 1;
    }

    // Add to interrupt schedule.
    barrier();
    struct ohci_hcca *hcca = (void*)cntl->regs->hcca;
    if (frameexp == 0) {
        // Add to existing interrupt entry.
        struct ohci_ed *intr_ed = (void*)hcca->int_table[0];
        ed->hwNextED = intr_ed->hwNextED;
        intr_ed->hwNextED = (u32)ed;
    } else {
        int startpos = 1<<(frameexp-1);
        ed->hwNextED = hcca->int_table[startpos];
        for (i=startpos; i<ARRAY_SIZE(hcca->int_table); i+=ms)
            hcca->int_table[i] = (u32)ed;
    }

    return &pipe->pipe;

err:
    free(pipe);
    free(tds);
    free(data);
    return NULL;
}

int
ohci_poll_intr(struct usb_pipe *p, void *data)
{
    (void) p;
    (void) data;

    ASSERT16();
    if (! CONFIG_USB_OHCI)
        return -1;
#if 1
    struct ohci_pipe *pipe = container_of(p, struct ohci_pipe, pipe);
    struct ohci_td *tds = GET_FLATPTR(pipe->tds);
    struct ohci_td *head = (void*)(GET_FLATPTR(pipe->ed.hwHeadP) & ~(ED_C|ED_H));
    struct ohci_td *tail = (void*)GET_FLATPTR(pipe->ed.hwTailP);
    int count = GET_FLATPTR(pipe->count);
    int pos = (tail - tds + 1) % count;
    struct ohci_td *next = &tds[pos];
    if (head == next)
        // No intrs found.
        return -1;
    // XXX - check for errors.

    // Copy data.
    int maxpacket = GET_FLATPTR(pipe->pipe.maxpacket);
    void *pipedata = GET_FLATPTR(pipe->data);
    void *intrdata = pipedata + maxpacket * pos;
    memcpy_far(GET_SEG(SS), data
               , FLATPTR_TO_SEG(intrdata), (void*)FLATPTR_TO_OFFSET(intrdata)
               , maxpacket);

    // Reenable this td.
    SET_FLATPTR(tail->hwINFO, TD_DP_IN | TD_T_TOGGLE | TD_CC);
    intrdata = pipedata + maxpacket * (tail-tds);
    SET_FLATPTR(tail->hwCBP, (u32)intrdata);
    SET_FLATPTR(tail->hwNextTD, (u32)next);
    SET_FLATPTR(tail->hwBE, (u32)intrdata + maxpacket - 1);
    barrier();
    SET_FLATPTR(pipe->ed.hwTailP, (u32)next);
#endif
    return 0;
}



static void dump_ohci_ed( struct ohci_ed *ed )
{
    SHOW_INFO(1, "OHCI ed hwInfo %x tail=%p head=%p nextEd=%p",
               ed->hwINFO,
               (void *)ed->hwTailP,
               (void *)ed->hwHeadP,
               (void *)ed->hwNextED );
}


#endif // HAVE_USB
