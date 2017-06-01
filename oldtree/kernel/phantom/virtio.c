/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * General VirtIo support.
 *
 *
**/

#if HAVE_PCI

#define DEBUG_MSG_PREFIX "VirtIo"
#include <debug_ext.h>
#define debug_level_flow 0
#define debug_level_error 10
#define debug_level_info 10

#include <kernel/virtio.h>
#include <hal.h>
#include <virtio_pci.h>
#include <ia32/pio.h>
#include <phantom_libc.h>
#include <kernel/vm.h>
#include <kernel/barriers.h>
#include <kernel/page.h>

static u_int32_t virtio_get_q_size( virtio_device_t *vd, int index );
static void 	virtio_ring_init(virtio_device_t *vd, int index, int num );

static void 	virtio_release_descriptor_index(virtio_ring_t *r, int toRelease);
static int 	virtio_get_free_descriptor_index(virtio_ring_t *r);

static void 	virtio_wait_for_free_descriptors(virtio_device_t *vd, int qindex, virtio_ring_t *r, int nDesc);

static void 	virtio_notify( virtio_device_t *vd, int index );

void 		virto_ring_dump(virtio_ring_t *r);



static void vio_irq_handler( void *arg )
{
    virtio_device_t *vd = (virtio_device_t *)arg;

    u_int8_t isr = inb( vd->basereg + VIRTIO_PCI_ISR );

    if(isr == 0)
        return; // Not ours

    //SHOW_FLOW( 1, " !_INT_! %s !", vd->name );

    // TODO VIRTIO_PCI_ISR_CONFIG
    //if (isr & VIRTIO_PCI_ISR_CONFIG) {


    if(vd->interrupt != 0)
        (vd->interrupt)( vd, isr );
}


int virtio_probe( virtio_device_t *vd, pci_cfg_t *pci )
{
    SHOW_INFO( 0, "probe got dev 0x%X class 0x%X subclass 0x%X",
           pci->device_id, pci->base_class, pci->sub_class );

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

    virtio_set_status( vd, VIRTIO_CONFIG_S_ACKNOWLEDGE );

    vd->host_features = virtio_get_features( vd );

    if( (~(vd->host_features)) & vd->guest_features )
    {
        SHOW_ERROR( 0, "Guest wants feature (0x%X) host doesnt have (0x%X): 0x%X",
               vd->guest_features, vd->host_features,
               (~(vd->host_features)) & vd->guest_features
              );
        virtio_set_status( vd, VIRTIO_CONFIG_S_FAILED );
        return -1;
    }

    virtio_set_features( vd, vd->guest_features );

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

    return 0;
}



// -----------------------------------------------------------------------
// VirtIO PCI IO
// -----------------------------------------------------------------------




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
    //outw( vd->basereg+VIRTIO_PCI_QUEUE_PFN, addr/VIRTIO_PCI_VRING_ALIGN  );
    outl( vd->basereg+VIRTIO_PCI_QUEUE_PFN, addr/VIRTIO_PCI_VRING_ALIGN  );
    //printf("set q %d addr %p\n", index, addr/VIRTIO_PCI_VRING_ALIGN );
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

    SHOW_FLOW( 3, "allocating %d pages for vring of size %d ", npages, num );


#if 0
    physaddr_t result;
    hal_alloc_phys_pages( &result, npages);
    void *buffer = phystokv(result);

    //hal_pages_control( result, buffer, npages, page_map_io, page_rw );
    hal_pages_control( result, buffer, npages, page_map, page_rw );
#else
    physaddr_t result;
    void *buffer;
    // TODO nocache?
    hal_pv_alloc( &result, &buffer, npages*PAGE_SIZE );
#endif

    memset( buffer, 0, size );

    SHOW_FLOW( 3, "phys mem @%p-%p ", (void *)result, (void *)result+size );


    virtio_ring_t *r = (virtio_ring_t *)calloc( 1, sizeof(virtio_ring_t) );


    hal_spin_init(&r->lock);
    r->index = index;
    r->phys = result;
    r->mem_bytes = npages*PAGE_SIZE;
    r->nFreeDescriptors = num;
    r->nAdded = 0;
    r->freeHead = 0;
    r->lastUsedIdx = 0;
    r->totalSize = num;

    SHOW_FLOW0(8, "will vring_init");
    vring_init( &(r->vr), num, buffer, VIRTIO_PCI_VRING_ALIGN );

    int i;
    for( i = 0; i < num-1; i++ )
    {
        r->vr.desc[i].flags = VRING_DESC_F_NEXT;
        r->vr.desc[i].next = i+1;
    }
    r->vr.desc[num-1].flags = 0;
    r->vr.desc[num-1].next = -1;

    vd->rings[index] = r;
    SHOW_FLOW0(8, "will virtio_set_q_physaddr");
    virtio_set_q_physaddr( vd, index, result );
}



void virto_ring_dump(virtio_ring_t *r)
{
    if( debug_level_flow < 10 ) return;

    printf("VRing @%p totalSize=%d, index=%d, phys=%p, nFreeDesc=%d, freeHead=%d, lastUsedIdx=%d \n",
           r,
           r->totalSize, r->index, (void *)(r->phys), r->nFreeDescriptors, r->freeHead, r->lastUsedIdx);

    int i;

    printf("VRing desc @%p: ", r->vr.desc);
    for( i = 0; i < r->totalSize; i++ )
    {
#if 0
        printf("%d@%p->%d/%x, \t",
               r->vr.desc[i].len, r->vr.desc[i].addr,
               (unsigned)(r->vr.desc[i].next), (unsigned)(r->vr.desc[i].flags)
              );
#else
        printf("%d@%lp->", r->vr.desc[i].len, (void *)(r->vr.desc[i].addr ));
        printf("%d/", r->vr.desc[i].next );
        printf("%x, \t", r->vr.desc[i].flags );
#endif

    }
    printf("\n");

    printf("VRing avail @%p idx=%d fl=%x: ", r->vr.avail, r->vr.avail->idx, r->vr.avail->flags );
    for( i = 0; i < r->totalSize && i < 10; i++ )
    {
        printf("%d\t",
               r->vr.avail->ring[i]
              );
    }
    printf("\n");

#if 0
    printf("VRing used @%p idx=%d fl=%x: ", r->vr.used, r->vr.used->idx, r->vr.used->flags );
    for( i = 0; i < r->totalSize; i++ )
    {
        printf("%d(%d)\t",
               r->vr.used->ring[i].id, r->vr.used->ring[i].len
              );
    }
    printf("\n");
#endif
}



// -----------------------------------------------------------------------
// VirtIO Ring - attach.detach buffers
// -----------------------------------------------------------------------


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

    assert(buf);
    assert(bufsize);

    VIRTIO_LOCK(r);
    virtio_wait_for_free_descriptors(vd, qindex, r, 1);

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
int virtio_attach_buffers_list(virtio_device_t *vd, int qindex,
                          int nDesc, struct vring_desc *desc
                          )
{
    assert( nDesc > 0 );

    virtio_ring_t *r = vd->rings[qindex];

    VIRTIO_LOCK(r);
    virtio_wait_for_free_descriptors(vd, qindex, r, nDesc);

    int descrIndex = -1;
    int last;

    // We put im in reverse order, but we read them in reverse order as well
    // So order is intact finally

    while(nDesc-- > 0)
    {
        assert(desc[nDesc].addr);
        assert(desc[nDesc].len);

        last = descrIndex;
        descrIndex = virtio_get_free_descriptor_index(r);
        r->vr.desc[descrIndex] = desc[nDesc];
        r->vr.desc[descrIndex].next = last;

        if(last < 0)
            r->vr.desc[descrIndex].flags &=~VRING_DESC_F_NEXT;
        else
            r->vr.desc[descrIndex].flags |= VRING_DESC_F_NEXT;
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
    ASSERT_VIRTIO_LOCKED(r);
    return (r->lastUsedIdx % r->vr.num) != (r->vr.used->idx % r->vr.num);
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
// ?
    //r->lastUsedIdx %= r->vr.num;

    virtio_release_descriptor_index(r, bufIndex);

    VIRTIO_UNLOCK(r);
    return 0;
}


int virtio_detach_buffers_list(virtio_device_t *vd, int qindex,
                          int nDesc, struct vring_desc *desc, int *dataLen
                          )
{
    virtio_ring_t *r = vd->rings[qindex];
    int nout = 0;

    VIRTIO_LOCK(r);
    if( !virtio_have_used(r) )
    {
        VIRTIO_UNLOCK(r);
        return -1;
    }
    assert(nDesc > 0);

    int pos = r->lastUsedIdx % r->vr.num;
    // suppose that incrementing it means we 'released' it? inc later
    //r->lastUsedIdx++;

    // ?
    //r->lastUsedIdx %= r->vr.num;

    unsigned int bufIndex = r->vr.used->ring[pos].id;

    if(bufIndex >= r->vr.num)
    {
        r->lastUsedIdx++;

        VIRTIO_UNLOCK(r);
        SHOW_ERROR(0, "bufIndex >= r->vr.num (%d)", bufIndex);
        return -1;
    }

    *dataLen = r->vr.used->ring[pos].len;

again:


    desc[nout] = r->vr.desc[bufIndex];
    assert(desc[nout].addr);     // to catch buggy detach
    nout++;

    // debug only - to catch buggy detach
    assert(nout < 4);

    int flagsCopy = r->vr.desc[bufIndex].flags;
    int nextCopy = r->vr.desc[bufIndex].next;

    SHOW_FLOW( 2, "detach desc from pos %d -> %d", bufIndex, nextCopy );

    virtio_release_descriptor_index(r, bufIndex);

    nDesc--;

    if( (flagsCopy & VRING_DESC_F_NEXT) )
    {
        bufIndex = nextCopy;
        if(nDesc > 0)
            goto again;
        else
            SHOW_ERROR0(0, "Can't recv descriptor");
    }


        r->lastUsedIdx++;

    VIRTIO_UNLOCK(r);
    return nout;
}





void virtio_kick(virtio_device_t *vd, int qindex)
{
    virtio_ring_t *r = vd->rings[qindex];

    SHOW_FLOW( 2, "Will kick %p, q %d", vd, qindex);

    mem_write_barrier();

    int add = r->nAdded;
    //r->vr.avail->idx += add;
    int tmp = r->vr.avail->idx + add;
    r->nAdded -= add;

    //r->vr.avail->idx = tmp % r->vr.num; // QEMU dies with "Guest moved used index from 127 to 0"
    r->vr.avail->idx = tmp % 0xFFFF;

    mem_barrier();

//#if 0 // temp notify allways
    if( !(r->vr.used->flags & VRING_USED_F_NO_NOTIFY) )
//#endif
        virtio_notify( vd, qindex );
}


// -----------------------------------------------------------------------
// VirtIO descriptors alloc/free
// -----------------------------------------------------------------------





// must be called in lock
static int virtio_get_free_descriptor_index(virtio_ring_t *r)
{
    ASSERT_VIRTIO_LOCKED(r);
    assert(r->freeHead >= 0);
    assert(r->freeHead < 0xFFFF );
    assert(r->nFreeDescriptors > 0);

    r->nFreeDescriptors--;

    int ret = r->freeHead;

    r->freeHead = r->vr.desc[r->freeHead].next;
    assert(r->freeHead >= 0);
    assert(r->freeHead < 0xFFFF );

    return ret;
}

// must be called in lock
static void virtio_release_descriptor_index(virtio_ring_t *r, int toRelease)
{
    ASSERT_VIRTIO_LOCKED(r);

    if( (r->freeHead < 0) || (r->freeHead >= 0xFFFF) )
        r->vr.desc[toRelease].flags = 0;
    else
        r->vr.desc[toRelease].flags = VRING_DESC_F_NEXT;

    r->vr.desc[toRelease].addr = NULL;
    r->vr.desc[toRelease].next = r->freeHead;
    r->freeHead = toRelease;

    r->nFreeDescriptors++;
}



// Sleep waiting for interrupt, retry
static void virtio_wait_for_free_descriptors(virtio_device_t *vd, int qindex, virtio_ring_t *r, int nDesc)
{
    ASSERT_VIRTIO_LOCKED(r);

    // TODO dumb impl with sleep
    while(r->nFreeDescriptors < nDesc )
    {
        virtio_notify( vd, qindex ); // Try to kick host
        VIRTIO_UNLOCK(r);
        //return -1; // ENOSPC
        hal_sleep_msec(10);
        VIRTIO_LOCK(r);
    }
}


// -----------------------------------------------------------------------
// VirtIO Debug
// -----------------------------------------------------------------------



void dump_phys( physaddr_t a, size_t len )
{
    printf( "dump physaddr %p\n, ", (void *)a );

    char buf[PAGE_SIZE];
    while(len > 0)
    {
        int ml = len;
        if(ml > PAGE_SIZE) ml = PAGE_SIZE;

        len -= ml;

        memcpy_p2v(buf, a, ml);

        a += ml;

        hexdump( buf, ml, "", 0);
    }
}

void virtio_dump_phys(virtio_ring_t *r)
{
    dump_phys(r->phys, r->mem_bytes);
}


#endif // HAVE_PCI



