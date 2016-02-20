/**
 *
 * Phantom OS
 *
 * Copyright (C) 2005-2010 Dmitry Zavalishin, dz@dz.ru
 *
 * UDP.
 *
**/

#include <kernel/config.h>
#include <kernel/stats.h>
#include <kernel/atomic.h>

#include <errno.h>

#if HAVE_NET
/*
 ** Copyright 2001, Travis Geiselbrecht. All rights reserved.
 ** Distributed under the terms of the NewOS License.
 */

#include "khash.h"

#include "endian.h"
#include <hal.h>

#include <compat/newos.h>
#include <kernel/net/udp.h>

#include "misc.h"



typedef struct udp_header {
    uint16 source_port;
    uint16 dest_port;
    uint16 length;
    uint16 checksum;
} _PACKED udp_header;

typedef struct udp_pseudo_header {
    ipv4_addr source_addr;
    ipv4_addr dest_addr;
    uint8 zero;
    uint8 protocol;
    uint16 udp_length;
} _PACKED udp_pseudo_header;

typedef struct udp_queue_elem {
    struct udp_queue_elem *next;
    struct udp_queue_elem *prev;
    ipv4_addr src_address;
    ipv4_addr target_address;
    uint16 src_port;
    uint16 target_port;
    int len;
    cbuf *buf;
} udp_queue_elem;

typedef struct udp_queue {
    udp_queue_elem *next;
    udp_queue_elem *prev;
    int count;
} udp_queue;

typedef struct udp_endpoint {
    struct udp_endpoint *next;
    hal_mutex_t lock;
    sem_id blocking_sem;
    uint16 port;
    udp_queue q;
    int ref_count;
} udp_endpoint;

static udp_endpoint *endpoints;
static hal_mutex_t endpoints_lock;

static int udp_endpoint_compare_func(void *_e, const void *_key)
{
    udp_endpoint *e = _e;
    const uint16 *port = _key;

    if(e->port == *port)
        return 0;
    else
        return 1;
}

static unsigned int udp_endpoint_hash_func(void *_e, const void *_key, unsigned int range)
{
    udp_endpoint *e = _e;
    const uint16 *port = _key;

    if(e)
        return e->port % range;
    else
        return *port % range;
}

static void udp_init_queue(udp_queue *q)
{
    q->count = 0;
    q->next = q->prev = (udp_queue_elem *)q;
}

static udp_queue_elem *udp_queue_pop(udp_queue *q)
{
    if(q->next != (udp_queue_elem *)q) {
        udp_queue_elem *e = q->next;

        q->next = e->next;
        e->next->prev = (udp_queue_elem *)q;
        q->count--;
        e->next = e->prev = NULL;
        return e;
    } else {
        return NULL;
    }
}

static void udp_queue_push(udp_queue *q, udp_queue_elem *e)
{
    e->prev = q->prev;
    e->next = (udp_queue_elem *)q;
    q->prev->next = e;
    q->prev = e;
    q->count++;
}

static void udp_endpoint_acquire_ref(udp_endpoint *e)
{
    ATOMIC_ADD_AND_FETCH(&e->ref_count, 1);
}

static void udp_endpoint_release_ref(udp_endpoint *e)
{
    if(atomic_add(&e->ref_count, -1) == 1) {
        udp_queue_elem *qe;

        mutex_destroy(&e->lock);
        sem_delete(e->blocking_sem);

        // clear out the queue of packets
        for(qe = udp_queue_pop(&e->q); qe; qe = udp_queue_pop(&e->q)) {
            if(qe->buf)
                cbuf_free_chain(qe->buf);
            kfree(qe);
        }
    }
}

int udp_input(cbuf *buf, ifnet *i, ipv4_addr source_address, ipv4_addr target_address)
{
    (void) i;

    udp_header *header;
    udp_endpoint *e;
    udp_queue_elem *qe;
    uint16 port;
    int err;

    STAT_INC_CNT(STAT_CNT_UDP_RX);

    header = cbuf_get_ptr(buf, 0);

#if NET_CHATTY
    dprintf("udp_input: src port %d, dest port %d, len %d, buf len %d, checksum 0x%x\n",
            ntohs(header->source_port), ntohs(header->dest_port), ntohs(header->length), (int)cbuf_get_len(buf), ntohs(header->checksum));
#endif
    if(ntohs(header->length) > (uint16)cbuf_get_len(buf)) {
        err = ERR_NET_BAD_PACKET;
        goto ditch_packet;
    }

    // deal with the checksum check
    if(header->checksum) {
        udp_pseudo_header pheader;
        uint16 checksum;

        // set up the pseudo header for checksum purposes
        pheader.source_addr = htonl(source_address);
        pheader.dest_addr = htonl(target_address);
        pheader.zero = 0;
        pheader.protocol = IP_PROT_UDP;
        pheader.udp_length = header->length;

        checksum = cbuf_ones_cksum16_2(buf, 0, ntohs(header->length), &pheader, sizeof(pheader));
        if(checksum != 0) {
#if NET_CHATTY
            dprintf("udp_receive: packet failed checksum\n");
#endif
            err = ERR_NET_BAD_PACKET;
            goto ditch_packet;
        }
    }

    // see if we have an endpoint
    port = ntohs(header->dest_port);
    mutex_lock(&endpoints_lock);
    e = hash_lookup(endpoints, &port);
    if(e)
        udp_endpoint_acquire_ref(e);
    mutex_unlock(&endpoints_lock);

    if(!e) {
        err = NO_ERROR;
#if NET_CHATTY
        dprintf("udp_receive: no endpoint found\n");
#endif
        goto ditch_packet;
    }

    // okay, we have an endpoint, lets queue our stuff up and move on
    qe = kmalloc(sizeof(udp_queue_elem));
    if(!qe) {
        udp_endpoint_release_ref(e);
        err = ERR_NO_MEMORY;
        goto ditch_packet;
    }
    qe->src_port = ntohs(header->source_port);
    qe->target_port = port;
    qe->src_address = source_address;
    qe->target_address = target_address;
    qe->len = ntohs(header->length) - sizeof(udp_header);

    // trim off the udp header
    buf = cbuf_truncate_head(buf, sizeof(udp_header), true);
    qe->buf = buf;

    mutex_lock(&e->lock);
    udp_queue_push(&e->q, qe);
    mutex_unlock(&e->lock);

    sem_release(e->blocking_sem);

    udp_endpoint_release_ref(e);

    err = NO_ERROR;
    return err;

ditch_packet:
#if NET_CHATTY
    dprintf("udp_receive: packet thrown away\n");
#endif
    cbuf_free_chain(buf);

    return err;
}

int udp_open(void **prot_data)
{
    udp_endpoint *e = calloc( sizeof(udp_endpoint), 1 );
    if(!e)
        return ERR_NO_MEMORY;

    hal_mutex_init(&e->lock, "UDP endp");
    //e->blocking_sem = sem_create(0, "udp endpoint sem");
    hal_sem_init(&(e->blocking_sem), "UDP blk");
    e->port = 0;
    e->ref_count = 1;
    udp_init_queue(&e->q);

    mutex_lock(&endpoints_lock);
    hash_insert(endpoints, e);
    mutex_unlock(&endpoints_lock);

    *prot_data = e;

    return 0;
}

int udp_bind(void *prot_data, i4sockaddr *addr)
{
    udp_endpoint *e = prot_data;
    int err;

    mutex_lock(&e->lock);

    if(e->port == 0) {
        // BUG TODO XXX search to make sure this port isn't used already
        if(addr->port != e->port) {
            // remove it from the hashtable
            mutex_lock(&endpoints_lock);
            hash_remove(endpoints, e);
            e->port = addr->port;
            hash_insert(endpoints, e);
            mutex_unlock(&endpoints_lock);
        }
        err = NO_ERROR;
    } else {
        err = ERR_NET_SOCKET_ALREADY_BOUND;
    }

    mutex_unlock(&e->lock);

    return err;
}

errno_t udp_connect(void *prot_data, i4sockaddr *addr)
{
    (void) prot_data;
    (void) addr;
    return ENOSYS;
}

errno_t udp_listen(void *prot_data)
{
    (void) prot_data;
    return ENOSYS;
}

errno_t udp_accept(void *prot_data, i4sockaddr *addr, void **new_socket)
{
    (void) prot_data;
    (void) addr;
    (void) new_socket;
    return ENOSYS;
}

errno_t udp_close(void *prot_data)
{
    udp_endpoint *e = prot_data;

    mutex_lock(&endpoints_lock);
    hash_remove(endpoints, e);
    mutex_unlock(&endpoints_lock);

    udp_endpoint_release_ref(e);

    return 0;
}

ssize_t udp_recvfrom(
                     void *prot_data,
                     void *buf, ssize_t len,
                     i4sockaddr *saddr,
                     int flags, bigtime_t timeout)
{
    udp_endpoint *e = prot_data;
    udp_queue_elem *qe;
    int err;
    ssize_t ret;

retry:
//#warning timeout ignored
#if 1
    if(flags & SOCK_FLAG_TIMEOUT)
        err = hal_sem_acquire_etc( &e->blocking_sem, 1, SEM_FLAG_TIMEOUT, timeout );
    else
#endif
        err = sem_acquire(e->blocking_sem);

    //if(err < 0)
    if(err)
        return -err;

    // pop an item off the list, if there are any
    mutex_lock(&e->lock);
    qe = udp_queue_pop(&e->q);
    mutex_unlock(&e->lock);

    if(!qe)
    {
#if 1||NET_CHATTY
        printf("UDP read retry");
#endif
        goto retry;
    }

    // we have the data, copy it out
    //err = cbuf_user_memcpy_from_chain(buf, qe->buf, 0, min(qe->len, len));
    err = cbuf_memcpy_from_chain(buf, qe->buf, 0, min(qe->len, len));
    if(err < 0) {
        ret = err;
        goto out;
    }
    ret = qe->len;

    // copy the address out
    if(saddr) {
        saddr->addr.len = 4;
        saddr->addr.type = ADDR_TYPE_IP;
        NETADDR_TO_IPV4(saddr->addr) = qe->src_address;
        saddr->port = qe->src_port;
    }

out:
    // free this queue entry
    cbuf_free_chain(qe->buf);
    kfree(qe);

    return ret;
}

ssize_t udp_sendto(void *prot_data, const void *inbuf, ssize_t len, i4sockaddr *toaddr)
{
    udp_endpoint *e = prot_data;
    udp_header *header;
    int total_len;
    cbuf *buf;
    udp_pseudo_header pheader;
    ipv4_addr srcaddr;
    int err;

    // make sure the args make sense
    if(len < 0 || len + sizeof(udp_header) > 0xffff)
        return ERR_INVALID_ARGS;
    if(toaddr->port < 0 || toaddr->port > 0xffff)
        return ERR_INVALID_ARGS;

    // allocate a buffer to hold the data + header
    total_len = len + sizeof(udp_header);
    buf = cbuf_get_chain(total_len);
    if(!buf)
        return ERR_NO_MEMORY;

    // copy the data to this new buffer
    //err = cbuf_user_memcpy_to_chain(buf, sizeof(udp_header), inbuf, len);
    err = cbuf_memcpy_to_chain(buf, sizeof(udp_header), inbuf, len);
    if(err < 0) {
        cbuf_free_chain(buf);
        return ERR_VM_BAD_USER_MEMORY;
    }

    // set up the udp pseudo header
    if(ipv4_lookup_srcaddr_for_dest(NETADDR_TO_IPV4(toaddr->addr), &srcaddr) < 0) {
        cbuf_free_chain(buf);
        return ERR_NET_NO_ROUTE;
    }
    pheader.source_addr = htonl(srcaddr);
    pheader.dest_addr = htonl(NETADDR_TO_IPV4(toaddr->addr));
    pheader.zero = 0;
    pheader.protocol = IP_PROT_UDP;
    pheader.udp_length = htons(total_len);

    // start setting up the header
    header = cbuf_get_ptr(buf, 0);
    header->source_port = htons(e->port);
    header->dest_port = htons(toaddr->port);
    header->length = htons(total_len);
    header->checksum = 0;
    header->checksum = cbuf_ones_cksum16_2(buf, 0, total_len, &pheader, sizeof(pheader));
    if(header->checksum == 0)
        header->checksum = 0xffff;

#if NET_CHATTY
    printf("UDP SEND port %d to %d, len %d (%d)\n", e->port, toaddr->port, total_len, len );
#endif
    // send it away
    err = ipv4_output(buf, NETADDR_TO_IPV4(toaddr->addr), IP_PROT_UDP);

    STAT_INC_CNT(STAT_CNT_UDP_TX);

    // if it returns ARP_QUEUED, then it's actually okay
    if(err == ERR_NET_ARP_QUEUED) {
        err = 0;
    }

    return err;
}

errno_t udp_init(void)
{
    hal_mutex_init(&endpoints_lock, "udp_endpoints");

    endpoints = hash_init(256, offsetof(udp_endpoint, next),
                          &udp_endpoint_compare_func, &udp_endpoint_hash_func);

    assert(endpoints);

    if(!endpoints)
        return ENOMEM;

    return 0;
}

#endif
