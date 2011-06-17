#ifdef ARCH_ia32 // TODO need PCI flag instead
/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2011 Dmitry Zavalishin, dz@dz.ru
 *
 * IDE AHCI driver.
 *
**/
#define DEV_NAME "ahci"
#define DEBUG_MSG_PREFIX "ahci"
#include <debug_ext.h>
#define debug_level_flow 9
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <kernel/page.h>
#include <kernel/atomic.h>
#include <kernel/libkern.h>

#include <i386/pio.h>
#include <errno.h>
#include <assert.h>
#include <hal.h>
#include <time.h>

#include <pager_io_req.h>

#include <dev/pci/ahci.h>

typedef struct
{
    int                 	exist;

    u_int32_t                   c_started; // which commands are started - to compare with running list

    physaddr_t          	clb_p;
    struct ahci_cmd_list*	clb;

    physaddr_t          	fis_p;
    void *              	fis;

    struct ahci_cmd_tab *       cmds;

    pager_io_request *          reqs[AHCI_CL_SIZE];
} ahci_port_t;


typedef struct
{
    int                 ncs;
    ahci_port_t         port[32];
} ahci_t;

static void ahci_interrupt(void *arg);

static int ahci_init(phantom_device_t *dev);

//static int ahci_start(phantom_device_t *dev);
//static int ahci_stop(phantom_device_t *dev);

static int ahci_write(phantom_device_t *dev, const void *buf, int len);
static int ahci_read(phantom_device_t *dev, void *buf, int len);

//static int ahci_ioctl(struct phantom_device *dev, int type, void *buf, int len);


static void ahci_process_finished_cmd(phantom_device_t *dev, int nport);




static int seq_number = 0;
phantom_device_t * driver_ahci_probe( pci_cfg_t *pci, int stage )
{

    (void) stage;

    SHOW_FLOW( 1, "Probe for " DEV_NAME " stage %d", stage );

    phantom_device_t * dev = malloc(sizeof(phantom_device_t));

    int i;
    for (i = 0; i < 6; i++)
    {
        if (pci->base[i] > 0xffff)
        {
            dev->iomem = (pci->base[i]);
            dev->iomemsize = pci->size[i];
            SHOW_INFO( 1, "mem base 0x%lx, size 0x%lx", dev->iomem, dev->iomemsize);
        } else if( pci->base[i] > 0) {
            dev->iobase = pci->base[i];
            SHOW_INFO( 1, "io_port 0x%x", dev->iobase);
        }
    }

    dev->irq = pci->interrupt;


    const int n_pages = BYTES_TO_PAGES(dev->iomemsize);
    void *va;
    if( hal_alloc_vaddress(&va,n_pages ) )
        panic("Can't alloc vaddress for %d mem pages", n_pages);

    hal_pages_control_etc( dev->iomem, va, n_pages, page_map_io, page_rw, 0 );

    dev->iomem = (addr_t)va; // loose phys addr?!

    /*
    // Gets port 0. uninited by BIOS? Need explicit PCI io addr assign?
    if( dev->iobase == 0 )
    {
        SHOW_ERROR0( 0, "No io port?" );
        goto free;
    }
    */

    SHOW_FLOW( 1, "Look for " DEV_NAME " at mem %X", dev->iomem );
    //if( check_ahci_sanity(dev->iobase) )        goto free;

    dev->name = DEV_NAME;
    dev->seq_number = seq_number++;

    //dev->dops.start = ahci_start;
    //dev->dops.stop  = ahci_stop;
    dev->dops.read  = ahci_read;
    dev->dops.write = ahci_write;
    //dev->dops.ioctl = ahci_ioctl;

    if( hal_irq_alloc( dev->irq, &ahci_interrupt, dev, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", dev->irq );
        goto free;
    }

    ahci_t *es = calloc(1,sizeof(ahci_t));
    assert(es);
    dev->drv_private = es;

    if( ahci_init(dev) )
        goto free1;

    return dev;
free1:
    free(es);

free:
    free(dev);
    return 0;
}


static inline addr_t ahci_port_base( phantom_device_t *dev, int port)
{
    return dev->iomem + 0x100 + (port * 0x80);
}

static inline u_int32_t RP32( phantom_device_t *dev, int port, int displ)
{
    return *(u_int32_t*) (ahci_port_base(dev,port)+displ);
}

static inline void WP32( phantom_device_t *dev, int port, int displ, u_int32_t v)
{
    *(u_int32_t*) (ahci_port_base(dev,port)+displ) = v;
}


#define W32(__d,__p,__v) ( *((u_int32_t*)(__d->iomem+(__p))) ) = __v
#define R32(__d,__p) ( *((u_int32_t*)( ((int)(__d)->iomem) + (int)(__p))) )


static int ahci_init_port(phantom_device_t *dev, int nport)
{
    ahci_t *a = dev->drv_private;

    SHOW_FLOW( 1, "Init " DEV_NAME " at mem %X, port %d", dev->iomem, nport );

    ahci_port_t *p = a->port+nport;

    // TODO 64bit -- NEED some define that we support 64 bit on this arch

    hal_pv_alloc( &(p->clb_p), (void**)&(p->clb), 1024 );
    WP32( dev, nport, AHCI_P_CLB, p->clb_p );
    //WP32( dev, nport, AHCI_P_CLBU, p->clb_p >> 32 );
    memset( p->clb, 1024, 0 );

    hal_pv_alloc( &(p->fis_p), (void**)&(p->fis), 4096 );
    WP32( dev, nport, AHCI_P_FB, p->fis_p );
    //WP32( dev, nport, AHCI_P_FBU, p->fis_p >> 32 );
    memset( p->fis, 4096, 0 );

    // We allocate 32 commands at once and fill phys addresses right now

    const unsigned int cmd_bytes = 0x100;
    assert(cmd_bytes >= sizeof(struct ahci_cmd_tab));

    physaddr_t pa;
    void *va;
    hal_pv_alloc( &(pa), &va, cmd_bytes*AHCI_CL_SIZE );
    memset( va, cmd_bytes*AHCI_CL_SIZE, 0 );

    p->cmds = va;

    int i;
    for( i = 0; i < AHCI_CL_SIZE; i++ )
    {
        p->clb[i].cmd_table_phys = pa + (i*cmd_bytes);
    }

    WP32( dev, nport, AHCI_P_IE, 0xFFFF ); // Turn on all...

    WP32( dev, nport, AHCI_P_CMD, AHCI_P_CMD_FRE|AHCI_P_CMD_SUD );
    WP32( dev, nport, AHCI_P_CMD, AHCI_P_CMD_FRE|AHCI_P_CMD_SUD|AHCI_P_CMD_ST );

    return 0;
}


static int ahci_init(phantom_device_t *dev)
{
    ahci_t *a = dev->drv_private;

    SHOW_FLOW( 1, "Init " DEV_NAME " at mem %X", dev->iomem );
    //SHOW_FLOW( 1, "read reg at mem %X", dev->iomem+AHCI_CAP );

    u_int32_t r = R32(dev,AHCI_GHC);
    if( ! (r & AHCI_GHC_AE ) )
    {
        SHOW_INFO( 1, "AHI ENABLE for " DEV_NAME " is off (%X)", r );

        W32(dev,AHCI_GHC, r | AHCI_GHC_AE );

        r = R32(dev,AHCI_GHC);
        if( ! (r & AHCI_GHC_AE ) )
        {
            SHOW_ERROR( 1, "Unable to ENABLE " DEV_NAME " (%X)", r );
            return ENXIO;
        }

        W32(dev,AHCI_GHC, r | AHCI_GHC_HR ); // Reset
        W32(dev,AHCI_GHC, r | AHCI_GHC_IE ); // Enable interrupts

    }

    // TODO bios ownership?

    u_int32_t cap = R32(dev,AHCI_CAP);
    //u_int32_t cap = *(u_int32_t *)(dev->iomem+AHCI_CAP);

    int nports = 1 + (cap & AHCI_CAP_NPMASK);
    a->ncs = 1 + ((cap & AHCI_CAP_NCS) >> AHCI_CAP_NCS_SHIFT);

    SHOW_INFO(0,"ports %d, cs %d", nports, a->ncs );

    u_int32_t ports = R32(dev,AHCI_PI);

    //SHOW_FLOW( 1, " " DEV_NAME ", ports %X", ports );

    int nport = 0;
    while(ports)
    {
        if( ports & 1 )
        {
            a->port[nport].exist = 1;
            SHOW_INFO( 4, "port %d implemented", nport );
            if( ahci_init_port(dev, nport) )
                return ENXIO;
        }
        ports <<= 1;
        nport++;
    }

    // now tell us something

    printf("AHCI flags: %s%s%s%s%s%s%s%s%s%s%s%s%s\n",
           cap & (1 << 31) ? "64bit " : "",
           cap & (1 << 30) ? "ncq " : "",
           cap & (1 << 28) ? "ilck " : "",
           cap & (1 << 27) ? "stag " : "",
           cap & (1 << 26) ? "pm " : "",
           cap & (1 << 25) ? "led " : "",
           cap & (1 << 24) ? "clo " : "",
           cap & (1 << 19) ? "nz " : "",
           cap & (1 << 18) ? "only " : "",
           cap & (1 << 17) ? "pmp " : "",
           cap & (1 << 15) ? "pio " : "",
           cap & (1 << 14) ? "slum " : "",
	       cap & (1 << 13) ? "part " : "");

    return 0;
}

static void ahci_port_interrupt(phantom_device_t *dev, int nport)
{
    ahci_t *a = dev->drv_private;

    u_int32_t is = RP32( dev, nport, AHCI_P_IS );
    WP32( dev, nport, AHCI_P_IS, is );

    if( !a->port[nport].exist )
    {
        SHOW_ERROR( 1, "Interrupt from nonexisting port %d, is %X", nport, is );
        WP32( dev, nport, AHCI_P_IE, 0 ); // Turn off!
        return;
    }

    SHOW_FLOW( 1, "Interrupt from port %d, is %X", nport, is );

    ahci_process_finished_cmd(dev, nport);
}


static void ahci_interrupt(void *arg)
{
    phantom_device_t *dev = arg;

    u_int32_t ports = R32(dev,AHCI_IS);
    u_int32_t ports_copy = ports;

    SHOW_FLOW( 1, "Interrupt from " DEV_NAME ", ports %X", ports );

    int nport = 0;
    while(ports)
    {
        if( ports & 1 )
            ahci_port_interrupt(dev, nport);
        ports <<= 1;
        nport++;
    }

    W32(dev,AHCI_IS, ports_copy );

}

static void ahci_wait_for_port_interrupt(phantom_device_t *dev, int nport)
{
    (void) dev;
    (void) nport;

    // XXX BUG TODO
    hal_sleep_msec(1);
}


static int ahci_find_free_cmd(phantom_device_t *dev, int nport)
{
    ahci_t *a = dev->drv_private;

    while(1)
    {
        u_int32_t slots = RP32(dev, nport, AHCI_P_CI);

        slots &= RP32(dev, nport, AHCI_P_SACT );

        slots |= a->port[nport].c_started;

        int slot = ffr(slots);
        if( slot == 0 )
            ahci_wait_for_port_interrupt(dev, nport);

        SHOW_FLOW( 8, "found slot %d on port %d ", slot-1, nport );
        return slot - 1; // 0 = none
    }

}



// returns cmd index
static void ahci_start_cmd(phantom_device_t *dev, int nport, int ncmd)
{
    ahci_t *a = dev->drv_private;
    WP32( dev, nport, AHCI_P_CI, 1 >> ncmd);
    atomic_or( &(a->port[nport].c_started), 1 >> ncmd );
}

// returns cmd index
static int ahci_build_cmd(phantom_device_t *dev, int nport, pager_io_request *req )
{
    ahci_t *a = dev->drv_private;

    int pFreeSlot = ahci_find_free_cmd( dev, nport );

    assert(pFreeSlot<AHCI_CL_SIZE);

    struct ahci_cmd_tab *       cmd = a->port[nport].cmds+pFreeSlot;
    struct ahci_cmd_list*	cp = a->port[nport].clb+pFreeSlot;

    a->port[nport].reqs[pFreeSlot] = req;

    cp->prd_length = 1;
    cp->cmd_flags = ( (req->flag_pageout) ? AHCI_CMD_WRITE : 0);
    cp->bytecount = 0;

    // TODO assert req->nSect * 512 < max size per prd

    cmd->prd_tab[0].dba = req->phys_page;
    cmd->prd_tab[0].dbc = req->nSect * 512;

    return pFreeSlot;
}


static void ahci_finish_cmd(phantom_device_t *dev, int nport, int slot)
{
    ahci_t *a = dev->drv_private;
    //ahci_port_t *p = a->port+nport;

    //struct ahci_cmd_tab *       cmd = a->port[nport].cmds+slot;
    struct ahci_cmd_list*	cp = a->port[nport].clb+slot;
    pager_io_request *          req = a->port[nport].reqs[slot];

    // Now do it

    req->rc = 0;

    // TODO check error!

    if( cp->bytecount != req->nSect * 512 )
    {
        req->rc = EIO;
        req->flag_ioerror = 1;
    }

    if(req) pager_io_request_done( req );
}


static void ahci_process_finished_cmd(phantom_device_t *dev, int nport)
{
    ahci_t *a = dev->drv_private;

    while(a->port[nport].c_started)
    {
        // TODO in splinlock to prevent races?

        u_int32_t slots = RP32( dev, nport, AHCI_P_CI );
        u_int32_t done = a->port[nport].c_started & ~slots;

        if( done == 0 )
            return;

        int slot = ffr(slots);
        if( slot == 0 )
            return;

        slot--; // 0 = none

        SHOW_FLOW( 8, "found completed slot %d on port %d ", slot, nport );


        ahci_finish_cmd( dev, nport, slot );

        // eat it
        atomic_and( &(a->port[nport].c_started), ~(1 >> slot) );
    }

}













static int ahci_read(phantom_device_t *dev, void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    //ahci_t *es = dev->drv_private;

    return -1;
}

static int ahci_write(phantom_device_t *dev, const void *buf, int len)
{
    (void) dev;
    (void) buf;
    (void) len;

    //ahci_t *es = dev->drv_private;

    return -1;
}





#endif // ARCH_ia32

