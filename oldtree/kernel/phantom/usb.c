#if HAVE_USB

// Main code for handling USB controllers and devices.
//
// Copyright (C) 2009  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#define DEBUG_MSG_PREFIX "usb"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <compat/seabios.h>

#include "usb-uhci.h" // uhci_init
#include "usb-ohci.h" // ohci_init
#include "usb-ehci.h" // ehci_init
#include "usb-hid.h" // usb_keyboard_setup
#include "usb-hub.h" // usb_hub_init
#include "usb-msc.h" // usb_msc_init
#include "usb.h" // struct usb_s

#include <time.h>
#include <ia32/pio.h>
#include <kernel/init.h>

struct extended_bios_data_area_s usb_ebda2;



u64 calc_future_tsc(u32 msecs)
{
    return hal_system_time() + ( ((u64)1000) * msecs );
}

u64 calc_future_tsc_usec(u32 usecs)
{
    return hal_system_time() + usecs;
}

int check_tsc(u64 end)
{
    int r = hal_system_time() >= end;

    if( r )
        SHOW_ERROR0( 2, "timed out" );

    return r;
}

#define CONFIG_PCI_ROOT1 0x00
#define CONFIG_PCI_ROOT2 0x00

#define PORT_PCI_CMD           0x0cf8
#define PORT_PCI_REBOOT        0x0cf9
#define PORT_PCI_DATA          0x0cfc

void pci_config_writel(u16 bdf, u32 addr, u32 val)
{
    outl( PORT_PCI_CMD, 0x80000000 | (bdf << 8) | (addr & 0xfc) );
    outl( PORT_PCI_DATA, val);
}

void pci_config_writew(u16 bdf, u32 addr, u16 val)
{
    outl( PORT_PCI_CMD, 0x80000000 | (bdf << 8) | (addr & 0xfc));
    outw( PORT_PCI_DATA + (addr & 2), val);
}

void pci_config_writeb(u16 bdf, u32 addr, u8 val)
{
    outl( PORT_PCI_CMD, (0x80000000 | (bdf << 8) | (addr & 0xfc)) );
    outb( PORT_PCI_DATA + (addr & 3), val);
}

u32 pci_config_readl(u16 bdf, u32 addr)
{
    outl( PORT_PCI_CMD, 0x80000000 | (bdf << 8) | (addr & 0xfc));
    return inl(PORT_PCI_DATA);
}

u16 pci_config_readw(u16 bdf, u32 addr)
{
    outl( PORT_PCI_CMD, 0x80000000 | (bdf << 8) | (addr & 0xfc));
    return inw(PORT_PCI_DATA + (addr & 2));
}

u8 pci_config_readb(u16 bdf, u32 addr)
{
    outl( PORT_PCI_CMD, 0x80000000 | (bdf << 8) | (addr & 0xfc));
    return inb(PORT_PCI_DATA + (addr & 3));
}

void
pci_config_maskw(u16 bdf, u32 addr, u16 off, u16 on)
{
    u16 val = pci_config_readw(bdf, addr);
    val = (val & ~off) | on;
    pci_config_writew(bdf, addr, val);
}

// Helper function for foreachpci() macro - return next device
int
pci_next(int bdf, int *pmax)
{
    if (pci_bdf_to_fn(bdf) == 1
        && (pci_config_readb(bdf-1, PCI_HEADER_TYPE) & 0x80) == 0)
        // Last found device wasn't a multi-function device - skip to
        // the next device.
        bdf += 7;

    int max = *pmax;
    for (;;) {
        if (bdf >= max) {
            if (CONFIG_PCI_ROOT1 && bdf <= (CONFIG_PCI_ROOT1 << 8))
                bdf = CONFIG_PCI_ROOT1 << 8;
            else if (CONFIG_PCI_ROOT2 && bdf <= (CONFIG_PCI_ROOT2 << 8))
                bdf = CONFIG_PCI_ROOT2 << 8;
            else
            	return -1;
            *pmax = max = bdf + 0x0100;
        }

        u16 v = pci_config_readw(bdf, PCI_VENDOR_ID);
        if (v != 0x0000 && v != 0xffff)
            // Device is present.
            break;

        if (pci_bdf_to_fn(bdf) == 0)
            bdf += 8;
        else
            bdf += 1;
    }

    // Check if found device is a bridge.
    u32 v = pci_config_readb(bdf, PCI_HEADER_TYPE);
    v &= 0x7f;
    if (v == PCI_HEADER_TYPE_BRIDGE || v == PCI_HEADER_TYPE_CARDBUS) {
        v = pci_config_readl(bdf, PCI_PRIMARY_BUS);
        int newmax = (v & 0xff00) + 0x0100;
        if (newmax > max)
            *pmax = newmax;
    }

    return bdf;
}



/****************************************************************
 * Controller function wrappers
 ****************************************************************/

// Free an allocated control or bulk pipe.
void
free_pipe(struct usb_pipe *pipe)
{
    ASSERT32FLAT();
    if (!pipe)
        return;
    switch (pipe->type) {
    default:
    case USB_TYPE_UHCI:
        return uhci_free_pipe(pipe);
    case USB_TYPE_OHCI:
        return ohci_free_pipe(pipe);
#if CONFIG_USB_EHCI
    case USB_TYPE_EHCI:
        return ehci_free_pipe(pipe);
#endif
    }
}

// Allocate a control pipe to a default endpoint (which can only be
// used by 32bit code)
static struct usb_pipe *
alloc_default_control_pipe(struct usb_pipe *dummy)
{
    switch (dummy->type) {
    default:
    case USB_TYPE_UHCI:
        return uhci_alloc_control_pipe(dummy);
    case USB_TYPE_OHCI:
        return ohci_alloc_control_pipe(dummy);
#if CONFIG_USB_EHCI
    case USB_TYPE_EHCI:
        return ehci_alloc_control_pipe(dummy);
#endif
    }
}

// Send a message on a control pipe using the default control descriptor.
static int
send_control(struct usb_pipe *pipe, int dir, const void *cmd, int cmdsize
             , void *data, int datasize)
{
    ASSERT32FLAT();
    switch (pipe->type) {
    default:
    case USB_TYPE_UHCI:
        return uhci_control(pipe, dir, cmd, cmdsize, data, datasize);
    case USB_TYPE_OHCI:
        return ohci_control(pipe, dir, cmd, cmdsize, data, datasize);
#if CONFIG_USB_EHCI
    case USB_TYPE_EHCI:
        return ehci_control(pipe, dir, cmd, cmdsize, data, datasize);
#endif
    }
}

// Fill "pipe" endpoint info from an endpoint descriptor.
static void
desc2pipe(struct usb_pipe *newpipe, struct usb_pipe *origpipe
          , struct usb_endpoint_descriptor *epdesc)
{
    memcpy(newpipe, origpipe, sizeof(*newpipe));
    newpipe->ep = epdesc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
    newpipe->maxpacket = epdesc->wMaxPacketSize;
}

struct usb_pipe *
alloc_bulk_pipe(struct usb_pipe *pipe, struct usb_endpoint_descriptor *epdesc)
{
    struct usb_pipe dummy;
    desc2pipe(&dummy, pipe, epdesc);
    switch (pipe->type) {
    default:
    case USB_TYPE_UHCI:
        return uhci_alloc_bulk_pipe(&dummy);
    case USB_TYPE_OHCI:
        return ohci_alloc_bulk_pipe(&dummy);
#if CONFIG_USB_EHCI
    case USB_TYPE_EHCI:
        return ehci_alloc_bulk_pipe(&dummy);
#endif
    }
}

int
usb_send_bulk(struct usb_pipe *pipe_fl, int dir, void *data, int datasize)
{
    switch (GET_FLATPTR(pipe_fl->type)) {
    default:
    case USB_TYPE_UHCI:
        return uhci_send_bulk(pipe_fl, dir, data, datasize);
    case USB_TYPE_OHCI:
        return ohci_send_bulk(pipe_fl, dir, data, datasize);
#if CONFIG_USB_EHCI
    case USB_TYPE_EHCI:
        return ehci_send_bulk(pipe_fl, dir, data, datasize);
#endif
    }
}

struct usb_pipe *
alloc_intr_pipe(struct usb_pipe *pipe, struct usb_endpoint_descriptor *epdesc)
{
    struct usb_pipe dummy;
    desc2pipe(&dummy, pipe, epdesc);
    // Find the exponential period of the requested time.
    int period = epdesc->bInterval;
    int frameexp;
    if (pipe->speed != USB_HIGHSPEED)
        frameexp = (period <= 0) ? 0 : __fls(period);
    else
        frameexp = (period <= 4) ? 0 : period - 4;
    switch (pipe->type) {
    default:
    case USB_TYPE_UHCI:
        return uhci_alloc_intr_pipe(&dummy, frameexp);
    case USB_TYPE_OHCI:
        return ohci_alloc_intr_pipe(&dummy, frameexp);
#if CONFIG_USB_EHCI
    case USB_TYPE_EHCI:
        return ehci_alloc_intr_pipe(&dummy, frameexp);
#endif
    }
}

int noinline
usb_poll_intr(struct usb_pipe *pipe_fl, void *data)
{
    switch (GET_FLATPTR(pipe_fl->type)) {
    default:
    case USB_TYPE_UHCI:
        return uhci_poll_intr(pipe_fl, data);
    case USB_TYPE_OHCI:
        return ohci_poll_intr(pipe_fl, data);
#if CONFIG_USB_EHCI
    case USB_TYPE_EHCI:
        return ehci_poll_intr(pipe_fl, data);
#endif
    }
}


/****************************************************************
 * Helper functions
 ****************************************************************/

// Find the first endpoing of a given type in an interface description.
struct usb_endpoint_descriptor *
findEndPointDesc(struct usb_interface_descriptor *iface, int imax
                 , int type, int dir)
{
    struct usb_endpoint_descriptor *epdesc = (void*)&iface[1];
    for (;;) {
        if ((void*)epdesc >= (void*)iface + imax
            || epdesc->bDescriptorType == USB_DT_INTERFACE) {
            return NULL;
        }
        if (epdesc->bDescriptorType == USB_DT_ENDPOINT
            && (epdesc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == dir
            && (epdesc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == type)
            return epdesc;
        epdesc = (void*)epdesc + epdesc->bLength;
    }
}

// Send a message to the default control pipe of a device.
int
send_default_control(struct usb_pipe *pipe, const struct usb_ctrlrequest *req
                     , void *data)
{
    SHOW_FLOW( 6, "pipe %p datasize %d", pipe, req->wLength );
    return send_control(pipe, req->bRequestType & USB_DIR_IN
                        , req, sizeof(*req), data, req->wLength);
}

// Get the first 8 bytes of the device descriptor.
static int
get_device_info8(struct usb_pipe *pipe, struct usb_device_descriptor *dinfo)
{
    SHOW_FLOW0( 7, "enter" );
    struct usb_ctrlrequest req;
    req.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest = USB_REQ_GET_DESCRIPTOR;
    req.wValue = USB_DT_DEVICE<<8;
    req.wIndex = 0;
    req.wLength = 8;
    return send_default_control(pipe, &req, dinfo);
}

static struct usb_config_descriptor *
get_device_config(struct usb_pipe *pipe)
{
    SHOW_FLOW0( 7, "enter" );

    struct usb_config_descriptor cfg;

    struct usb_ctrlrequest req;
    req.bRequestType = USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest = USB_REQ_GET_DESCRIPTOR;
    req.wValue = USB_DT_CONFIG<<8;
    req.wIndex = 0;
    req.wLength = sizeof(cfg);
    int ret = send_default_control(pipe, &req, &cfg);
    if (ret)
        return NULL;

    void *config = malloc_tmphigh(cfg.wTotalLength);
    if (!config)
        return NULL;
    req.wLength = cfg.wTotalLength;
    ret = send_default_control(pipe, &req, config);
    if (ret)
        return NULL;
    //hexdump(config, cfg.wTotalLength);
    return config;
}

static int
set_configuration(struct usb_pipe *pipe, u16 val)
{
    SHOW_FLOW0( 7, "enter" );
    struct usb_ctrlrequest req;
    req.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest = USB_REQ_SET_CONFIGURATION;
    req.wValue = val;
    req.wIndex = 0;
    req.wLength = 0;
    return send_default_control(pipe, &req, NULL);
}


/****************************************************************
 * Initialization and enumeration
 ****************************************************************/

// Assign an address to a device in the default state on the given
// controller.
static struct usb_pipe *
usb_set_address(struct usbhub_s *hub, int port, int speed)
{
    ASSERT32FLAT();
    struct usb_s *cntl = hub->cntl;

    SHOW_FLOW( 3, "cntl %p", cntl);
    

    if (cntl->maxaddr >= USB_MAXADDR)
        return NULL;

    struct usb_pipe *defpipe = cntl->defaultpipe;
    if (!defpipe) {
        // Create a pipe for the default address.
        struct usb_pipe dummy;
        memset(&dummy, 0, sizeof(dummy));
        dummy.cntl = cntl;
        dummy.type = cntl->type;
        dummy.maxpacket = 8;
        dummy.path = (u64)-1;
        cntl->defaultpipe = defpipe = alloc_default_control_pipe(&dummy);
        if (!defpipe)
        {
            SHOW_ERROR( 2, "alloc def pipe failed at port %d", port );
            return NULL;
        }
    }
    defpipe->speed = speed;
    if (hub->pipe) {
        if (hub->pipe->speed == USB_HIGHSPEED) {
            defpipe->tt_devaddr = hub->pipe->devaddr;
            defpipe->tt_port = port;
        } else {
            defpipe->tt_devaddr = hub->pipe->tt_devaddr;
            defpipe->tt_port = hub->pipe->tt_port;
        }
    } else {
        defpipe->tt_devaddr = defpipe->tt_port = 0;
    }

    msleep(USB_TIME_RSTRCY);

    struct usb_ctrlrequest req;
    req.bRequestType = USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE;
    req.bRequest = USB_REQ_SET_ADDRESS;
    req.wValue = cntl->maxaddr + 1;
    req.wIndex = 0;
    req.wLength = 0;
    int ret = send_default_control(defpipe, &req, NULL);
    if (ret)
    {
        SHOW_ERROR( 2, "send def control failed at port %d", port );
        return NULL;
    }

    msleep(USB_TIME_SETADDR_RECOVERY);

    cntl->maxaddr++;
    defpipe->devaddr = cntl->maxaddr;
    struct usb_pipe *pipe = alloc_default_control_pipe(defpipe);
    defpipe->devaddr = 0;
    if (hub->pipe)
        pipe->path = hub->pipe->path;
    pipe->path = (pipe->path << 8) | port;
    return pipe;
}

// Called for every found device - see if a driver is available for
// this device and do setup if so.
static int
configure_usb_device(struct usb_pipe *pipe)
{
    ASSERT32FLAT();
    SHOW_FLOW( 3, "config_usb: %p", pipe);

    // Set the max packet size for endpoint 0 of this device.
    struct usb_device_descriptor dinfo;
    int ret = get_device_info8(pipe, &dinfo);
    if (ret)
    {
        SHOW_ERROR( 1, "get_device_info8 rc = %d", ret );
        return 0;
    }

    SHOW_FLOW(3, "device rev=%04x cls=%02x sub=%02x proto=%02x size=%02x"
            , dinfo.bcdUSB, dinfo.bDeviceClass, dinfo.bDeviceSubClass
            , dinfo.bDeviceProtocol, dinfo.bMaxPacketSize0);

    if(dinfo.bMaxPacketSize0 < 8 || dinfo.bMaxPacketSize0 > 64)
    {
        SHOW_ERROR( 1, "dinfo.bMaxPacketSize0 = %d", dinfo.bMaxPacketSize0 );
        return 0;
    }

    pipe->maxpacket = dinfo.bMaxPacketSize0;

    SHOW_FLOW( 3, "get_device_config: %p", pipe);
    // Get configuration
    struct usb_config_descriptor *config = get_device_config(pipe);
    if (!config)
        return 0;

    // Determine if a driver exists for this device - only look at the
    // first interface of the first configuration.
    struct usb_interface_descriptor *iface = (void*)(&config[1]);
    if (iface->bInterfaceClass != USB_CLASS_HID
        && iface->bInterfaceClass != USB_CLASS_MASS_STORAGE
        && iface->bInterfaceClass != USB_CLASS_HUB)
    {
        // Not a supported device.
        SHOW_ERROR( 1, "not supported iface->bInterfaceClass = %d", iface->bInterfaceClass );
        goto fail;
    }

    SHOW_FLOW( 3, "set_configuration: %d", config->bConfigurationValue );
    // Set the configuration.
    ret = set_configuration(pipe, config->bConfigurationValue);
    if (ret)
        goto fail;

    // Configure driver.
    int imax = (void*)config + config->wTotalLength - (void*)iface;
    if (iface->bInterfaceClass == USB_CLASS_HUB)
        ret = usb_hub_init(pipe);
#if CONFIG_USB_MSC
    else if (iface->bInterfaceClass == USB_CLASS_MASS_STORAGE)
        ret = usb_msc_init(pipe, iface, imax);
#endif
    else
        ret = usb_hid_init(pipe, iface, imax);
    if (ret)
        goto fail;

    free(config);
    return 1;
fail:
    free(config);
    return 0;
}

static void
usb_init_hub_port(void *data)
{
    struct usbhub_s *hub = data;
    u32 port = hub->port; // XXX - find better way to pass port

    // Detect if device present (and possibly start reset)
    int ret = hub->op->detect(hub, port);
    if (ret)
    {
        // No device present
        SHOW_ERROR( 2, "No dev at port %d", port );
        goto done;
    }

    // TODO! Find BETTER place to do that! Might happen twice!
    hal_mutex_init(&hub->cntl->resetlock, "hub_reset");

    // Reset port and determine device speed
    mutex_lock(&hub->cntl->resetlock);
    ret = hub->op->reset(hub, port);
    if (ret < 0)
    {
        // Reset failed
        SHOW_ERROR( 2, "Reset failed at port %d", port );
        goto resetfail;
    }

    // Set address of port
    struct usb_pipe *pipe = usb_set_address(hub, port, ret);
    if (!pipe) {
        hub->op->disconnect(hub, port);
        SHOW_ERROR( 2, "Set address failed at port %d", port );
        goto resetfail;
    }
    mutex_unlock(&hub->cntl->resetlock);

    // Configure the device
    int count = configure_usb_device(pipe);
    free_pipe(pipe);
    if (!count)
        hub->op->disconnect(hub, port);
    hub->devcount += count;
done:
    hub->threads--;
    return;

resetfail:
    mutex_unlock(&hub->cntl->resetlock);
    goto done;
}

void
usb_enumerate(struct usbhub_s *hub)
{
    u32 portcount = hub->portcount;
    //hub->threads = portcount;
    hub->threads = 0;

    SHOW_FLOW( 0, "hub %p", hub );

    // Launch a thread for every port.
    unsigned int i;
    for (i=0; i<portcount; i++)
    {
        SHOW_FLOW( 2, "Look at port %d", i );

        hub->port = i;
        hub->threads++;

        //run_thread(usb_init_hub_port, hub);
        //while (hub->threads)            yield();
        usb_init_hub_port( hub );
    }

    // Wait for threads to complete.
    //while (hub->threads)        yield();

}

void
usb_setup(void)
{
    ASSERT32FLAT();
    if (! CONFIG_USB)
        return;

    SHOW_FLOW0(3, "init usb");

    // Look for USB controllers
#if CONFIG_USB_EHCI
    int ehcibdf = -1;
#endif
    int count = 0;
    int bdf, max;
    foreachpci(bdf, max) {
        u32 code = pci_config_readl(bdf, PCI_CLASS_REVISION) >> 8;

        if (code >> 8 != PCI_CLASS_SERIAL_USB)
            continue;

#if CONFIG_USB_EHCI
        if (bdf > ehcibdf) {
            // Check to see if this device has an ehci controller
            ehcibdf = bdf;
            u32 ehcicode = code;
            int found = 0;
            for (;;) {
                if (ehcicode == PCI_CLASS_SERIAL_USB_EHCI) {
                    // Found an ehci controller.

                    int ret = ehci_init(ehcibdf, count++, bdf);
                    if (ret)
                        // Error
                        break;
                    count += found;
                    bdf = ehcibdf;
                    code = 0;
                    break;
                }
                if (ehcicode >> 8 == PCI_CLASS_SERIAL_USB)
                    found++;
                ehcibdf = pci_next(ehcibdf+1, &max);
                if (ehcibdf < 0
                    || pci_bdf_to_busdev(ehcibdf) != pci_bdf_to_busdev(bdf))
                    // No ehci controller found.
                    break;
                ehcicode = pci_config_readl(ehcibdf, PCI_CLASS_REVISION) >> 8;
            }
        }
#endif
        if (code == PCI_CLASS_SERIAL_USB_UHCI)
            uhci_init(bdf, count++);
        else if (code == PCI_CLASS_SERIAL_USB_OHCI)
            ohci_init(bdf, count++);
    }
}

#endif // HAVE_USB
