/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2009 Dmitry Zavalishin, dz@dz.ru
 *
 * Main part of VirtIo support.
 *
 *
**/

#define DEBUG_MSG_PREFIX "VirtIo"
#include "debug_ext.h"
#define debug_level_flow 6
#define debug_level_error 10
#define debug_level_info 10

#include "virtio.h"
#include "hal.h"
#include <virtio_pci.h>
#include <i386/pio.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <kernel/barriers.h>

static u_int32_t virtio_get_q_size( virtio_device_t *vd, int index );
static void virtio_ring_init(virtio_device_t *vd, int index, int num );

static void virtio_release_descriptor_index(virtio_ring_t *r, int toRelease);
static int virtio_get_free_descriptor_index(virtio_ring_t *r);

static void virtio_notify( virtio_device_t *vd, int index );

static void vio_irq_handler( void *arg )
{
    virtio_device_t *vd = (virtio_device_t *)arg;

    u_int8_t isr = inb( vd->basereg + VIRTIO_PCI_ISR );

    if(isr == 0)
        return; // Not ours

    SHOW_FLOW( 1, " !_INT_! %s ! ", vd->name );

    // TODO?
    //if (isr & VIRTIO_PCI_ISR_CONFIG) {


    if(vd->interrupt != 0)
        vd->interrupt( vd, isr );
}


int virtio_probe( virtio_device_t *vd, pci_cfg_t *pci )
{
    SHOW_INFO( 0, "probe got dev 0x%X class 0x%X subclass 0x%X",
           pci->device_id, pci->base_class, pci->sub_class );
//getchar();
    if( pci->device_id < 0x1000 || pci->device_id > 0x103f )
    {
        SHOW_ERROR0( 0, "Dev ID is out of virtio range");
        return -1;
    }

    if( pci->revision_id != VIRTIO_PCI_ABI_VERSION )
    {
        SHOW_ERROR( 0, "Unknown ABI ver %d", pci->revision_id );
        return -1;
    }

    vd->irq = pci->interrupt;
    vd->basereg = pci->base[0];
    vd->pci = pci;

    vd->host_features = virtio_get_features( vd );//inl(basereg+VIRTIO_PCI_HOST_FEATURES);

    if( (~(vd->host_features)) & vd->guest_features )
    {
        SHOW_ERROR( 0, "Guest wants feature (0x%X) host doesnt have (0x%X): 0x%X",
               vd->guest_features, vd->host_features,
               (~(vd->host_features)) & vd->guest_features
              );
        virtio_set_status( vd, VIRTIO_CONFIG_S_FAILED );
        return -1;
    }

    //outl(basereg+VIRTIO_PCI_GUEST_FEATURES,  );
    virtio_set_features( vd, vd->guest_features );

    virtio_set_status( vd, VIRTIO_CONFIG_S_ACKNOWLEDGE );

    if( hal_irq_alloc( pci->interrupt, vio_irq_handler, vd, HAL_IRQ_SHAREABLE ) )
    {
        SHOW_ERROR( 0, "IRQ %d is busy", pci->interrupt );
        return -1;
    }


    int qindex;
    for(qindex = 0; qindex < VIRTIO_MAX_RINGS; qindex++ )
    {
        u_int32_t qsize = virtio_get_q_size( vd, qindex );
        if( qsize == 0 )
            break;

        SHOW_INFO( 1, "Ring %d size %d, ", qindex, qsize );
        virtio_ring_init( vd, qindex, qsize );
    }

    // TODO use the subsystem vendor/device id as the virtio vendor/device id.


    return 0;
}






void virtio_reset( virtio_device_t *vd )
{
    outb( vd->basereg + VIRTIO_PCI_STATUS, 0 );
}

static void virtio_notify( virtio_device_t *vd, int index )
{
    outb( vd->basereg + VIRTIO_PCI_QUEUE_NOTIFY, index );
}



void virtio_set_status( virtio_device_t *vd, u_int8_t status )
{
    if(status == 0) panic("attempted to set zero virtio status");
    outb( vd->basereg + VIRTIO_PCI_STATUS, status );
}

u_int8_t virtio_get_status( virtio_device_t *vd )
{
    return inb( vd->basereg + VIRTIO_PCI_STATUS );
}




u_int32_t virtio_get_features( virtio_device_t *vd )
{
    return inl(vd->basereg+VIRTIO_PCI_HOST_FEATURES);
}

void virtio_set_features( virtio_device_t *vd, u_int32_t features )
{
    outl(vd->basereg+VIRTIO_PCI_GUEST_FEATURES, features );
}




void virtio_get_config_struct( virtio_device_t *vd, void *buf, unsigned len )
{
    u_int8_t *p = buf;
    unsigned int i;

    for (i = 0; i < len; i++)
        p[i] = inb( vd->basereg + VIRTIO_PCI_CONFIG + i );
}

void virtio_set_config_struct( virtio_device_t *vd, const void *buf, unsigned len )
{
    const u_int8_t *p = buf;
    unsigned int i;

    for (i = 0; i < len; i++)
        outb( vd->basereg + VIRTIO_PCI_CONFIG + i, p[i] );
}


static u_int32_t virtio_get_q_size( virtio_device_t *vd, int index )
{
    outw( vd->basereg+VIRTIO_PCI_QUEUE_SEL, index );
    return inw(vd->basereg+VIRTIO_PCI_QUEUE_NUM);
}


static void virtio_set_q_physaddr( virtio_device_t *vd, int index, physaddr_t addr )
{
    outw( vd->basereg+VIRTIO_PCI_QUEUE_SEL, index );
    outw( vd->basereg+VIRTIO_PCI_QUEUE_PFN, addr/VIRTIO_PCI_VRING_ALIGN  );
}


// ---------------------------------------------------------------------
// Virt Q support
// ---------------------------------------------------------------------

/**
 *
 * index is pos in rings[] (and in dev)
 * num is what device says in pci q size field
 *
 */
static void virtio_ring_init(virtio_device_t *vd, int index, int num )
{
    int size = vring_size( num, VIRTIO_PCI_VRING_ALIGN );

    int npages = (size-1+VIRTIO_PCI_VRING_ALIGN)/VIRTIO_PCI_VRING_ALIGN;

    SHOW_FLOW( 3, "allocating %d pages for vring, ", npages );

    physaddr_t result;
    hal_alloc_phys_pages( &result, npages);
    void *buffer = phystokv(result);
    hal_pages_control( result, buffer, npages, page_map, page_rw );

    memset( buffer, size, 0 );



    virtio_ring_t *r = (virtio_ring_t *)calloc( 1, sizeof(virtio_ring_t) );


    hal_spin_init(&r->lock);
    r->index = index;
    r->phys = result;
    r->nFreeDescriptors = num;
    r->nAdded = 0;
    r->freeHead = 0;
    r->lastUsedIdx = 0;

    int i;
    for( i = 0; i < num-1; i++ )
    {
        r->vr.desc[i].flags = VRING_DESC_F_NEXT;
        r->vr.desc[i].next = i+1;
    }
    r->vr.desc[num-1].flags = 0;
    r->vr.desc[num-1].next = -1;

    vring_init( &(r->vr), num, buffer, VIRTIO_PCI_VRING_ALIGN );


    vd->rings[index] = r;
    virtio_set_q_physaddr( vd, index, result );
}





/**
 *
 * Attach a new buffer to the queue.
 *
 * qindex - queue (ring) index.
 *
 * returns: zero on success
 *
 * Caller must call virtio_kick() after some of attaches.
 *
 */
int virtio_attach_buffer(virtio_device_t *vd, int qindex,
                         physaddr_t buf, size_t bufsize, int isWrite )
{
    virtio_ring_t *r = vd->rings[qindex];

    VIRTIO_LOCK(r);
    // TODO Errno?
    if(r->nFreeDescriptors == 0)
    {
        virtio_notify( vd, qindex ); // Try to kick host
        VIRTIO_UNLOCK(r);
        return -1; // ENOSPC
    }

    int descrIndex = virtio_get_free_descriptor_index(r);

    r->vr.desc[descrIndex].flags = isWrite ? VRING_DESC_F_WRITE : 0;
    r->vr.desc[descrIndex].addr = buf;
    r->vr.desc[descrIndex].len = bufsize;

    int availPos = r->vr.avail->idx;
    availPos %= r->vr.num;
    r->vr.avail->ring[availPos] = descrIndex;
    r->nAdded++;

    VIRTIO_UNLOCK(r);

    return 0;
}






/**
 *
 * Attach scatter/gather lists to the queue.
 *
 * qindex - queue (ring) index.
 *
 * returns: zero on success
 *
 * Caller must call virtio_kick() after some of attaches.
 *
 */
int virtio_attach_buffers(virtio_device_t *vd, int qindex,
                          int nWrite, struct vring_desc *writeData,
                          int nRead, struct vring_desc *readData
                          )
{
    virtio_ring_t *r = vd->rings[qindex];

    VIRTIO_LOCK(r);
    // TODO Errno?
    if(r->nFreeDescriptors < nWrite+nRead )
    {
        virtio_notify( vd, qindex ); // Try to kick host
        VIRTIO_UNLOCK(r);
        return -1; // ENOSPC
    }

    int descrIndex = -1;
    int elem = 0;

    while(nRead-- > 0)
    {
        int last = descrIndex;
        descrIndex = virtio_get_free_descriptor_index(r);
        r->vr.desc[descrIndex] = readData[elem++];
        r->vr.desc[descrIndex].flags = last < 0 ? 0 : VRING_DESC_F_NEXT;//isWrite ? VRING_DESC_F_WRITE : 0;
        r->vr.desc[descrIndex].next = last;
    }

    while(nWrite-- > 0)
    {
        int last = descrIndex;
        descrIndex = virtio_get_free_descriptor_index(r);
        r->vr.desc[descrIndex] = writeData[elem++];
        r->vr.desc[descrIndex].flags = VRING_DESC_F_WRITE|(last < 0 ? 0 : VRING_DESC_F_NEXT);
        r->vr.desc[descrIndex].next = last;
    }

    int availPos = r->vr.avail->idx;
    availPos %= r->vr.num;
    r->vr.avail->ring[availPos] = descrIndex;
    r->nAdded++;

    VIRTIO_UNLOCK(r);

    return 0;
}








static int virtio_have_used(const virtio_ring_t *r)
{
	return r->lastUsedIdx != r->vr.used->idx;
}


/**
 *
 * Detach returned (from device) buffer from used ring.
 *
 * Returns: 0 on success.
 *
 */
int virtio_detach_buffer( virtio_device_t *vd, int qindex,
                          physaddr_t *buf, size_t *bufsize, size_t *datalen )
{
    virtio_ring_t *r = vd->rings[qindex];

    VIRTIO_LOCK(r);
    if( !virtio_have_used(r) )
    {
        VIRTIO_UNLOCK(r);
        return -1;
    }

    int pos = r->lastUsedIdx % r->vr.num;
    unsigned int bufIndex = r->vr.used->ring[pos].id;

    if(bufIndex >= r->vr.num)
    {
        VIRTIO_UNLOCK(r);
        return -1;
    }

    *datalen = r->vr.used->ring[pos].len;
    *bufsize = r->vr.desc[bufIndex].len;
    *buf = r->vr.desc[bufIndex].addr;


    assert(! (r->vr.desc[bufIndex].flags & VRING_DESC_F_NEXT) );

    r->lastUsedIdx++;

    virtio_release_descriptor_index(r, bufIndex);

    VIRTIO_UNLOCK(r);
    return 0;
}








void virtio_kick(virtio_device_t *vd, int qindex)
{
    virtio_ring_t *r = vd->rings[qindex];

    mem_write_barrier();

    int add = r->nAdded;
    r->vr.avail->idx += add;
    r->nAdded -= add;

    mem_barrier();

#if 0 // temp notify allways
    if( !(r->vr.used->flags & VRING_USED_F_NO_NOTIFY) )
#endif
        virtio_notify( vd, qindex );
}







// must be called in lock
static int virtio_get_free_descriptor_index(virtio_ring_t *r)
{
    assert(r->freeHead >= 0);

    int ret = r->freeHead;

    r->freeHead = r->vr.desc[r->freeHead].next;

    return ret;
}

// must be called in lock
static void virtio_release_descriptor_index(virtio_ring_t *r, int toRelease)
{
    if(r->freeHead < 0)
        r->vr.desc[toRelease].flags = 0;
    else
        r->vr.desc[toRelease].flags = VRING_DESC_F_NEXT;

    r->vr.desc[toRelease].addr = NULL;
    r->vr.desc[toRelease].next = r->freeHead;
    r->freeHead = toRelease;
}


