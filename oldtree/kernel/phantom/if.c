#include <kernel/config.h>

#if HAVE_NET

#define DEBUG_MSG_PREFIX "if"
#include <debug_ext.h>
#define debug_level_info 0
#define debug_level_flow 0
#define debug_level_error 10


/*
 ** Copyright 2001, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include <newos/nqueue.h>

#include <kernel/khash.h>
#include <kernel/net.h>
#include <kernel/net/ethernet.h>

#include <kernel/atomic.h>
#include <threads.h>

#include <endian.h>
#include <hal.h>

#include "misc.h"



#define IF_PRIO 1

#define TX_QUEUE_SIZE 64

#define LOSE_RX_PACKETS 0
#define LOSE_RX_PERCENTAGE 5

#define LOSE_TX_PACKETS 0
#define LOSE_TX_PERCENTAGE 5

static void *ifhash;
static hal_mutex_t ifhash_lock;
static if_id next_id;

static int if_compare_func(void *_i, const void *_key)
{
    struct ifnet *i = _i;
    const if_id *id = _key;

    if(i->id == *id) return 0;
    else return 1;
}

static unsigned int if_hash_func(void *_i, const void *_key, unsigned int range)
{
    struct ifnet *i = _i;
    const if_id *id = _key;

    if(i)
        return (i->id % range);
    else
        return (*id % range);
}

ifnet *if_id_to_ifnet(if_id id)
{
    ifnet *i;

    mutex_lock(&ifhash_lock);
    i = hash_lookup(ifhash, &id);
    mutex_unlock(&ifhash_lock);

    return i;
}

#if 0
ifnet *if_path_to_ifnet(const char *path)
{
    ifnet *i;
    struct hash_iterator iter;

    mutex_lock(&ifhash_lock);
    hash_open(ifhash, &iter);
    while((i = hash_next(ifhash, &iter)) != NULL) {
        if(!strcmp(path, i->path))
            break;
    }
    hash_close(ifhash, &iter, false);
    mutex_unlock(&ifhash_lock);

    return i;
}
#endif

// type is IF_TYPE_LOOPBACK, IF_TYPE_ETHERNET
//int if_register_interface(const char *path, ifnet **_i)
int if_register_interface(int type, ifnet **_i, phantom_device_t *dev)
{
    ifnet *i;
    //int type;
    int err;
    ifaddr *address;

    i = kmalloc(sizeof(ifnet));
    if(!i) {
        err = ERR_NO_MEMORY;
        goto err;
    }
    memset(i, 0, sizeof(ifnet));

    i->dev = dev;

    if(dev != 0 && (i->dev->dops.write == 0 || i->dev->dops.read == 0) )
        panic("dev has no read or write!");



    /* open the device * /
     if(!strcmp(path, "loopback")) {
     // the 'loopback' device is special
     type = IF_TYPE_LOOPBACK;
     i->fd = -1;
     } else {
     i->fd = sys_open(path, 0);
     if(i->fd < 0) {
     err = i->fd;
     goto err1;
     }
     // find the device's type
      err = sys_ioctl(i->fd, IOCTL_NET_IF_GET_TYPE, &type, sizeof(type));
      if(err < 0) {
      goto err2;
      }
      } */

    // find the appropriate function calls to the link layer drivers
    switch(type) {
    case IF_TYPE_LOOPBACK:
        i->link_input = &loopback_input;
        i->link_output = &loopback_output;
        i->mtu = 65535;
        break;
    case IF_TYPE_ETHERNET:

        assert(dev);

        i->link_input = &ethernet_input;
        i->link_output = &ethernet_output;
        i->mtu = ETHERNET_MAX_SIZE - ETHERNET_HEADER_SIZE;

        /* bind the ethernet link address */
        address = kmalloc(sizeof(ifaddr));
        address->addr.len = 6;
        address->addr.type = ADDR_TYPE_ETHERNET;

        if( dev->dops.get_address == 0 )
        {
            err = ERR_NET_GENERAL;
            kfree(address);
            goto err2;
        }

        err = dev->dops.get_address(dev, &address->addr.addr[0], 6);
        if(err < 0) {
            err = ERR_NET_GENERAL;
            kfree(address);
            goto err2;
        }

        /*err = sys_ioctl(i->fd, IOCTL_NET_IF_GET_ADDR, &address->addr.addr[0], 6);
        if(err < 0) {
            kfree(address);
            goto err2;
        }*/

        address->broadcast.len = 6;
        address->broadcast.type = ADDR_TYPE_ETHERNET;
        memset(&address->broadcast.addr[0], 0xff, 6);
        address->netmask.type = ADDR_TYPE_NULL;
        if_bind_link_address(i, address);
        break;
    default:
        err = ERR_NET_GENERAL;
        goto err1;
    }

    i->id = ATOMIC_ADD_AND_FETCH(&next_id, 1);
    //strlcpy(i->path, path, sizeof(i->path));
    i->type = type;
    i->rx_thread = -1;
    i->tx_thread = -1;

    //i->tx_queue_sem = sem_create(0, "tx_queue_sem");
    hal_sem_init( &(i->tx_queue_sem), "IF TX sem" );
    hal_mutex_init(&i->tx_queue_lock, "IF TX Q");
    fixed_queue_init(&i->tx_queue, TX_QUEUE_SIZE);

    mutex_lock(&ifhash_lock);
    hash_insert(ifhash, i);
    mutex_unlock(&ifhash_lock);

    // don't need threads for loopback if
    if(type != IF_TYPE_LOOPBACK)
    {
        /* start the rx and tx threads on this interface */
        err = if_boot_interface(i);
        if(err < 0)
            goto err2;

        //bootp(i);
    }
    *_i = i;


    return NO_ERROR;

err2:
    //sys_close(i->fd);
err1:
    kfree(i);
err:
    return err;
}

void if_bind_address(ifnet *i, ifaddr *addr)
{
    addr->if_owner = i;
    addr->next = i->addr_list;
    i->addr_list = addr;
}

void if_bind_link_address(ifnet *i, ifaddr *addr)
{
    i->link_addr = addr;
}

//! Clear all existing interface addresses and set new one
void if_replace_address(ifnet *i, ifaddr *addr)
{
    ifaddr *old;

    addr->if_owner = i;
    addr->next = 0;
    old = i->addr_list;
    i->addr_list = addr;

    while( old )
    {
        ifaddr * next = old->next;
        free(old);
        old = next;
    }
}



/*
static void bootp_thread(void *args)
{
    bootp(args);
}
*/

/*
 TODO simplify

  net = addr & netmask
  router = addr

*/

void if_simple_setup(ifnet *interface, int addr, int netmask, int bcast, int net, int router, int def_router)
{
    ifaddr *address;

    //addr = htonl(addr);
    //netmask = htonl(netmask);
    //bcast = htonl(bcast);

    // set the ip address for this net interface
    address = malloc(sizeof(ifaddr));
    address->addr.len = 4;
    address->addr.type = ADDR_TYPE_IP;
    //NETADDR_TO_IPV4(address->addr) = htonl(addr);
    NETADDR_TO_IPV4(address->addr) = addr;

    address->netmask.len = 4;
    address->netmask.type = ADDR_TYPE_IP;
    //NETADDR_TO_IPV4(address->netmask) = htonl(netmask);
    NETADDR_TO_IPV4(address->netmask) = netmask;

    address->broadcast.len = 4;
    address->broadcast.type = ADDR_TYPE_IP;
    //NETADDR_TO_IPV4(address->broadcast) = htonl(bcast);
    NETADDR_TO_IPV4(address->broadcast) = bcast;

    if_bind_address(interface, address);

#if 1
    printf("if a ");
    dump_ipv4_addr(addr);
    printf(" mask ");
    dump_ipv4_addr(netmask);
    printf(" broad ");
    dump_ipv4_addr(bcast);
    printf("\n");
#endif

    // set up an initial routing table
    int rc;

    if( (rc = ipv4_route_add( net, netmask, router, interface->id) ) )
    {
        SHOW_ERROR( 1, "Adding route - failed, rc = %d", rc);
    }
    else
    {
        SHOW_INFO0( 2, "Adding route - ok");
    }


    SHOW_INFO0( 2, "Adding default route...");
    if( (rc = ipv4_route_add_default( router, interface->id, def_router ) ) )
    {
        SHOW_ERROR( 1, "Adding route - failed, rc = %d", rc);
    }
    else
    {
        SHOW_INFO0( 2, "Adding route - ok");
    }

#if 0
    hal_start_kernel_thread_arg( bootp_thread, interface );
#else
    // Now try to get something real :)
    bootp(interface);
#endif
}





















int if_output(cbuf *b, ifnet *i)
{
    bool release_sem = false;
    bool enqueue_failed = false;
//printf("if out");

    // stick the buffer on a transmit queue
    mutex_lock(&i->tx_queue_lock);
    if(fixed_queue_enqueue(&i->tx_queue, b) < 0)
        enqueue_failed = true;
    if(i->tx_queue.count == 1)
        release_sem = true;
    mutex_unlock(&i->tx_queue_lock);

    if(enqueue_failed) {
        cbuf_free_chain(b);
        return ERR_NO_MEMORY;
    }

    if(release_sem)
        sem_release(i->tx_queue_sem);

//printf("if %x out ok\n", i);
    return NO_ERROR;
}

static void if_tx_thread(void *args)
{
    ifnet *i = args;
    cbuf *buf;
    ssize_t len;

    t_current_set_name("IF Xmit");

#if IF_PRIO
    //thread_set_priority(i->tx_thread, THREAD_MAX_RT_PRIORITY - 2);
    t_current_set_priority(PHANTOM_SYS_THREAD_PRIO);
#endif
    //if(i->fd < 0)        return -1;


//printf("if %x tx thread inited", i);
    for(;;) {
        sem_acquire(i->tx_queue_sem);
//printf("if %x tx thread gogo\n", i);

        for(;;) {
            // pull a packet out of the queue
            mutex_lock(&i->tx_queue_lock);
            buf = fixed_queue_dequeue(&i->tx_queue);
            mutex_unlock(&i->tx_queue_lock);
            if(!buf)
                break;

#if LOSE_TX_PACKETS
            if(rand() % 100 < LOSE_TX_PERCENTAGE) {
                cbuf_free_chain(buf);
                continue;
            }
#endif

            // put the cbuf chain into a flat buffer
            len = cbuf_get_len(buf);
            cbuf_memcpy_from_chain(i->tx_buf, buf, 0, len);

            cbuf_free_chain(buf);

#if 0||NET_CHATTY
            dprintf("if_tx_thread: sending packet size %ld\n", (long)len);
#endif
            //sys_write(i->fd, i->tx_buf, 0, len);
            i->dev->dops.write(i->dev, i->tx_buf, len);
        }
    }
}

static void if_rx_thread(void *args)
{
    ifnet *i = args;
    cbuf *b;

    t_current_set_name("IF Recv");
#if IF_PRIO
    //thread_set_priority(i->rx_thread, THREAD_MAX_RT_PRIORITY - 2);
    t_current_set_priority(PHANTOM_SYS_THREAD_PRIO);
#endif
    //if(i->fd < 0)        return -1;

    for(;;) {
        ssize_t len;

        //len = sys_read(i->fd, i->rx_buf, 0, sizeof(i->rx_buf));
        len = i->dev->dops.read(i->dev, i->rx_buf, sizeof(i->rx_buf));

#if 0||NET_CHATTY
        dprintf("if_rx_thread: got ethernet packet, size %ld\n", (long)len);
#endif
        if(len < 0) {
            thread_snooze(10000);
            continue;
        }
        if(len == 0)
            continue;

#if LOSE_RX_PACKETS
        if(rand() % 100 < LOSE_RX_PERCENTAGE) {
            dprintf("if_rx_thread: purposely lost packet, size %d\n", len);
            continue;
        }
#endif

        // check to see if we have a link layer address attached to us
        if(!i->link_addr) {
#if 1||NET_CHATTY
            dprintf("if_rx_thread: dumping packet because of no link address (%p)\n", i);
#endif
            continue;
        }

        // for now just move it over into a cbuf
        b = cbuf_get_chain(len);
        if(!b) {
            dprintf("if_rx_thread: could not allocate cbuf to hold ethernet packet\n");
            continue;
        }
        cbuf_memcpy_to_chain(b, 0, i->rx_buf, len);

        i->link_input(b, i);
    }

}

int if_boot_interface(ifnet *i)
{
    int err;

    // create the receive thread
    i->rx_thread = thread_create_kernel_thread("net_rx_thread", &if_rx_thread, i);
    if(i->rx_thread < 0) {
        err = i->rx_thread;
        goto err1;
    }

    // create the transmit thread
    i->tx_thread = thread_create_kernel_thread("net_tx_thread", &if_tx_thread, i);
    if(i->tx_thread < 0) {
        err = i->tx_thread;
        goto err2;
    }

    // start the threads
    //thread_resume_thread(i->rx_thread);
    //thread_resume_thread(i->tx_thread);

    return NO_ERROR;

err2:
    //thread_kill_thread_nowait(i->rx_thread);
    t_kill_thread(i->rx_thread);
err1:
    return err;
}

int if_init(void)
{
    int err;

    next_id = 0;

    // create a hash table to store the interface list
    ifhash = hash_init(16, offsetof(ifnet, next),
                       &if_compare_func, &if_hash_func);
    err = hal_mutex_init(&ifhash_lock, "if list");
    if(err < 0)
        return err;

    return NO_ERROR;
}


int if_get_count(void)
{
    return next_id;
}

#endif // HAVE_NET

