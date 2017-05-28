#define AHCI_Q 0
#define AHCI_REG_DISK 1

#if HAVE_PCI

#if !HAVE_AHCI
#include <device.h>
#include <kernel/bus/pci.h>
phantom_device_t * driver_ahci_probe( pci_cfg_t *pci, int stage )
{
    (void) pci;
    (void) stage;
    return 0;
}
#else // HAVE_AHCI

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
#define debug_level_flow 10
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/drivers.h>
#include <kernel/page.h>
#include <kernel/atomic.h>
#include <kernel/libkern.h>

#include <kernel/info/idisk.h>

#include <ia32/pio.h>
#include <errno.h>
#include <assert.h>
#include <hal.h>
#include <time.h>
#include <threads.h>
#include <disk.h>
#include <disk_q.h>

#include <pager_io_req.h>

#include <dev/pci/ahci.h>
#include <dev/sata.h>
#include <dev/ata.h>

#define MAX_PORTS 32
#define RECV_FIS_SIZE 4096

typedef struct
{
    phantom_device_t *		dev;            // disk io needs it
    int                         nport;

    int                         sig;            // Signature

    int                 	exist;
    u_int32_t                   nSect;

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
    int                 nunit;

    int                 ncs;
    ahci_port_t         port[MAX_PORTS];

    tid_t               tid;
    hal_sem_t           finsem;
} ahci_t;


static void dump_ahci_registers( void *reg );



static void ahci_interrupt(void *arg);

static int ahci_init(phantom_device_t *dev);

//static int ahci_start(phantom_device_t *dev);
//static int ahci_stop(phantom_device_t *dev);

static int ahci_write(phantom_device_t *dev, const void *buf, int len);
static int ahci_read(phantom_device_t *dev, void *buf, int len);

//static int ahci_ioctl(struct phantom_device *dev, int type, void *buf, int len);


static void ahci_process_finished_cmd(phantom_device_t *dev, int nport);

static errno_t ahci_do_inquiry(phantom_device_t *dev, int nport, void *data, size_t data_len );


//static void dump_ataid(hd_driveid_t *ataid);
//static void ahci_dump_port_info(phantom_device_t *dev, int nport );

static void ahci_connect_port( ahci_port_t *p );



static int seq_number = 0;
phantom_device_t * driver_ahci_probe( pci_cfg_t *pci, int stage )
{

    (void) stage;

    SHOW_FLOW( 1, "Probe for " DEV_NAME " stage %d", stage );

    phantom_pci_enable( pci, 1 );
    phantom_pci_dump( pci );

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

    es->nunit = seq_number-1;

    if( ahci_init(dev) )
        goto free1;
//getchar();
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
    return *(volatile u_int32_t*) (ahci_port_base(dev,port)+displ);
}

static inline void WP32( phantom_device_t *dev, int port, int displ, u_int32_t v)
{
    *(volatile u_int32_t*) (ahci_port_base(dev,port)+displ) = v;
}


#define W32(__d,__p,__v) ( *((volatile u_int32_t*)(__d->iomem+(__p))) ) = __v
#define R32(__d,__p) ( *((volatile u_int32_t*)( ((int)(__d)->iomem) + (int)(__p))) )


static errno_t ahci_init_port(phantom_device_t *dev, int nport)
{
    ahci_t *a = dev->drv_private;

    SHOW_FLOW( 1, "Init " DEV_NAME " at mem %X, port %d", dev->iomem, nport );

    ahci_port_t *p = a->port+nport;

    p->dev = dev;
    p->nport = nport;

    // TODO 64bit -- NEED some define that we support 64 bit on this arch

    hal_pv_alloc( &(p->clb_p), (void**)&(p->clb), 1024 );
    WP32( dev, nport, AHCI_P_CLB, p->clb_p );
    //WP32( dev, nport, AHCI_P_CLBU, p->clb_p >> 32 );
    memset( p->clb, 1024, 0 );

    hal_pv_alloc( &(p->fis_p), (void**)&(p->fis), RECV_FIS_SIZE );
    WP32( dev, nport, AHCI_P_FB, p->fis_p );
    //WP32( dev, nport, AHCI_P_FBU, p->fis_p >> 32 );
    memset( p->fis, RECV_FIS_SIZE, 0 );

    // We allocate 32 commands at once and fill phys addresses right now

    const unsigned int cmd_bytes = 0x100;
    assert(cmd_bytes >= sizeof(struct ahci_cmd_tab));

    physaddr_t pa;
    void *va;
    hal_pv_alloc( &(pa), &va, cmd_bytes*AHCI_CL_SIZE ); // TODO alloc a space & mem separately, map with io flag?
    memset( va, cmd_bytes*AHCI_CL_SIZE, 0 );

    p->cmds = va;

    int i;
    for( i = 0; i < AHCI_CL_SIZE; i++ )
    {
        p->clb[i].cmd_table_phys = pa + (i*cmd_bytes);
    }

    // Reset port

    WP32( dev, nport, AHCI_P_CMD, 0 ); // All off
    WP32( dev, nport, AHCI_P_SCTL, 1 ); // Reset
    hal_sleep_msec(2); // need 1 msec
    WP32( dev, nport, AHCI_P_SCTL, 0 ); // Reset done

    u_int32_t sata_status = RP32( dev, nport, AHCI_P_SSTS );
    if( (0xF & (sata_status >> 8)) == 0 )
    {
        SHOW_ERROR( 0, DEV_NAME " port %d dev not present (IPM)", nport );
        p->exist = 0;
        return ENXIO;
    }
    if( (0xF & (sata_status >> 4)) == 0 )
    {
        SHOW_ERROR( 0, DEV_NAME " port %d dev not present (SPD)", nport );
        p->exist = 0;
        return ENXIO;
    }
    if( (0xF & (sata_status >> 0)) == 0 )
    {
        SHOW_ERROR( 0, DEV_NAME " port %d phy offline, %X", nport, 0xF & sata_status );
        p->exist = 0;
        return ENXIO;
    }


    p->sig = RP32( dev, nport, AHCI_P_SIG );
    if( (p->sig == SATA_SIG_ATA) || (p->sig == 0xFFFFFFFFu) ) // TODO fix me QEMU returns this sig! Or do we read it in a wrong way?
        SHOW_INFO( 0, DEV_NAME " port %d is ATA", nport );
    else
    {
        switch( p->sig )
        {
        case SATA_SIG_ATAPI:
            SHOW_INFO( 0, DEV_NAME " port %d is ATAPI, disabled", nport );
            break;
        case SATA_SIG_SEMB:
            SHOW_INFO( 0, DEV_NAME " port %d is bridge, disabled", nport );
            break;
        case SATA_SIG_PM:
            SHOW_INFO( 0, DEV_NAME " port %d is port multiplier, disabled", nport );
            break;
        default:
            SHOW_INFO( 0, DEV_NAME " port %d is unknown sig %X, disabled", nport, p->sig );
            break;
        }
        p->exist = 0;
        return ENXIO;
    }

    // Start port

    WP32( dev, nport, AHCI_P_IE, 0xFFFF ); // Turn on all...

    WP32( dev, nport, AHCI_P_CMD, AHCI_P_CMD_FRE|AHCI_P_CMD_SUD|AHCI_P_CMD_POD );
    WP32( dev, nport, AHCI_P_CMD, AHCI_P_CMD_FRE|AHCI_P_CMD_SUD|AHCI_P_CMD_ST|AHCI_P_CMD_ACTIVE|AHCI_P_CMD_POD );

    //ahci_dump_port_info( dev, nport );

    {
        // TODO kill, replace with int buf[256], kill dump_ataid, move hd_driveid_t knowledge to parse_i_disk_ata
        //hd_driveid_t id;
        u_int16_t id[256];

        errno_t rc = ahci_do_inquiry( dev, nport, &id, sizeof(id) );
        if(rc)
        {
            SHOW_ERROR( 0, "ahci_do_inquiry rc = %d", rc );
            return rc;
        }

        //p->nSect = id.lba_capacity;
        //dump_ataid( &id );

        i_disk_t info;
        parse_i_disk_ata( &info, (void *)&id );

        p->nSect = info.nSectors;

        dump_i_disk( &info );

        ahci_connect_port( p );
    }


    return 0;
}

static void finalize_thread(void *arg);

static int ahci_init(phantom_device_t *dev)
{
    ahci_t *a = dev->drv_private;

    SHOW_FLOW( 1, "Init " DEV_NAME " at mem %X", dev->iomem );
    //SHOW_FLOW( 1, "read reg at mem %X", dev->iomem+AHCI_CAP );

    hal_sem_init( &a->finsem, DEV_NAME " fin" );
    a->tid = hal_start_thread( finalize_thread, dev, 0 );


    u_int32_t r = R32(dev,AHCI_GHC);
    if( ! (r & AHCI_GHC_AE ) )
    {
        SHOW_INFO( 1, "AHCI ENABLE for " DEV_NAME " is off (%X)", r );

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
            errno_t rc = ahci_init_port(dev, nport);
            if( rc )
            {
                SHOW_INFO( 4, "port %d init err=%d", nport, rc );
                //return ENXIO;
            }
        }
        //ports <<= 1;
        ports >>= 1;
        nport++;
    }

    // now tell us something

    printf("AHCI flags: < %s%s%s%s%s%s%s%s%s%s%s%s%s>\n",
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

    dump_ahci_registers( dev->iomem );

    return 0;
}

static void finalize_thread(void *arg)
{
    phantom_device_t *dev = arg;
    ahci_t *a = dev->drv_private;

    //hal_set_current_thread_name(DEV_NAME " drv");
    t_current_set_name(DEV_NAME " drv");
    while(1)
    {
        // TODO stop driver?
        hal_sem_acquire( &a->finsem );

        int nport;
        for( nport = 0; nport < MAX_PORTS; nport++ )
        {
            if( !a->port[nport].exist )
                continue;
            ahci_process_finished_cmd(dev, nport);
        }


    }
}

static void ahci_port_interrupt(phantom_device_t *dev, int nport)
{
    ahci_t *a = dev->drv_private;
    ahci_port_t *p = a->port+nport;

    u_int32_t is = RP32( dev, nport, AHCI_P_IS );

    u_int32_t sata_status = RP32( dev, nport, AHCI_P_SSTS );
    u_int32_t sata_control= RP32( dev, nport, AHCI_P_SCTL );
    u_int32_t sata_error  = RP32( dev, nport, AHCI_P_SERR );
    u_int32_t sata_active = RP32( dev, nport, AHCI_P_SACT );

    // Ack interrupt
    WP32( dev, nport, AHCI_P_IS, is );

    if( !p->exist )
    {
        SHOW_ERROR( 1, "Interrupt from nonexisting port %d, is %X", nport, is );
        WP32( dev, nport, AHCI_P_IE, 0 ); // Turn off!
        return;
    }

    SHOW_FLOW( 10, "Interrupt from port %d, is %X", nport, is );
    SHOW_FLOW( 10, "st %X ctl %X err %X act %X ", sata_status, sata_control, sata_error, sata_active );


    //if( is & (AHCI_P_IX_DHR|AHCI_P_IX_PS|AHCI_P_IX_DS|AHCI_P_IX_UF) )
    //    hexdump( p->fis, 0xA0, 0, 0 );

    if( is & (AHCI_P_IX_DHR) )
    {
        fis_reg_d2h_t *fis = (void *)p->fis+0x40;
        printf("D2Host FIS I%d S%02x E%x D%02x:\n", fis->i, fis->status, fis->error, fis->device);
        hexdump( p->fis+0x40, 0x14, 0, 0 );

    }

    if( is & (AHCI_P_IX_PS) )
    {
        printf("PIO setup FIS:\n");
        hexdump( p->fis+0x20, 0x14, 0, 0 );
    }

    if( is & (AHCI_P_IX_DS) )
    {
        printf("DMA setup FIS:\n");
        hexdump( p->fis, 0x1C, 0, 0 );
    }

    if( is & (AHCI_P_IX_UF) )
    {
        printf("Unknown FIS:\n");
        hexdump( p->fis+0x60, 0xA0-0x60, 0, 0 );
    }

    //ahci_process_finished_cmd(dev, nport);
    hal_sem_release( &a->finsem );

}


static void ahci_interrupt(void *arg)
{
    phantom_device_t *dev = arg;

    u_int32_t ports = R32(dev,AHCI_IS);
    u_int32_t ports_copy = ports;

    SHOW_FLOW( 10, "Interrupt from " DEV_NAME ", ports %X", ports );

    int nport = 0;
    while(ports)
    {
        if( ports & 1 )
            ahci_port_interrupt(dev, nport);
        //ports <<= 1;
        ports >>= 1;
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

//TODO check max slots value!
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



static void ahci_start_cmd(phantom_device_t *dev, int nport, int ncmd)
{
    ahci_t *a = dev->drv_private;
    SHOW_FLOW( 8, "start slot %d on port %d ", ncmd, nport );

    // TODO am I right? Don't we loose something?
    WP32( dev, nport, AHCI_P_IS, -1 ); // reset all interrupts

    WP32( dev, nport, AHCI_P_CI, 1 << ncmd);
    atomic_or( (int *)&(a->port[nport].c_started), 1 << ncmd );

}


static void ahci_wait_cmd(phantom_device_t *dev, int nport, int ncmd)
{
    //ahci_t *a = dev->drv_private;

    while( RP32( dev, nport, AHCI_P_CI ) & (1 << ncmd) )
    {
        hal_sleep_msec( 1 );
    }
}


// returns cmd index
static int ahci_build_req_cmd(phantom_device_t *dev, int nport, pager_io_request *req )
{
    ahci_t *a = dev->drv_private;

    int pFreeSlot = ahci_find_free_cmd( dev, nport );

    assert(pFreeSlot<AHCI_CL_SIZE);

    volatile struct ahci_cmd_tab *       cmd = a->port[nport].cmds+pFreeSlot;
    volatile struct ahci_cmd_list*	cp = a->port[nport].clb+pFreeSlot;

    a->port[nport].reqs[pFreeSlot] = req;

    SHOW_FLOW( 6, "slot %d rq sect %ld nsect %d", pFreeSlot, (long)req->blockNo, req->nSect );

    cp->prd_length = 1;
    cp->cmd_flags = ( (req->flag_pageout) ? AHCI_CMD_WRITE : 0);
    cp->bytecount = 0;

    // TODO assert req->nSect * 512 < max size per prd

    cmd->prd_tab[0].dba = req->phys_page;
    //cmd->prd_tab[0].dbc = (req->nSect * 512) - 1; // dbc of 0 means 1 byte!!!!
    cmd->prd_tab[0].dbc = (req->nSect * 512); // dbc of 0 means 1 byte!!!! -- check

    // This is wrong interrupt cause?
    cmd->prd_tab[0].dbc |= AHCI_PRD_IPC; // Req interrupt!

    u_int8_t fis[20];

    bzero( fis, sizeof(fis) );

    /* Construct the FIS */
    fis[0] = 0x27;		/* Host to device FIS. */
    fis[1] = 1 << 7;	        /* Command FIS. */

    fis[2] = (req->flag_pageout) ? ATA_CMD_WRITE_DMA_EXT : ATA_CMD_READ_DMA_EXT;	/* Command byte. LBA48 */
    //fis[2] = (req->flag_pageout) ? ATA_CMD_WRITE_DMA : ATA_CMD_READ_DMA;	/* Command byte. LBA28 */

    u_int32_t lba = req->blockNo;
    SHOW_FLOW( 1, "lba %d", lba );

    /* LBA48 address */
    fis[4] = lba;
    fis[5] = lba >> 8;
    fis[6] = lba >> 16;
    fis[7] = 1 << 6; // LBA48

    //fis[7] |= 0xF & (lba >> 24); // LBA28

    fis[8] = lba >> 24;
    fis[9] = 0; // use for upper LBA bytes later
    fis[10] = 0;



    u_int32_t nSect = req->nSect;

    assert(nSect <= 16); // One PRD can't process more

    assert( 0 == (nSect & 0xFFFF0000) );
    /* Sector Count */
    fis[12] = nSect;
    fis[13] = nSect >> 8;

    memcpy( cmd->cfis, fis, umin( sizeof(cmd->cfis), sizeof(fis) ) );

    SHOW_FLOW( 1, "cfis nsect %d", cmd->cfis[12] + (cmd->cfis[13] << 8) );

    hexdump( cmd->cfis, sizeof(fis), 0, 0 );
    //hexdump( cp, sizeof(*cp), 0, 0 );

    unsigned fl = sizeof(fis_reg_h2d_t);
    //unsigned fl = 16;
    cp->cmd_flags |= (( fl-1 ) >> 2 ) + 1;

    return pFreeSlot;
}

#if 0
// returns cmd index
static int ahci_build_ncq_cmd(phantom_device_t *dev, int nport, pager_io_request *req )
{
    ahci_t *a = dev->drv_private;

    int pFreeSlot = ahci_find_free_cmd( dev, nport );

    assert(pFreeSlot<AHCI_CL_SIZE);

    struct ahci_cmd_tab *       cmd = a->port[nport].cmds+pFreeSlot;
    struct ahci_cmd_list*	cp = a->port[nport].clb+pFreeSlot;

    a->port[nport].reqs[pFreeSlot] = req;

    SHOW_FLOW( 6, "rq sect %d", req->blockNo );

    // TODO move prd fill to separate func, use here and in ahci_build_req_cmd

    cp->prd_length = 1;
    cp->cmd_flags = ( (req->flag_pageout) ? AHCI_CMD_WRITE : 0);
    cp->bytecount = 0;

    // TODO assert req->nSect * 512 < max size per prd

    cmd->prd_tab[0].dba = req->phys_page;
    cmd->prd_tab[0].dbc = (req->nSect * 512) - 1; // dbc of 0 means 1 byte!!!!

    // This is wrong interrupt cause?
    cmd->prd_tab[0].dbc |= AHCI_PRD_IPC; // Req interrupt!

    u_int8_t fis[20];

    bzero( fis, sizeof(fis) );

    /* Construct the FIS */
    fis[0] = 0x27;		/* Host to device FIS. */
    fis[1] = 1 << 7;	        /* Command FIS. */

    fis[2] = (req->flag_pageout) ? ATA_CMD_WRITE_FPDMA_QUEUED : ATA_CMD_READ_FPDMA_QUEUED;	/* Command byte */

    u_int32_t lba = req->blockNo;
    SHOW_FLOW( 1, "lba %d", lba );

    /* LBA48 address */
    fis[4] = lba;
    fis[5] = lba >> 8;
    fis[6] = lba >> 16;

    //fis[7] = 1 << 6; // LBA48
    //fis[7] |= 0xF & (lba >> 24); // LBA28

    fis[8] = lba >> 24;
    fis[9] = 0; // use for upper LBA bytes later
    fis[10] = 0;

    u_int32_t nSect = req->nSect;

    assert(nSect <= 16); // One PRD can't process more
    assert( 0 == (nSect & 0xFFFF0000) );

    /* Sector Count */
    fis[3] = nSect;
    fis[11] = nSect >> 8;

    fis[12] = pFreeSlot << 3; // Tag

    memcpy( cmd->cfis, fis, umin( sizeof(cmd->cfis), sizeof(fis) ) );

    hexdump( cmd->cfis, sizeof(fis), 0, 0 );
    //hexdump( cp, sizeof(*cp), 0, 0 );

    unsigned fl = sizeof(fis_reg_h2d_t);
    //unsigned fl = 16;
    cp->cmd_flags |= fl>>2;

    return pFreeSlot;
}
#endif

// returns cmd index
static int ahci_build_fis_cmd(phantom_device_t *dev, int nport, void *fis, size_t fis_len, physaddr_t data, size_t data_len, int isWrite )
{
    ahci_t *a = dev->drv_private;

    int pFreeSlot = ahci_find_free_cmd( dev, nport );

    assert(pFreeSlot<AHCI_CL_SIZE);

    struct ahci_cmd_tab *       cmd = a->port[nport].cmds+pFreeSlot;
    struct ahci_cmd_list*	cp = a->port[nport].clb+pFreeSlot;

    a->port[nport].reqs[pFreeSlot] = 0;

    cp->prd_length = 1;
    cp->cmd_flags = ( isWrite ? AHCI_CMD_WRITE : 0);
    cp->bytecount = 0;

    cp->cmd_flags |= fis_len >> 2;

    // TODO assert data_len < max size per prd

    memcpy( cmd->cfis, fis, umin( sizeof(cmd->cfis), fis_len ) );

    cmd->prd_tab[0].dba = data;
    cmd->prd_tab[0].dbc = data_len-1;
    cmd->prd_tab[0].dbc |= AHCI_PRD_IPC; // Req interrupt!

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
    if( req != 0 )
    {
        req->rc = 0;

        // TODO check error!
#if 1
        if( cp->bytecount != ((unsigned) (req->nSect * 512)) )
        {
            req->rc = EIO;
            //req->flag_ioerror = 1;
            SHOW_ERROR( 1, "IO error port %d, expected %d bytes, got %d", nport, req->nSect * 512, cp->bytecount );
        }
#endif
        pager_io_request_done( req );
    }
}


static void ahci_process_finished_cmd(phantom_device_t *dev, int nport)
{
    ahci_t *a = dev->drv_private;

    SHOW_FLOW( 7, "look for completed slots on port %d, started %x ", nport, a->port[nport].c_started );

    while(a->port[nport].c_started)
    {
        // TODO in splinlock to prevent races?

        u_int32_t slots = RP32( dev, nport, AHCI_P_CI );
        u_int32_t done = a->port[nport].c_started & ~slots;

        if( done == 0 )
            return;

        int slot = ffs(done);
        if( slot == 0 )
            return;

        slot--; // 0 = none

        SHOW_FLOW( 8, "found completed slot %d on port %d ", slot, nport );

        ahci_finish_cmd( dev, nport, slot );

        // eat it
        atomic_and( (int *)&(a->port[nport].c_started), ~(1 << slot) );
    }

}



static errno_t ahci_sync_read(phantom_device_t *dev, int nport, void *fis, size_t fis_len, void *data, size_t data_len )
{
    errno_t rc = 0;

    physaddr_t pa;
    void *va;
    hal_pv_alloc( &pa, &va, data_len );
    //va = calloc( data_len, 1 );    pa = va;

    SHOW_FLOW( 7, "pa %p va %p", pa, va );

    int slot = ahci_build_fis_cmd( dev, nport, fis, fis_len, pa, data_len, 0 );
    ahci_start_cmd( dev, nport, slot );
    ahci_wait_cmd( dev, nport, slot );

    memcpy( data, va, data_len );
    hal_pv_free( pa, va, data_len );

    ahci_t *a = dev->drv_private;
    struct ahci_cmd_list*	cp = a->port[nport].clb+slot;

    // check error

    if( cp->bytecount != data_len )
        rc = EIO;

    return rc;
}


/* SCSI INQUIRY */

static errno_t ahci_do_inquiry(phantom_device_t *dev, int nport, void *data, size_t data_len )
{
    u_int8_t fis[20];

    bzero( fis, sizeof(fis) );

    /* Construct the FIS */
    fis[0] = 0x27;		/* Host to device FIS. */
    fis[1] = 1 << 7;	/* Command FIS. */
    fis[2] = ATA_CMD_IDENT;	/* Command byte. */

    errno_t rc = ahci_sync_read( dev, nport, fis, sizeof(fis), data, data_len );

    //if( rc )
    return rc;
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





//---------------------------------------------------------------------------
// Debug
//---------------------------------------------------------------------------




/*
static void dump_ataid(hd_driveid_t *ataid)
{
    SHOW_INFO( 0, "lba_capacity = %d Kb", ataid->lba_capacity/2 );

    SHOW_INFO( 0, "(49) capability 	= 0x%x", ataid->capability);
    SHOW_INFO( 0, "(53) field_valid 	= 0x%x", ataid->field_valid);
    SHOW_INFO( 0, "(63) dma_mword 	= 0x%x", ataid->dma_mword);
    SHOW_INFO( 0, "(64) eide_pio_modes 	= 0x%x", ataid->eide_pio_modes);
    SHOW_INFO( 0, "(75) queue_depth 	= 0x%x", ataid->queue_depth);
    SHOW_INFO( 0, "(80) major_rev_num 	= 0x%x", ataid->major_rev_num);
    SHOW_INFO( 0, "(81) minor_rev_num 	= 0x%x", ataid->minor_rev_num);
    SHOW_INFO( 0, "(82) command_set_1 	= 0x%x", ataid->command_set_1);
    SHOW_INFO( 0, "(83) command_set_2 	= 0x%x", ataid->command_set_2);
    SHOW_INFO( 0, "(84) cfsse 		= 0x%x", ataid->cfsse);
    SHOW_INFO( 0, "(85) cfs_enable_1 	= 0x%x", ataid->cfs_enable_1);
    SHOW_INFO( 0, "(86) cfs_enable_2 	= 0x%x", ataid->cfs_enable_2);
    SHOW_INFO( 0, "(87) csf_default 	= 0x%x", ataid->csf_default);
    SHOW_INFO( 0, "(88) dma_ultra 	= 0x%x", ataid->dma_ultra);
    SHOW_INFO( 0, "(93) hw_config 	= 0x%x", ataid->hw_config);
}
*/

//---------------------------------------------------------------------------
// Disk io interface
//---------------------------------------------------------------------------

// TODO specific must contain port, not dev! and need dev ptr in port



#if AHCI_Q

static void startIo( struct disk_q *q )
{
    ahci_port_t *p = q->device;
    pager_io_request *rq = q->current;
    assert(rq);

    //rq->flag_ioerror = 0;
    rq->rc = 0;

    int slot = ahci_build_req_cmd(dev, p->nport, rq );
    ahci_start_cmd( dev, p->nport, slot );
}

#else
static errno_t ahci_AsyncIo( struct phantom_disk_partition *part, pager_io_request *rq )
{
    ahci_port_t *p = part->specific;
    phantom_device_t *dev = p->dev;
    ahci_t *a = dev->drv_private;

    //rq->flag_ioerror = 0;
    rq->rc = 0;

    int slot = ahci_build_req_cmd(dev, p->nport, rq );
    ahci_start_cmd( dev, p->nport, slot );

    // uncomment to forse check op result now
    hal_sem_release( &a->finsem );


    return 0;
}
#endif

phantom_disk_partition_t *phantom_create_ahci_partition_struct( ahci_port_t *p, long size, int unit )
{
#if AHCI_Q
    phantom_disk_partition_t * ret = phantom_create_disk_partition_struct( size, p, unit, startIo );

    snprintf( ret->name, sizeof(ret->name), "AHCI%d", unit  );

    return ret;
#else
    //phantom_device_t *dev = p->dev;
    //ahci_t *a = dev->drv_private;

    phantom_disk_partition_t * ret = phantom_create_partition_struct( 0, 0, size );

    ret->asyncIo = ahci_AsyncIo;
    ret->flags |= PART_FLAG_IS_WHOLE_DISK;


    //struct disk_q *q = calloc( 1, sizeof(struct disk_q) );
    //phantom_init_disk_q( q, startIoFunc );

    ret->specific = p;
    strlcpy( ret->name, "AHCI0", sizeof(ret->name) );


    //q->device = private;
    //q->unit = unit; // if this is multi-unit device, let 'em distinguish

    // errno_t phantom_register_disk_drive(ret);

    return ret;
#endif
}


static void ahci_connect_port( ahci_port_t *p )
{

    int size = p->nSect;

    if(size <= 0)
    {
        SHOW_ERROR( 0, "Disk %d size %d?", p->nport, size );
        return;
    }

    int unit = p->nport;

    phantom_device_t *dev = p->dev;
    ahci_t *a = dev->drv_private;

    unit += 100*a->nunit;

    phantom_disk_partition_t *part = phantom_create_ahci_partition_struct( p, size, unit );
    if(part == 0)
    {
        SHOW_ERROR0( 0, "Failed to create whole disk partition" );
        return;
    }
// hangs
#if AHCI_REG_DISK
    errno_t err = phantom_register_disk_drive(part);
    if(err)
    {
        SHOW_ERROR( 0, "Disk %d err %d", p->nport, err );
        return;
    }
#endif
}



// -----------------------------------------------------------------------
// Dump port state, using alternative structs defs
// -----------------------------------------------------------------------


typedef volatile struct tagHBA_PORT
{
    DWORD	clb;		// 0x00, command list base address, 1K-byte aligned
    DWORD	clbu;		// 0x04, command list base address upper 32 bits
    DWORD	fb;		// 0x08, FIS base address, 256-byte aligned
    DWORD	fbu;		// 0x0C, FIS base address upper 32 bits
    DWORD	is;		// 0x10, interrupt status
    DWORD	ie;		// 0x14, interrupt enable
    DWORD	cmd;		// 0x18, command and status
    DWORD	rsv0;		// 0x1C, Reserved
    DWORD	tfd;		// 0x20, task file data
    DWORD	sig;		// 0x24, signature
    DWORD	ssts;		// 0x28, SATA status (SCR0:SStatus)
    DWORD	sctl;		// 0x2C, SATA control (SCR2:SControl)
    DWORD	serr;		// 0x30, SATA error (SCR1:SError)
    DWORD	sact;		// 0x34, SATA active (SCR3:SActive)
    DWORD	ci;		// 0x38, command issue
    DWORD	sntf;		// 0x3C, SATA notification (SCR4:SNotification)
    DWORD	fbs;		// 0x40, FIS-based switch control
    DWORD	rsv1[11];	// 0x44 ~ 0x6F, Reserved
    DWORD	vendor[4];	// 0x70 ~ 0x7F, vendor specific
} HBA_PORT;


typedef volatile struct tagHBA_DEV
{
    // 0x00 - 0x2B, Generic Host Control
    DWORD	cap;		// 0x00, Host capability
    DWORD	ghc;		// 0x04, Global host control
    DWORD	is;		// 0x08, Interrupt status
    DWORD	pi;		// 0x0C, Port implemented
    DWORD	vs;		// 0x10, Version
    DWORD	ccc_ctl;	// 0x14, Command completion coalescing control
    DWORD	ccc_pts;	// 0x18, Command completion coalescing ports
    DWORD	em_loc;		// 0x1C, Enclosure management location
    DWORD	em_ctl;		// 0x20, Enclosure management control
    DWORD	cap2;		// 0x24, Host capabilities extended
    DWORD	bohc;		// 0x28, BIOS/OS handoff control and status

    // 0x2C - 0x9F, Reserved
    BYTE	rsv[0xA0-0x2C];

    // 0xA0 - 0xFF, Vendor specific registers
    BYTE	vendor[0x100-0xA0];

    // 0x100 - 0x10FF, Port control registers
    HBA_PORT	ports[1];	// 1 ~ 32
} HBA_DEV;


static void dump_ahci_registers( void *reg )
{
    HBA_DEV *dev = reg;

    printf("cap 0x%08x ghc 0x%08x is 0x%08x pi 0x%08x\n", dev->cap, dev->ghc, dev->is, dev->pi );
    printf("vs %d ccc_ctl 0x%08x ccc_pts 0x%08x bohc 0x%08x\n", dev->vs, dev->ccc_ctl, dev->ccc_pts, dev->bohc );
    printf("em_loc %d em_ctl 0x%08x cap2 0x%08x\n", dev->em_loc, dev->em_ctl, dev->cap2 );

}


#endif // HAVE_AHCI
#endif // HAVE_PCI









