/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * VMWare 'SVGA' driver.
 *
 **/

#ifdef ARCH_ia32

#define DEBUG_MSG_PREFIX "vmware"
#include <debug_ext.h>
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <hal.h>
#include <errno.h>
#include <device.h>

#include <ia32/pio.h>

#include <kernel/page.h>
#include <kernel/atomic.h>
#include <kernel/libkern.h>
#include <phantom_libc.h>
#include <video/screen.h>
#include <video/internal.h>

#include <dev/pci/vmware/svga_reg.h>
#include <dev/pci/vmware/svga.h>


//#define VMWARE_VIDEO_DRV_DEFAULT_X_SIZE 1024
//#define VMWARE_VIDEO_DRV_DEFAULT_Y_SIZE 768
#define VMWARE_VIDEO_DRV_DEFAULT_X_SIZE 1280
//#define VMWARE_VIDEO_DRV_DEFAULT_Y_SIZE 960
#define VMWARE_VIDEO_DRV_DEFAULT_Y_SIZE 900

static void dump_bits( u_int8_t *bits, size_t noutbytes ) __attribute__((unused));


static void vmware_draw_mouse_bp2(void);
static void vmware_set_mouse_cursor_bp2( drv_video_bitmap_t *cursor );
static void vmware_mouse_on_bp2(void);
static void vmware_mouse_off_bp2(void);


static int vmware_video_probe();
static int vmware_video_start();
static int vmware_video_stop();
static void vmware_video_update(void);
static errno_t vmware_accel_start(void);

static void vmware_accel_copy(int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize ); // Screen to screen copy    
static void vmware_accel_clear(int xpos, int ypos, int xsize, int ysize ); // Screen rect clear


static void vmware_detect2(int xsize, int ysize, int bpp);


u_int32_t SVGA_ReadReg(u_int32_t index);
void SVGA_WriteReg(u_int32_t index,  u_int32_t value);
u_int32_t SVGA_ClearIRQ(void);
//static void SVGAInterruptHandler(void *arg);

void SVGAFIFOFull(void);


void SVGA_Panic(const char *s) { panic("VmWare SVGA: %s", s); }

SVGADevice gSVGA; // TODO rename

static hal_mutex_t fifo_mutex;

struct drv_video_screen_t        video_driver_vmware_svga =
{
    "VMWare SVGA II",
    // size
    VMWARE_VIDEO_DRV_DEFAULT_X_SIZE, VMWARE_VIDEO_DRV_DEFAULT_Y_SIZE,
    // bpp
    //24,
    32,
    // mouse x y flags
    0, 0, 0,

    // screen
.screen		=	0,

.probe	        =	vmware_video_probe,
.start          = 	vmware_video_start,
.accel          =	vmware_accel_start,
.stop           =	vmware_video_stop,

.update         =	vmware_video_update,

// todo hw mouse!
#if 0
mouse_redraw_cursor: 	drv_video_draw_mouse_deflt,
mouse_set_cursor: 	drv_video_set_mouse_cursor_deflt,
mouse_disable:          drv_video_mouse_off_deflt,
mouse_enable:          	drv_video_mouse_on_deflt,
#endif

};




static int vmware_video_probe()
{
    return gSVGA.found ? VIDEO_PROBE_SUCCESS : VIDEO_PROBE_FAIL;
}





// need 4Mbytes aperture
//#define N_PAGES 1024
//#define N_PAGES 2048

static void vmware_map_video(int on_off)
{
    int n_pages = BYTES_TO_PAGES(gSVGA.fbSize);

    if( video_driver_vmware_svga.screen == 0 )
    {
        void *vva;

        if( hal_alloc_vaddress(&vva, n_pages) )
            panic("Can't alloc vaddress for %d videmem pages", n_pages);

        video_driver_vmware_svga.screen = vva;
    }

    hal_pages_control( gSVGA.fbMem, video_driver_vmware_svga.screen, n_pages, on_off ? page_map_io : page_unmap, page_rw );

}

static int vmware_video_start()
{
    vmware_detect2( video_driver_vmware_svga.xsize, video_driver_vmware_svga.ysize, video_driver_vmware_svga.bpp );

    switch_screen_bitblt_to_32bpp(1);
    vmware_map_video( 1 );

    // In fact, detect already set it - here we init fifo
    SVGA_SetMode( video_driver_vmware_svga.xsize, video_driver_vmware_svga.ysize, video_driver_vmware_svga.bpp );

    return 0;
}

static int vmware_video_stop()
{
    vmware_map_video(0);
    SVGA_WriteReg(SVGA_REG_ENABLE, 0 );
    //vbe_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    video_drv_basic_vga_set_text_mode();
    return 0;
}

static errno_t vmware_accel_start(void)
{
    if( !gSVGA.found )
        return ENXIO;

    vmware_detect2( video_drv->xsize, video_drv->ysize, video_drv->bpp );

    // In fact, detect already set it - here we init fifo
    SVGA_SetMode( video_drv->xsize, video_drv->ysize, video_drv->bpp );

    video_drv->update = vmware_video_update;

#if 1 // doesn't work
    SHOW_FLOW( 0, "VmWare accelerator add on start, id %x", gSVGA.deviceVersionId );

    if( gSVGA.capabilities & SVGA_CAP_CURSOR )
    {
        int cpb2 = gSVGA.capabilities & SVGA_CAP_CURSOR_BYPASS_2;
        int alpha = gSVGA.capabilities & SVGA_CAP_ALPHA_CURSOR;
        SHOW_FLOW( 2, "capas: cursor %s %s", (cpb2 ? "bypass2" : ""), alpha ? "alpha" : "(and/xor only)"  );

        if(cpb2)
        {
            // Take over mouse
            video_drv->mouse_redraw_cursor = vmware_draw_mouse_bp2;
            video_drv->mouse_set_cursor    = vmware_set_mouse_cursor_bp2;
            video_drv->mouse_disable       = vmware_mouse_off_bp2;
            video_drv->mouse_enable        = vmware_mouse_on_bp2;
        }

    }
#endif

    /*
    // now take over screen too - it seems that we ruin default vmem mapping somehow
    vmware_map_video(1);
    video_drv->screen = video_driver_vmware_svga.screen;
    */
    return 0;
}


// -----------------------------------------------------------------------
// VMWARE SVGA II probe
// -----------------------------------------------------------------------



phantom_device_t * driver_vmware_svga_pci_probe( pci_cfg_t *pci, int stage )
{
    (void) stage;
    // TODO why?
#if 0
    (void) pci;
    return 0;
#else
    SHOW_FLOW0( 0, "Init" );

    hal_mutex_init( &fifo_mutex, "vmware fifo" );
    /*
     * Use the default base address for each memory region.
     * We must map at least ioBase before using ReadReg/WriteReg.
     */

    //PCI_SetMemEnable(&gSVGA.pciAddr, 1);
    phantom_pci_enable( pci, 1 );

    gSVGA.ioBase =  pci->base[0];
    gSVGA.fbMem =   pci->base[1];
    gSVGA.fifoPhys = pci->base[2];

    //gSVGA.fbMemSize =   pci->size[1];

    SHOW_FLOW( 1, "io %p, framebuf %p, fifo %p", (void *)gSVGA.ioBase, (void *)gSVGA.fbMem, (void *)gSVGA.fifoPhys );



    /*
     * Version negotiation:
     *
     *   1. Write to SVGA_REG_ID the maximum ID supported by this driver.
     *   2. Read from SVGA_REG_ID
     *      a. If we read back the same value, this ID is supported. We're done.
     *      b. If not, decrement the ID and repeat.
     */

    gSVGA.deviceVersionId = SVGA_ID_2;
    do {
        SVGA_WriteReg(SVGA_REG_ID, gSVGA.deviceVersionId);
        if (SVGA_ReadReg(SVGA_REG_ID) == gSVGA.deviceVersionId) {
            break;
        } else {
            gSVGA.deviceVersionId--;
        }
    } while (gSVGA.deviceVersionId >= SVGA_ID_0);

    if (gSVGA.deviceVersionId < SVGA_ID_0) {
        SHOW_ERROR0( 0, "Error negotiating SVGA device version.");
        return 0;
    }

    SHOW_FLOW( 0, "device version = %d", gSVGA.deviceVersionId & 0xFF );

#if 0
    vmware_detect2();
#endif
    /*
     * If the device is new enough to support capability flags, get the
     * capabilities register.
     */

    if (gSVGA.deviceVersionId >= SVGA_ID_1) {
        gSVGA.capabilities = SVGA_ReadReg(SVGA_REG_CAPABILITIES);
        SHOW_FLOW( 1, "capas %x", gSVGA.capabilities );

        if( gSVGA.capabilities & SVGA_CAP_RECT_FILL )
        {
            // QEMU does it, TODO
            video_driver_vmware_svga.clear = &vmware_accel_clear;
            SHOW_FLOW0( 2, "capas: rect fill" );
        }

        if( gSVGA.capabilities & SVGA_CAP_RECT_COPY )
        {
            // QEMU does it, TODO
            SHOW_FLOW0( 2, "capas: rect copy" );
            video_driver_vmware_svga.copy = &vmware_accel_copy;
        }

        if( gSVGA.capabilities & SVGA_CAP_RECT_PAT_FILL )
        {
            SHOW_FLOW0( 2, "capas: rect pat fill" );
        }

        if( gSVGA.capabilities & SVGA_CAP_RASTER_OP )
        {
            SHOW_FLOW0( 2, "capas: raster op" );
        }

        if( gSVGA.capabilities & SVGA_CAP_GLYPH )
        {
            SHOW_FLOW0( 2, "capas: glyph" );
        }

        if( gSVGA.capabilities & SVGA_CAP_EXTENDED_FIFO )
        {
            SHOW_FLOW0( 2, "capas: ext fifo" );
        }

        if( gSVGA.capabilities & SVGA_CAP_CURSOR )
        {
            int cpb2 = gSVGA.capabilities & SVGA_CAP_CURSOR_BYPASS_2;
            int alpha = gSVGA.capabilities & SVGA_CAP_ALPHA_CURSOR;
            SHOW_FLOW( 2, "capas: cursor %s %s", (cpb2 ? "bypass2" : ""), alpha ? "alpha" : "(and/xor only)"  );

            if(cpb2)
            {
                video_driver_vmware_svga.mouse_redraw_cursor = vmware_draw_mouse_bp2;
                video_driver_vmware_svga.mouse_set_cursor    = vmware_set_mouse_cursor_bp2;
                video_driver_vmware_svga.mouse_disable       = vmware_mouse_off_bp2;
                video_driver_vmware_svga.mouse_enable        = vmware_mouse_on_bp2;
            }

        }
    }



    /*
     * Optional interrupt initialization.
     *
     * This uses the default IRQ that was assigned to our
     * device by the BIOS.
     */

    if(gSVGA.capabilities & SVGA_CAP_IRQMASK) {
        //uint8 irq = PCI_ConfigRead8(&gSVGA.pciAddr, offsetof(PCIConfigSpace, intrLine));
        int irq = pci->interrupt;

        /* Start out with all SVGA IRQs masked */
        SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

        /* Clear all pending IRQs stored by the device */
        outl(gSVGA.ioBase + SVGA_IRQSTATUS_PORT, 0xFF);

        /* Clear all pending IRQs stored by us */
        SVGA_ClearIRQ();

        /* Enable the IRQ */
        //Intr_SetHandler(IRQ_VECTOR(irq), SVGAInterruptHandler);
        //Intr_SetMask(irq, TRUE);
        /*
        if( hal_irq_alloc( irq, &SVGAInterruptHandler, 0, HAL_IRQ_SHAREABLE ) )
        {
            SHOW_ERROR( 0, "IRQ %d is busy", irq );
            return 0;
        }*/

        SHOW_FLOW( 1, "(unused) irq %d", irq );
    }

    gSVGA.found = 1;

    // We have to report success or there will be more attempts to probe

    phantom_device_t * dev = calloc(1, sizeof(phantom_device_t));
    assert(dev != 0);

    dev->name = "VMWareSVGA";
    dev->seq_number = 0;

    return dev;
#endif
}


static void vmware_detect2(int xsize, int ysize, int bpp)
{
    // dz - original stub driver fails on reading fb size.
    // apparently some sane numbers are needed in theese regs
    SVGA_WriteReg(SVGA_REG_ENABLE, 0 );

    SVGA_WriteReg(SVGA_REG_WIDTH, 		xsize );
    SVGA_WriteReg(SVGA_REG_HEIGHT, 		ysize );
    SVGA_WriteReg(SVGA_REG_BITS_PER_PIXEL, 	bpp   );

    SVGA_WriteReg(SVGA_REG_ENABLE, 1 );

    /*
     * We must determine the FIFO and FB size after version
     * negotiation, since the default version (SVGA_ID_0)
     * does not support the FIFO buffer at all.
     */

    gSVGA.fifoSize = SVGA_ReadReg(SVGA_REG_MEM_SIZE);
    gSVGA.fbSize = SVGA_ReadReg(SVGA_REG_FB_SIZE);

    gSVGA.fbOffset = SVGA_ReadReg(SVGA_REG_FB_OFFSET);

    SHOW_FLOW( 1, "framebuf size %dKb, offset %x, fifo size %d", gSVGA.fbSize/1024, gSVGA.fbOffset, gSVGA.fifoSize );

    /*
     * Sanity-check the FIFO and framebuffer sizes.
     * These are arbitrary values.
     */

    if (gSVGA.fbSize < 0x100000) {
        SHOW_ERROR( 0, "FB size (%d) very small, probably incorrect.", gSVGA.fbSize );
    }
    if (gSVGA.fifoSize < 0x10000) {
        SHOW_ERROR( 0, "FIFO size (%d) very small, probably incorrect.", gSVGA.fifoSize );
    }

    if(gSVGA.fifoMem == 0)
    {
        void *vva;
        int nfifo_pages = BYTES_TO_PAGES(gSVGA.fifoSize);

        if( hal_alloc_vaddress(&vva, nfifo_pages) )
            panic("Can't alloc vaddress for %d fifo pages", nfifo_pages);

        gSVGA.fifoMem = vva;
        hal_pages_control( gSVGA.fifoPhys, gSVGA.fifoMem, nfifo_pages, page_map_io, page_rw );
    }


}




/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_ReadReg --
 *
 *      Read an SVGA device register (SVGA_REG_*).
 *      This must be called after the PCI device's IOspace has been mapped.
 *
 * Results:
 *      32-bit register value.
 *
 * Side effects:
 *      Depends on the register value. Some special registers
 *      like SVGA_REG_BUSY have significant side-effects.
 *
 *-----------------------------------------------------------------------------
 */


u_int32_t
SVGA_ReadReg(u_int32_t index)  // IN
{
   outl(gSVGA.ioBase + SVGA_INDEX_PORT, index);
   return inl(gSVGA.ioBase + SVGA_VALUE_PORT);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_WriteReg --
 *
 *      Write an SVGA device register (SVGA_REG_*).
 *      This must be called after the PCI device's IOspace has been mapped.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Depends on the register value.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_WriteReg(u_int32_t index, u_int32_t value)
{
   outl(gSVGA.ioBase + SVGA_INDEX_PORT, index);
   outl(gSVGA.ioBase + SVGA_VALUE_PORT, value);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_ClearIRQ --
 *
 *      Clear all pending IRQs. Any IRQs which occurred prior to this
 *      function call will be ignored by the next SVGA_WaitForIRQ()
 *      call.
 *
 *      Does not affect the current IRQ mask. This function is not
 *      useful unless the SVGA device has IRQ support.
 *
 * Results:
 *      Returns a mask of all the interrupt flags that were set prior
 *      to the clear.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

u_int32_t SVGA_ClearIRQ(void)
{
    return ATOMIC_FETCH_AND_SET( (int *) &gSVGA.irq.pending, 0 );
}

#if 0
/*
 *-----------------------------------------------------------------------------
 *
 * SVGAInterruptHandler --
 *
 *      This is the ISR for the SVGA device's interrupt. We ask the
 *      SVGA device which interrupt occurred, and clear its flag.
 *
 *      To report this IRQ to the rest of the driver, we:
 *
 *        1. Atomically remember the IRQ in the irq.pending bitmask.
 *
 *        2. Optionally switch to a different light-weight thread
 *           or a different place in this thread upon returning.
 *           This is analogous to waking up a sleeping thread in
 *           a driver for a real operating system. See SVGA_WaitForIRQ
 *           for details.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Sets bits in pendingIRQs. Reads and clears the device's IRQ flags.
 *      May switch execution contexts.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGAInterruptHandler(void *arg)  // IN (unused)
{
    (void) arg;
    //IntrContext *context = Intr_GetContext(vector);

    /*
     * The SVGA_IRQSTATUS_PORT is a separate I/O port, not a register.
     * Reading from it gives us the set of IRQ flags which are
     * set. Writing a '1' to any bit in this register will clear the
     * corresponding flag.
     *
     * Here, we read then clear all flags.
     */

    u_int16_t port = gSVGA.ioBase + SVGA_IRQSTATUS_PORT;
    u_int32_t irqFlags = inl(port);
    outl(port, irqFlags);

    gSVGA.irq.count++;

    if (!irqFlags) {
        SHOW_ERROR0( 1, "Spurious SVGA IRQ" );
    }

    atomic_or( (int *)&gSVGA.irq.pending, irqFlags);

    /*
    if (gSVGA.irq.switchContext) {
        memcpy((void*) &gSVGA.irq.oldContext, context, sizeof *context);
        memcpy(context, (void*) &gSVGA.irq.newContext, sizeof *context);
        gSVGA.irq.switchContext = FALSE;
        }
    */
}
#endif

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_SetMode --
 *
 *      This switches to SVGA video mode, and enables the command FIFO.
 *
 * Results:
 *      void.
 *
 * Side effects:
 *      Switches modes. Disables legacy video modes (VGA, VBE).
 *      Clears the command FIFO, and asks the host to start processing it.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_SetMode(uint32 width,   // IN
             uint32 height,  // IN
             uint32 bpp)     // IN
{
   gSVGA.width = width;
   gSVGA.height = height;
   gSVGA.bpp = bpp;

   SVGA_WriteReg(SVGA_REG_WIDTH, width);
   SVGA_WriteReg(SVGA_REG_HEIGHT, height);
   SVGA_WriteReg(SVGA_REG_BITS_PER_PIXEL, bpp);
   SVGA_WriteReg(SVGA_REG_ENABLE, 1);
   gSVGA.pitch = SVGA_ReadReg(SVGA_REG_BYTES_PER_LINE);

   /*
    * Initialize the command FIFO. The beginning of FIFO memory is
    * used for an additional set of registers, the "FIFO registers".
    * These are higher-performance memory mapped registers which
    * happen to live in the same space as the FIFO. The driver is
    * responsible for allocating space for these registers, according
    * to the maximum number of registers supported by this driver
    * release.
    */

   gSVGA.fifoMem[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * sizeof(uint32);
   gSVGA.fifoMem[SVGA_FIFO_MAX] = gSVGA.fifoSize;
   gSVGA.fifoMem[SVGA_FIFO_NEXT_CMD] = gSVGA.fifoMem[SVGA_FIFO_MIN];
   gSVGA.fifoMem[SVGA_FIFO_STOP] = gSVGA.fifoMem[SVGA_FIFO_MIN];

   /*
    * Prep work for 3D version negotiation. See SVGA3D_Init for
    * details, but we have to give the host our 3D protocol version
    * before enabling the FIFO.
    */

   if (SVGA_HasFIFOCap(SVGA_CAP_EXTENDED_FIFO) &&
       SVGA_IsFIFORegValid(SVGA_FIFO_GUEST_3D_HWVERSION)) {

      gSVGA.fifoMem[SVGA_FIFO_GUEST_3D_HWVERSION] = SVGA3D_HWVERSION_CURRENT;
   }

   /*
    * Enable the FIFO.
    */

   SVGA_WriteReg(SVGA_REG_CONFIG_DONE, 1);

   /*
    * Now that the FIFO is initialized, we can do an IRQ sanity check.
    * This makes sure that the VM's chipset and our own IRQ code
    * works. Better to find out now if something's wrong, than to
    * deadlock later.
    *
    * This inserts a FIFO fence, does a legacy sync to drain the FIFO,
    * then ensures that we received all applicable interrupts.
    */
#if 0
   if (gSVGA.capabilities & SVGA_CAP_IRQMASK) {

      SVGA_WriteReg(SVGA_REG_IRQMASK, SVGA_IRQFLAG_ANY_FENCE);
      SVGA_ClearIRQ();

      SVGA_InsertFence();

      SVGA_WriteReg(SVGA_REG_SYNC, 1);
      while (SVGA_ReadReg(SVGA_REG_BUSY) != 0);

      SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

      /* Check whether the interrupt occurred without blocking. */
      if ((gSVGA.irq.pending & SVGA_IRQFLAG_ANY_FENCE) == 0) {
         SVGA_Panic("SVGA IRQ appears to be present but broken.");
      }

      SVGA_WaitForIRQ();
   }
#endif
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_IsFIFORegValid --
 *
 *      Check whether space has been allocated for a particular FIFO register.
 *
 *      This isn't particularly useful for our simple examples, since the
 *      same binary is responsible for both allocating and using the FIFO,
 *      but a real driver may need to check this if the module which handles
 *      initialization and mode switches is separate from the module which
 *      actually writes to the FIFO.
 *
 * Results:
 *      TRUE if the register has been allocated, FALSE if the register
 *      does not exist.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

Bool
SVGA_IsFIFORegValid(unsigned int reg)
{
   return gSVGA.fifoMem[SVGA_FIFO_MIN] > (reg << 2);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_HasFIFOCap --
 *
 *      Check whether the SVGA device has a particular FIFO capability bit.
 *
 * Results:
 *      TRUE if the capability is present, FALSE if it's absent.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

Bool
SVGA_HasFIFOCap(int cap)
{
   return (gSVGA.fifoMem[SVGA_FIFO_CAPABILITIES] & cap) != 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOReserve --
 *
 *      Begin writing a command to the FIFO buffer. There are several
 *      examples floating around which show how to write to the FIFO
 *      buffer, but this is the preferred method: write directly to
 *      FIFO memory in the common case, but if the command would not
 *      be contiguous, use a bounce buffer.
 *
 *      This method is easy to use, and quite fast. The X.org driver
 *      does not yet use this method, but recent Windows drivers use
 *      it.
 *
 *      The main principles here are:
 *
 *        - There are multiple code paths. In the best case, we write
 *          directly to the FIFO. In the next-best case, we use a
 *          static bounce buffer.  If you need to support arbitrarily
 *          large commands, you can have a worst case in which you use
 *          a dynamically sized bounce buffer.
 *
 *        - We must tell the host that we're reserving FIFO
 *          space. This is important because the device doesn't
 *          guarantee it will preserve the contents of FIFO memory
 *          which hasn't been reserved. If we write to a totally
 *          unused portion of the FIFO and the VM is suspended, on
 *          resume that data will no longer exist.
 *
 *      This function is not re-entrant. If your driver is
 *      multithreaded or may be used from multiple processes
 *      concurrently, you must make sure to serialize all FIFO
 *      commands.
 *
 *      The caller must pair this command with SVGA_FIFOCommit or
 *      SVGA_FIFOCommitAll.
 *
 * Results:
 *      Returns a pointer to the location where the FIFO command can
 *      be written. There will be room for at least 'bytes' bytes of
 *      data.
 *
 * Side effects:
 *      Begins a FIFO command, reserves space in the FIFO.
 *      May block (in SVGAFIFOFull) if the FIFO is full.
 *
 *-----------------------------------------------------------------------------
 */
// TODO spinlock or mutex me
void *
SVGA_FIFOReserve(u_int32_t bytes)  // IN
{
   volatile u_int32_t *fifo = gSVGA.fifoMem;
   u_int32_t max = fifo[SVGA_FIFO_MAX];
   u_int32_t min = fifo[SVGA_FIFO_MIN];
   u_int32_t nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
   Bool reserveable = SVGA_HasFIFOCap(SVGA_FIFO_CAP_RESERVE);

   hal_mutex_lock( &fifo_mutex );

   /*
    * This example implementation uses only a statically allocated
    * buffer.  If you want to support arbitrarily large commands,
    * dynamically allocate a buffer if and only if it's necessary.
    */

   if (bytes > sizeof gSVGA.fifo.bounceBuffer ||
       bytes > (max - min)) {
       panic("VmWare: FIFO command too large (%d > %d)", bytes, sizeof gSVGA.fifo.bounceBuffer );
   }

   if (bytes % sizeof(u_int32_t)) {
      SVGA_Panic("FIFO command length not 32-bit aligned");
   }

   if (gSVGA.fifo.reservedSize != 0) {
      SVGA_Panic("FIFOReserve before FIFOCommit");
   }

   gSVGA.fifo.reservedSize = bytes;

   while (1) {
      u_int32_t stop = fifo[SVGA_FIFO_STOP];
      Bool reserveInPlace = 0;
      Bool needBounce = 0;

      /*
       * Find a strategy for dealing with "bytes" of data:
       * - reserve in place, if there's room and the FIFO supports it
       * - reserve in bounce buffer, if there's room in FIFO but not
       *   contiguous or FIFO can't safely handle reservations
       * - otherwise, sync the FIFO and try again.
       */

      if (nextCmd >= stop) {
         /* There is no valid FIFO data between nextCmd and max */

         if (nextCmd + bytes < max ||
             (nextCmd + bytes == max && stop > min)) {
            /*
             * Fastest path 1: There is already enough contiguous space
             * between nextCmd and max (the end of the buffer).
             *
             * Note the edge case: If the "<" path succeeds, we can
             * quickly return without performing any other tests. If
             * we end up on the "==" path, we're writing exactly up to
             * the top of the FIFO and we still need to make sure that
             * there is at least one unused DWORD at the bottom, in
             * order to be sure we don't fill the FIFO entirely.
             *
             * If the "==" test succeeds, but stop <= min (the FIFO
             * would be completely full if we were to reserve this
             * much space) we'll end up hitting the FIFOFull path below.
             */
            reserveInPlace = 1;
         } else if ((max - nextCmd) + (stop - min) <= bytes) {
            /*
             * We have to split the FIFO command into two pieces,
             * but there still isn't enough total free space in
             * the FIFO to store it.
             *
             * Note the "<=". We need to keep at least one DWORD
             * of the FIFO free at all times, or we won't be able
             * to tell the difference between full and empty.
             */
            SVGAFIFOFull();
         } else {
            /*
             * Data fits in FIFO but only if we split it.
             * Need to bounce to guarantee contiguous buffer.
             */
            needBounce = 1;
         }

      } else {
         /* There is FIFO data between nextCmd and max */

         if (nextCmd + bytes < stop) {
            /*
             * Fastest path 2: There is already enough contiguous space
             * between nextCmd and stop.
             */
            reserveInPlace = 1;
         } else {
            /*
             * There isn't enough room between nextCmd and stop.
             * The FIFO is too full to accept this command.
             */
            SVGAFIFOFull();
         }
      }

      /*
       * If we decided we can write directly to the FIFO, make sure
       * the VMX can safely support this.
       */
      if (reserveInPlace) {
         if (reserveable || bytes <= sizeof(u_int32_t)) {
            gSVGA.fifo.usingBounceBuffer = 0;
            if (reserveable) {
               fifo[SVGA_FIFO_RESERVED] = bytes;
            }
            return nextCmd + (uint8*) fifo;
         } else {
            /*
             * Need to bounce because we can't trust the VMX to safely
             * handle uncommitted data in FIFO.
             */
            needBounce = 1;
         }
      }

      /*
       * If we reach here, either we found a full FIFO, called
       * SVGAFIFOFull to make more room, and want to try again, or we
       * decided to use a bounce buffer instead.
       */
      if (needBounce) {
         gSVGA.fifo.usingBounceBuffer = 1;
         return gSVGA.fifo.bounceBuffer;
      }
   } /* while (1) */
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOCommit --
 *
 *      Commit a block of FIFO data which was placed in the buffer
 *      returned by SVGA_FIFOReserve. Every Reserve must be paired
 *      with exactly one Commit, but the sizes don't have to match.
 *      The caller is free to commit less space than they
 *      reserved. This can be used if the command size isn't known in
 *      advance, but it is reasonable to make a worst-case estimate.
 *
 *      The commit size does not have to match the size of a single
 *      FIFO command. This can be used to write a partial command, or
 *      to write multiple commands at once.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_FIFOCommit(u_int32_t bytes)  // IN
{
   volatile u_int32_t *fifo = gSVGA.fifoMem;
   u_int32_t nextCmd = fifo[SVGA_FIFO_NEXT_CMD];
   u_int32_t max = fifo[SVGA_FIFO_MAX];
   u_int32_t min = fifo[SVGA_FIFO_MIN];
   Bool reserveable = SVGA_HasFIFOCap(SVGA_FIFO_CAP_RESERVE);

   if (gSVGA.fifo.reservedSize == 0) {
      SVGA_Panic("FIFOCommit before FIFOReserve");
   }
   gSVGA.fifo.reservedSize = 0;

   if (gSVGA.fifo.usingBounceBuffer) {
      /*
       * Slow paths: copy out of a bounce buffer.
       */
      uint8 *buffer = gSVGA.fifo.bounceBuffer;

      if (reserveable) {
         /*
          * Slow path: bulk copy out of a bounce buffer in two chunks.
          *
          * Note that the second chunk may be zero-length if the reserved
          * size was large enough to wrap around but the commit size was
          * small enough that everything fit contiguously into the FIFO.
          *
          * Note also that we didn't need to tell the FIFO about the
          * reservation in the bounce buffer, but we do need to tell it
          * about the data we're bouncing from there into the FIFO.
          */

         u_int32_t chunkSize = umin(bytes, max - nextCmd);
         fifo[SVGA_FIFO_RESERVED] = bytes;
         memcpy(nextCmd + (uint8*) fifo, buffer, chunkSize);
         memcpy(min + (uint8*) fifo, buffer + chunkSize, bytes - chunkSize);

      } else {
         /*
          * Slowest path: copy one dword at a time, updating NEXT_CMD as
          * we go, so that we bound how much data the guest has written
          * and the host doesn't know to checkpoint.
          */

         u_int32_t *dword = (u_int32_t *)buffer;

         while (bytes > 0) {
            fifo[nextCmd / sizeof *dword] = *dword++;
            nextCmd += sizeof *dword;
            if (nextCmd == max) {
               nextCmd = min;
            }
            fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
            bytes -= sizeof *dword;
         }
      }
   }

   /*
    * Atomically update NEXT_CMD, if we didn't already
    */
   if (!gSVGA.fifo.usingBounceBuffer || reserveable) {
      nextCmd += bytes;
      if (nextCmd >= max) {
         nextCmd -= max - min;
      }
      fifo[SVGA_FIFO_NEXT_CMD] = nextCmd;
   }

   /*
    * Clear the reservation in the FIFO.
    */
   if (reserveable) {
      fifo[SVGA_FIFO_RESERVED] = 0;
   }

   hal_mutex_unlock( &fifo_mutex );
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOCommitAll --
 *
 *      This is a convenience wrapper for SVGA_FIFOCommit(), which
 *      always commits the last reserved block in its entirety.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      SVGA_FIFOCommit.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_FIFOCommitAll(void)
{
   SVGA_FIFOCommit(gSVGA.fifo.reservedSize);
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOReserveCmd --
 *
 *      This is a convenience wrapper around SVGA_FIFOReserve, which
 *      prefixes the reserved memory block with a u_int32_t that
 *      indicates the command type.
 *
 * Results:
 *      Always returns a pointer to 'bytes' bytes of reserved space in the FIFO.
 *
 * Side effects:
 *      Begins a FIFO command, reserves space in the FIFO. Writes a
 *      1-word header into the FIFO.  May block (in SVGAFIFOFull) if
 *      the FIFO is full.
 *
 *-----------------------------------------------------------------------------
 */

void *
SVGA_FIFOReserveCmd(u_int32_t type,   // IN
                    u_int32_t bytes)  // IN
{
   u_int32_t *cmd = SVGA_FIFOReserve(bytes + sizeof type);
   cmd[0] = type;
   return cmd + 1;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_FIFOReserveEscape --
 *
 *      This is a convenience wrapper around SVGA_FIFOReserve, which
 *      prefixes the reserved memory block with an ESCAPE command header.
 *
 *      ESCAPE commands are a way of encoding extensible and
 *      variable-length packets within the basic FIFO protocol
 *      itself. ESCAPEs are used for some SVGA device functionality,
 *      like video overlays, for VMware's internal debugging tools,
 *      and for communicating with third party code that can load into
 *      the SVGA device.
 *
 * Results:
 *      Always returns a pointer to 'bytes' bytes of reserved space in the FIFO.
 *
 * Side effects:
 *      Begins a FIFO command, reserves space in the FIFO. Writes a
 *      3-word header into the FIFO.  May block (in SVGAFIFOFull) if
 *      the FIFO is full.
 *
 *-----------------------------------------------------------------------------
 */

void *
SVGA_FIFOReserveEscape(u_int32_t nsid,   // IN
                       u_int32_t bytes)  // IN
{
   u_int32_t paddedBytes = (bytes + 3) & ~3UL;
   struct {
      u_int32_t cmd;
      u_int32_t nsid;
      u_int32_t size;
   } __attribute__ ((__packed__)) *header = SVGA_FIFOReserve(paddedBytes
                                                             + sizeof *header);

   header->cmd = SVGA_CMD_ESCAPE;
   header->nsid = nsid;
   header->size = bytes;

   return header + 1;
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGAFIFOFull --
 *
 *      This function is called repeatedly as long as the FIFO has too
 *      little free space for us to continue.
 *
 *      The simplest implementation of this function is a no-op.  This
 *      will just burn guest CPU until space is available. (That's a
 *      bad idea, since the host probably needs that CPU in order to
 *      make progress on emptying the FIFO.)
 *
 *      A better implementation would sleep until a FIFO progress
 *      interrupt occurs. Depending on the OS you're writing drivers
 *      for, this may deschedule the calling task or it may simply put
 *      the CPU to sleep.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGAFIFOFull(void)
{
#if 0
   if (SVGA_IsFIFORegValid(SVGA_FIFO_FENCE_GOAL) &&
       (gSVGA.capabilities & SVGA_CAP_IRQMASK)) {

      /*
       * On hosts which support interrupts, we can sleep until the
       * FIFO_PROGRESS interrupt occurs. This is the most efficient
       * thing to do when the FIFO fills up.
       *
       * As with the IRQ-based SVGA_SyncToFence(), this will only work
       * on Workstation 6.5 virtual machines and later.
       */

      SVGA_WriteReg(SVGA_REG_IRQMASK, SVGA_IRQFLAG_FIFO_PROGRESS);
      SVGA_ClearIRQ();
      SVGA_RingDoorbell();
      SVGA_WaitForIRQ();
      SVGA_WriteReg(SVGA_REG_IRQMASK, 0);

   }
   else
#endif
   {

      /*
       * Fallback implementation: Perform one iteration of the
       * legacy-style sync. This synchronously processes FIFO commands
       * for an arbitrary amount of time, then returns control back to
       * the guest CPU.
       */
      //lprintf("vmware SVGA FIFO full\n");
      SVGA_WriteReg(SVGA_REG_SYNC, 1);
      SVGA_ReadReg(SVGA_REG_BUSY);
   }
}

/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_RingDoorbell --
 *
 *      FIFO fences are fundamentally a host-to-guest notification
 *      mechanism.  This is the opposite: we can explicitly wake up
 *      the host when we know there is work for it to do.
 *
 *      Background: The host processes the SVGA command FIFO in a
 *      separate thread which runs asynchronously with the virtual
 *      machine's CPU and other I/O devices. When the SVGA device is
 *      idle, this thread is sleeping. It periodically wakes up to
 *      poll for new commands. This polling must occur for various
 *      reasons, but it's mostly related to the historical way in
 *      which the SVGA device processes 2D updates.
 *
 *      This polling introduces significant latency between when the
 *      first new command is placed in an empty FIFO, and when the
 *      host begins processing it. Normally this isn't a huge problem
 *      since the host and guest run fairly asynchronously, but in
 *      a synchronization-heavy workload this can be a bottleneck.
 *
 *      For example, imagine an application with a worst-case
 *      synchronization bottleneck: The guest enqueues a single FIFO
 *      command, then waits for that command to finish using
 *      SyncToFence, then the guest spends a little time doing
 *      CPU-intensive processing before the cycle repeats. The
 *      workload may be latency-bound if the host-to-guest or
 *      guest-to-host notifications ever block.
 *
 *      One solution would be for the guest to explicitly wake up the
 *      SVGA3D device any time a command is enqueued. This would solve
 *      the latency bottleneck above, but it would be inefficient on
 *      single-CPU host machines. One could easily imagine a situation
 *      in which we wake up the host after enqueueing one FIFO
 *      command, the physical CPU context switches to the SVGA
 *      device's thread, the single command is processed, then we
 *      context switch back to running guest code.
 *
 *      Our recommended solution is to wake up the host only either:
 *
 *         - After a "significant" amount of work has been enqueued into
 *           the FIFO. For example, at least one 3D drawing command.
 *
 *         - After a complete frame has been rendered.
 *
 *         - Just before the guest sleeps for any reason.
 *
 *      This function implements the above guest wakeups. It uses the
 *      SVGA_FIFO_BUSY register to quickly assess whether the SVGA
 *      device may be idle. If so, it asynchronously wakes up the host
 *      by writing to SVGA_REG_SYNC.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      May wake up the SVGA3D device.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_RingDoorbell(void)
{
   if (SVGA_IsFIFORegValid(SVGA_FIFO_BUSY) &&
       gSVGA.fifoMem[SVGA_FIFO_BUSY] == 0) {

      /* Remember that we already rang the doorbell. */
      gSVGA.fifoMem[SVGA_FIFO_BUSY] = 1;

      /*
       * Asynchronously wake up the SVGA3D device.  The second
       * parameter is an arbitrary nonzero 'sync reason' which can be
       * used for debugging, but which isn't part of the SVGA3D
       * protocol proper and which isn't used by release builds of
       * VMware products.
       */
      SVGA_WriteReg(SVGA_REG_SYNC, 1);
   }
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_Update --
 *
 *      Send a 2D update rectangle through the FIFO.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *-----------------------------------------------------------------------------
 */

void
SVGA_Update(u_int32_t x,       // IN
            u_int32_t y,       // IN
            u_int32_t width,   // IN
            u_int32_t height)  // IN
{
   SVGAFifoCmdUpdate *cmd = SVGA_FIFOReserveCmd(SVGA_CMD_UPDATE, sizeof *cmd);
   cmd->x = x;
   cmd->y = y;
   cmd->width = width;
   cmd->height = height;
   SVGA_FIFOCommitAll();
}

// TODO can update not all the screen each time
static void vmware_video_update(void)
{
    if(!gSVGA.found) return;

    SVGA_Update( 0, 0, video_driver_vmware_svga.xsize, video_driver_vmware_svga.ysize );
}


/*
 *-----------------------------------------------------------------------------
 *
 * SVGA_BeginDefineCursor --
 *
 *      Begin an SVGA_CMD_DEFINE_CURSOR command. This copies the command header
 *      into the FIFO, and reserves enough space for the cursor image itself.
 *      We return pointers to FIFO memory where the AND/XOR masks can be written.
 *      When finished, the caller must invoke SVGA_FIFOCommitAll().
 *
 * Results:
 *      Returns pointers to memory where the caller can store the AND/XOR masks.
 *
 * Side effects:
 *      Reserves space in the FIFO.
 *
 *-----------------------------------------------------------------------------
 */



// ----------------------------------------------------------------------------
// Bypass 2 mouse
// ----------------------------------------------------------------------------


void vmware_draw_mouse_bp2(void)
{
    SVGA_WriteReg(SVGA_REG_CURSOR_X, video_driver_vmware_svga.mouse_x );
    //SVGA_WriteReg(SVGA_REG_CURSOR_Y, scr_get_ysize() - video_driver_vmware_svga.mouse_y );
    SVGA_WriteReg(SVGA_REG_CURSOR_Y, Y_PHANTOM_TO_HOST(video_driver_vmware_svga.mouse_y) );
    SVGA_WriteReg(SVGA_REG_CURSOR_ON, SVGA_CURSOR_ON_SHOW);
    SVGA_WriteReg(SVGA_REG_CURSOR_ON, SVGA_CURSOR_ON_REMOVE_FROM_FB);
}

#define ALPHA_BITS_RIGHT 0

static void alpha_to_bits_line( u_int8_t *bits, size_t noutbytes, rgba_t *pixels, size_t npixels, u_int32_t mask, bool invert )
{
    int bitsLeft = 8;
    *bits = 0;
    while( (npixels-- > 0) && (noutbytes > 0) )
    {
        if( bitsLeft <= 0 )
        {
            bits++;
            *bits = 0;
            bitsLeft = 8;
            noutbytes--;
        }
#if ALPHA_BITS_RIGHT
        *bits >>= 1;

        //if( pixels->a )
        if( (!!(*((int*)pixels) & mask)) ^ (!!invert) )
            *bits |= 0x80;
#else
        *bits <<= 1;

        //if( pixels->a )
        if( (!!(*((int*)pixels) & mask)) ^ (!!invert) )
            *bits |= 1;
#endif
        pixels++;
        bitsLeft--;
    }
}

static void dump_bits( u_int8_t *bits, size_t noutbytes )
{
    while( noutbytes-- > 0 )
    {
        int bitsLeft = 8;
        while( bitsLeft-- > 0 )
        {
            putchar( (*bits & 0x80) ? '*' : '.' );
            *bits <<= 1;
        }
        bits++;
    }

    //putchar( '\n' );
    printf( "\n" );
}


static void alpha_to_bits( u_int8_t *bits, size_t noutbytes, drv_video_bitmap_t *bmp, u_int32_t mask, bool invert )
{
    rgba_t *pixels = bmp->pixel;
    int xsize = bmp->xsize;
    int ysize = bmp->ysize;

    assert(xsize > 0);
    //assert(ysize > 0);

    int npixels = xsize * ysize;
    size_t skip_bytes = ((xsize-1)/8)+1;

    int ycnt = 0;

    //SHOW_FLOW( 0, "%d x %d, skip %d, npix %d", xsize, ysize, skip_bytes, npixels );

    while( ycnt < ysize )
    {
        u_int8_t *bp = bits + (skip_bytes * (ysize-ycnt-1)); // flip y

        alpha_to_bits_line( bp, skip_bytes, pixels, xsize, mask, invert );
        //dump_bits( bp, skip_bytes );
        pixels += xsize;
        npixels -= xsize;
        noutbytes -= skip_bytes;
        //bits += skip_bytes;
        ycnt++;
    }
}

// QEMU converts color cursor to bw anyway
#define CUR32 0

void vmware_set_mouse_cursor_bp2( drv_video_bitmap_t *cursor )
{
    SHOW_ERROR0( 0, "set cursor" );

    void *andMask;
    void *xorMask;

    SVGAFifoCmdDefineCursor cursorInfo;

    cursorInfo.id = 0;
    cursorInfo.hotspotX = 0;
    cursorInfo.hotspotY = 0;

    cursorInfo.width = cursor->xsize;
    cursorInfo.height = cursor->ysize;
    cursorInfo.andMaskDepth = 1;   // ignored in qemu, but seems that must be 1
#if CUR32
    cursorInfo.xorMaskDepth = 32;   // Value must be 1 or equal to BITS_PER_PIXEL
#else
    cursorInfo.xorMaskDepth = 1;   // mono is easier
#endif

    uint32 andPitch = ((cursorInfo.andMaskDepth * cursorInfo.width + 31) >> 5) << 2;
    uint32 andSize = andPitch * cursorInfo.height;
    uint32 xorPitch = ((cursorInfo.xorMaskDepth * cursorInfo.width + 31) >> 5) << 2;
    uint32 xorSize = xorPitch * cursorInfo.height;

    SVGAFifoCmdDefineCursor *cmd = SVGA_FIFOReserveCmd(SVGA_CMD_DEFINE_CURSOR,
                                                       sizeof *cmd + andSize + xorSize);

    *cmd = cursorInfo;
    andMask = (void*) (cmd + 1);
    xorMask = (void*) (andSize + (uint8*) andMask);

    bzero(andMask, andSize);
    memset(xorMask, 0xFF, xorSize);

    alpha_to_bits( andMask, andSize, cursor, 0xFF000000, 1 );
    //alpha_to_bits( xorMask, andSize, cursor, 0xFF000000, 1 );
    alpha_to_bits( xorMask, xorSize, cursor, 0x00FFFFFF, 0 );
#if !CUR32
//    alpha_to_bits( xorMask, xorSize, cursor, 0x00FFFFFF, 0 );
#endif


#if CUR32
    //bitmap2bitmap(
    bitmap2bitmap_yflip(
                   xorMask, cursor->xsize, cursor->ysize, 0, 0,
                   cursor->pixel, cursor->xsize, cursor->ysize, 0, 0,
                   cursor->xsize, cursor->ysize
                  );
#endif

    SVGA_FIFOCommitAll();
}



void vmware_mouse_off_bp2(void)
{
    //SHOW_FLOW0( 0, "cursor off" );
    //SVGA_WriteReg(SVGA_REG_CURSOR_ON, SVGA_CURSOR_ON_REMOVE_FROM_FB);
}

void vmware_mouse_on_bp2(void)
{
    //SHOW_FLOW0( 0, "cursor on" );
    //SVGA_WriteReg(SVGA_REG_CURSOR_ON, SVGA_CURSOR_ON_RESTORE_TO_FB);
}

// there is no such bit in distributed header, but QEMU reports and, supposedly, does it
static const int SVGA_CMD_RECT_FILL = 2;

typedef
struct {
   uint32 color;
   uint32 x;
   uint32 y;
   uint32 width;
   uint32 height;
} __packed
SVGAFifoCmdFill;


static void vmware_accel_clear(int xpos, int ypos, int xsize, int ysize ) // Screen rect clear
{
   SVGAFifoCmdFill *cmd = SVGA_FIFOReserveCmd(SVGA_CMD_RECT_FILL, sizeof *cmd);
   cmd->x = xpos;
   cmd->y = Y_PHANTOM_TO_HOST(ypos) - ysize;
   cmd->width = xsize;
   cmd->height = ysize;
   cmd->color = 0; // TODO add parameter and test
   SVGA_FIFOCommitAll();
}

static void vmware_accel_copy(int src_xpos, int src_ypos, int dst_xpos, int dst_ypos, int xsize, int ysize ) // Screen to screen copy                
{  
    // TODO coord sanity check and clip, QEMU does not accept out of screen coords (negative at least)      
   SVGAFifoCmdRectCopy *cmd = SVGA_FIFOReserveCmd(SVGA_CMD_RECT_COPY, sizeof *cmd);
   cmd->srcX = src_xpos;
   cmd->srcY = Y_PHANTOM_TO_HOST(src_ypos) - ysize;
   cmd->destX = dst_xpos;
   cmd->destY = Y_PHANTOM_TO_HOST(dst_ypos) - ysize;
   cmd->width = xsize;
   cmd->height = ysize;
   SVGA_FIFOCommitAll();
}













#endif // ARCH_ia32
